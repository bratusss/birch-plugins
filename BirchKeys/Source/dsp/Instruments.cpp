#include "Instruments.h"

// Every number here comes from RESEARCH.md: measured mode ratios, decay times, pickup
// behaviour, filter frequencies. Each instrument is a different mechanism, not a preset.

namespace bk
{

namespace
{
    void addMode (InstrumentDef& d, float ratio, float amp, float decay, float velBright)
    {
        d.modes[d.numModes++] = { ratio, amp, decay, velBright };
    }

    void setDrawbars (InstrumentDef& d, const char* registration) // e.g. "888000000"
    {
        for (int i = 0; i < 9; ++i)
            d.drawbars[i] = (float) (registration[i] - '0');
    }

    void setBody (InstrumentDef& d, int i, float hz, float q, float gain) { d.body[i] = { hz, q, gain }; }

    std::vector<InstrumentDef> build()
    {
        std::vector<InstrumentDef> list;

        //==================================================================== KEYS
        {
            // ISMA 2014: after 10-14 ms the tine is a pure sine; tonebar (own f0 far below the note)
            // and upper tine modes exist only in that transient. Growl = Gaussian pickup field.
            InstrumentDef d;
            d.name = "Rhodes Mk I";
            d.category = "Keys";
            d.physics = "Tērauda tine: pēc ~12 ms tīrs sinuss (ISMA 2014). Tonebar un augstās modas tikai transientā. Pickup lauks ir zvana līkne: centrā oktāva, malā pamattonis + growl.";
            d.engine = Engine::modal;
            d.pickup = Pickup::magnetic;
            addMode (d, 1.0f, 1.0f, 3.0f, 0.0f);          // tine, T60 ~7 s mid keyboard
            addMode (d, 6.27f, 0.6f, 0.012f, 0.6f);       // 2nd cantilever mode, transient only
            addMode (d, 17.55f, 0.25f, 0.006f, 0.9f);
            d.tonebarRatio = 0.4f;                        // table 1: 105 Hz tonebar under a 263 Hz note
            d.transientMs = 13.0f;
            d.pickupOffset = 0.4f;
            d.deflection = 1.0f;
            d.decayKeytrack = 0.62f;
            d.releaseTau = 0.08f;
            d.characterDefault = 0.5f;
            d.motion = Motion::stereoTremolo;
            d.motionRate = 4.2f;
            d.motionDefault = 0.3f;
            d.noiseAmount = 0.06f;                        // neoprene hammer thud, 6.4 ms contact
            d.noiseFreq = 600.0f;
            d.noiseDecay = 0.005f;
            d.lowpass = 7000.0f;                          // suitcase speaker roll-off
            d.gain = 0.3f;
            d.spread = 0.25f;
            list.push_back (d);
        }
        {
            // DAGA 2017: reed moves sinusoidally; capacitive pickup is asymmetric (i = V dC/dt),
            // richer harmonics with velocity, beating of upper partials, then an RC high-pass.
            InstrumentDef d;
            d.name = "Wurlitzer 200A";
            d.category = "Keys";
            d.physics = "Tērauda mēlīte ar lodes masu galā. Elektrostatiskais pickup: strāva = V·dC/dt, kondensators 1/(d−x) ir asimetrisks, tāpēc pāra harmonikas un „bark”. RC high-pass ~150 Hz.";
            d.engine = Engine::modal;
            d.pickup = Pickup::electrostatic;
            addMode (d, 1.0f, 1.0f, 1.6f, 0.0f);
            addMode (d, 1.0012f, 0.3f, 1.1f, 0.0f);       // reed/plate coupling partner, slow beat
            addMode (d, 6.1f, 0.3f, 0.02f, 0.7f);         // solder tip lowers the 6.27 cantilever ratio
            addMode (d, 15.8f, 0.1f, 0.008f, 0.9f);
            d.transientMs = 25.0f;
            d.deflection = 1.0f;
            d.decayKeytrack = 0.66f;
            d.releaseTau = 0.06f;
            d.characterDefault = 0.5f;
            d.motion = Motion::tremolo;
            d.motionRate = 5.6f;                          // twin-T oscillator in the 200A
            d.motionDefault = 0.0f;
            d.noiseAmount = 0.05f;
            d.noiseFreq = 1400.0f;
            d.noiseDecay = 0.004f;
            d.lowpass = 5500.0f;                          // small internal speakers
            d.gain = 0.22f;
            d.spread = 0.15f;
            list.push_back (d);
        }
        {
            // Hohner Pianet: the reed is plucked by a sticky pad (no hammer), so a smooth onset
            InstrumentDef d;
            d.name = "Pianet";
            d.category = "Keys";
            d.physics = "Hohner Pianet: mēlīti atrauj lipīgs spilventiņš, nevis sit āmurs. Mīksts ieskaņojums bez klikšķa, gandrīz tīrs tonis, īss sustain.";
            d.engine = Engine::modal;
            d.pickup = Pickup::electrostatic;
            addMode (d, 1.0f, 1.0f, 0.9f, 0.0f);
            addMode (d, 6.2f, 0.12f, 0.03f, 0.3f);
            d.transientMs = 40.0f;
            d.deflection = 0.55f;
            d.attack = 0.006f;
            d.decayKeytrack = 0.7f;
            d.releaseTau = 0.05f;
            d.characterDefault = 0.3f;
            d.noiseAmount = 0.0f;
            d.lowpass = 6000.0f;
            d.gain = 0.4f;
            d.spread = 0.2f;
            list.push_back (d);
        }
        {
            InstrumentDef d;
            d.name = "Grand Piano";
            d.category = "Keys";
            d.physics = "Garas stīgas: mazs neharmoniskums (B≈3·10⁻⁴ pie C4), 3 stīgas uz noti ar divpakāpju izdzišanu, filca āmurs (kontakts 1–4 ms, spožāks stiprāk sitot), liels skaņas galds.";
            d.engine = Engine::modal;
            d.pianoPartials = true;
            d.inharmonicity = 0.00032f;
            d.unisonCents = 0.9f;
            d.baseDecay = 4.0f;                           // aftersound T60 ~9 s at C4
            d.hammerBright = 1.0f;
            d.hammerPos = 0.118f;
            d.promptShare = 0.6f;
            d.promptRatio = 0.2f;
            d.maxPartials = 24;
            d.decayKeytrack = 0.62f;
            d.releaseTau = 0.11f;
            d.noiseAmount = 0.2f;
            d.noiseFreq = 250.0f;
            d.noiseDecay = 0.014f;
            setBody (d, 0, 95.0f, 1.2f, 0.25f);           // soundboard low resonance
            setBody (d, 1, 180.0f, 1.0f, 0.15f);
            d.gain = 0.42f;
            d.spread = 0.55f;
            list.push_back (d);
        }
        {
            InstrumentDef d;
            d.name = "Upright Piano";
            d.category = "Keys";
            d.physics = "Īsas, resnas stīgas: liels neharmoniskums (B≈9·10⁻⁴), nesaskaņotas unisona stīgas, īsāks sustain, kastes rezonanses ap 200–400 Hz un dzirdams āmura „thump”.";
            d.engine = Engine::modal;
            d.pianoPartials = true;
            d.inharmonicity = 0.0009f;
            d.unisonCents = 3.5f;
            d.baseDecay = 2.4f;
            d.hammerBright = 0.85f;
            d.hammerPos = 0.125f;
            d.promptShare = 0.7f;
            d.promptRatio = 0.22f;
            d.maxPartials = 20;
            d.decayKeytrack = 0.66f;
            d.releaseTau = 0.13f;
            d.noiseAmount = 0.35f;
            d.noiseFreq = 320.0f;
            d.noiseDecay = 0.012f;
            setBody (d, 0, 210.0f, 2.0f, 0.3f);
            setBody (d, 1, 380.0f, 2.2f, 0.2f);
            d.lowpass = 9000.0f;
            d.gain = 0.5f;
            d.spread = 0.45f;
            list.push_back (d);
        }
        {
            // EURASIP 2013: T60 up to 20 s, triangular tangent pulses, two pickups that comb-filter,
            // a knock on release from the tangent hitting the stud.
            InstrumentDef d;
            d.name = "Clavinet D6";
            d.category = "Keys";
            d.physics = "Stīgu sit gumijas tangents tuvu galam un tur piespiestu (T60 līdz 20 s). Divi magnētiskie pickup ar comb filtru, LC rezonanse ~4 kHz, un klikšķis atlaižot.";
            d.noiseFreq = 2600.0f;
            d.engine = Engine::string;
            d.t60 = 12.0f;
            d.stringDamping = 0.1f;
            d.pluckPos = 0.07f;
            d.pluckWidth = 0.012f;
            d.pluckNoise = 0.35f;
            d.pickupPos = 0.14f;
            d.pickupResHz = 4200.0f;
            d.pickupResQ = 2.2f;
            d.postLowpass = 9000.0f;
            d.releaseTau = 0.02f;
            d.fingerNoise = 0.25f;                        // tangent knock
            d.decayKeytrack = 0.85f;
            d.characterDefault = 0.6f;
            d.gain = 0.5f;
            d.spread = 0.2f;
            list.push_back (d);
        }
        {
            // Yamaha DX7 "E.PIANO 1": algorithm 5, three carrier/modulator pairs; the tine ping is a
            // high-ratio modulator with a fast index decay, the body a 1:1 pair.
            InstrumentDef d;
            d.name = "DX7 E.Piano";
            d.category = "Keys";
            d.physics = "Yamaha DX7 FM (algoritms 5): trīs operatoru pāri. 1:1 pāris dod ķermeni, 1:14 pāris ar ātri krītošu indeksu dod stikla „ping”, trešais pāris oktāvu zemāk piepilda basu.";
            d.engine = Engine::fm;
            d.fm[0] = { 1.0f, 1.0f, 0.9f, 1.6f, 0.25f, 0.9f, 2.5f, 0.6f, 0.0f };      // body
            d.fm[1] = { 1.0f, 14.0f, 0.35f, 1.8f, 0.0f, 0.07f, 0.4f, 1.2f, 3.0f };   // tine ping
            d.fm[2] = { 0.5f, 0.5f, 0.45f, 0.9f, 0.1f, 1.2f, 2.0f, 0.3f, -3.0f };    // sub body
            d.numFm = 3;
            d.fmNoise = 0.03f;
            d.decayKeytrack = 0.7f;
            d.releaseTau = 0.09f;
            d.characterDefault = 0.5f;
            d.motion = Motion::stereoTremolo;
            d.motionRate = 4.5f;
            d.motionDefault = 0.2f;
            d.gain = 0.45f;
            d.spread = 0.5f;
            list.push_back (d);
        }

        //==================================================================== MALLETS
        {
            // bars tuned 1:4:10, aluminium, T60 6-30 s, resonator tubes, motor valves at the top
            InstrumentDef d;
            d.name = "Vibraphone";
            d.category = "Mallets";
            d.physics = "Alumīnija stieņi noskaņoti 1 : 4 : 10, skan 6–30 s (patents). Rezonatora caurule pastiprina pamattoni, motora vārsti dod tremolo.";
            d.engine = Engine::modal;
            addMode (d, 1.0f, 1.0f, 7.0f, 0.0f);
            addMode (d, 4.0f, 0.22f, 1.2f, 0.5f);
            addMode (d, 10.0f, 0.06f, 0.25f, 0.9f);
            addMode (d, 1.002f, 0.2f, 4.0f, 0.0f);        // torsional / tube coupling: slow shimmer
            d.decayKeytrack = 0.72f;
            d.releaseTau = 0.2f;                          // damper bar
            d.characterDefault = 0.45f;
            d.motion = Motion::vibesMotor;
            d.motionRate = 4.5f;
            d.motionDefault = 0.4f;
            d.noiseAmount = 0.05f;
            d.noiseFreq = 2500.0f;
            d.noiseDecay = 0.004f;
            d.gain = 0.3f;
            d.spread = 0.45f;
            list.push_back (d);
        }
        {
            // rosewood bars 1:3.92:9.24 (euphonics measurement), 2 s at the bottom down to 20 ms at C7
            InstrumentDef d;
            d.name = "Marimba";
            d.category = "Mallets";
            d.physics = "Rožkoka stieņi, mērīti 1 : 3,92 : 9,24. Koks izdziest ātri (2 s basā, ~20 ms augšā); rezonatori ceļ pamattoni; mīksta vāle = maz virstoņu.";
            d.engine = Engine::modal;
            addMode (d, 1.0f, 1.0f, 0.9f, 0.0f);
            addMode (d, 3.92f, 0.32f, 0.2f, 0.5f);
            addMode (d, 9.24f, 0.1f, 0.05f, 0.9f);
            d.decayKeytrack = 0.58f;
            d.releaseTau = 1.0f;
            d.characterDefault = 0.4f;
            d.noiseAmount = 0.15f;
            d.noiseFreq = 1200.0f;
            d.noiseDecay = 0.005f;
            d.gain = 0.4f;
            d.spread = 0.45f;
            list.push_back (d);
        }
        {
            // xylophone bars tuned 1:3 (quint tuning), 1:3:6.16 measured, very short, hard mallets
            InstrumentDef d;
            d.name = "Xylophone";
            d.category = "Mallets";
            d.physics = "Koka stieņi ar kvintas skaņojumu 1 : 3 : 6,16. Ļoti īsa skaņa un cieta vāle: spilgts, sauss klikšķis ar skaidru augstumu.";
            d.engine = Engine::modal;
            addMode (d, 1.0f, 1.0f, 0.35f, 0.0f);
            addMode (d, 3.0f, 0.5f, 0.12f, 0.3f);
            addMode (d, 6.16f, 0.2f, 0.04f, 0.7f);
            addMode (d, 10.29f, 0.08f, 0.02f, 0.9f);
            d.decayKeytrack = 0.6f;
            d.releaseTau = 1.0f;
            d.characterDefault = 0.75f;
            d.noiseAmount = 0.3f;
            d.noiseFreq = 3000.0f;
            d.noiseDecay = 0.003f;
            d.gain = 0.55f;
            d.spread = 0.45f;
            list.push_back (d);
        }
        {
            // untuned steel bars: free-free ratios 1:2.76:5.40:8.93 (+ torsional), long ring
            InstrumentDef d;
            d.name = "Glockenspiel";
            d.category = "Mallets";
            d.physics = "Taisni tērauda stieņi bez izgrebuma: brīvas sijas modas 1 : 2,76 : 5,40 : 8,93 plus torsijas moda starp tām. Tāpēc zvaniņa skaņa, ilgs ring.";
            d.engine = Engine::modal;
            addMode (d, 1.0f, 1.0f, 3.5f, 0.0f);
            addMode (d, 2.756f, 0.5f, 1.4f, 0.3f);
            addMode (d, 4.1f, 0.12f, 0.8f, 0.5f);         // torsional
            addMode (d, 5.404f, 0.3f, 0.6f, 0.6f);
            addMode (d, 8.933f, 0.12f, 0.3f, 0.8f);
            addMode (d, 13.34f, 0.05f, 0.15f, 1.0f);
            d.decayKeytrack = 0.78f;
            d.releaseTau = 2.0f;
            d.characterDefault = 0.6f;
            d.noiseAmount = 0.04f;
            d.noiseFreq = 6000.0f;
            d.noiseDecay = 0.002f;
            d.gain = 0.22f;
            d.spread = 0.5f;
            list.push_back (d);
        }
        {
            // steel plates struck by felt hammers over wooden resonator boxes: glockenspiel partials, fewer highs
            InstrumentDef d;
            d.name = "Celesta";
            d.category = "Mallets";
            d.physics = "Tērauda plāksnes, ko sit filca āmuri (kā klavierēm) virs koka rezonatoru kastēm. Zvaniņa modas, bet filcs noņem augšas un koks pieliek siltumu.";
            d.engine = Engine::modal;
            addMode (d, 1.0f, 1.0f, 2.2f, 0.0f);
            addMode (d, 2.76f, 0.3f, 0.9f, 0.5f);
            addMode (d, 5.4f, 0.1f, 0.35f, 0.8f);
            d.decayKeytrack = 0.72f;
            d.releaseTau = 0.15f;                         // has dampers and a sustain pedal
            d.characterDefault = 0.35f;
            d.noiseAmount = 0.12f;
            d.noiseFreq = 500.0f;
            d.noiseDecay = 0.008f;
            setBody (d, 0, 420.0f, 2.5f, 0.25f);
            d.lowpass = 8000.0f;
            d.gain = 0.22f;
            d.spread = 0.4f;
            list.push_back (d);
        }
        {
            InstrumentDef d;
            d.name = "Kalimba";
            d.category = "Mallets";
            d.physics = "Metāla mēlītes (konsoles sijas, ~1 : 6 : 16) ar īkšķa piesitienu uz koka kastes ar Helmholca rezonansi. Augstās modas izdziest 30 ms laikā, paliek apaļš tonis.";
            d.engine = Engine::modal;
            addMode (d, 1.0f, 1.0f, 1.3f, 0.0f);
            addMode (d, 5.9f, 0.3f, 0.03f, 0.5f);
            addMode (d, 16.2f, 0.08f, 0.012f, 0.9f);
            addMode (d, 1.004f, 0.15f, 0.9f, 0.0f);       // neighbouring tines sympathetic hum
            d.decayKeytrack = 0.72f;
            d.releaseTau = 0.5f;
            d.characterDefault = 0.5f;
            d.noiseAmount = 0.12f;
            d.noiseFreq = 900.0f;
            d.noiseDecay = 0.006f;
            setBody (d, 0, 240.0f, 3.0f, 0.35f);          // box Helmholtz
            d.gain = 0.22f;
            d.spread = 0.3f;
            list.push_back (d);
        }

        //==================================================================== ORGAN
        {
            InstrumentDef d;
            d.name = "Hammond B3";
            d.category = "Organ";
            d.physics = "91 tonewheel: gandrīz tīri sinusi 9 pēdu garumos (16′…1′), 9 kontakti ar klikšķi, percussion uz frāzes pirmo noti, Leslie (rags 400/48, rotors 340/40 apgr./min).";
            d.engine = Engine::organ;
            setDrawbars (d, "888000000");
            d.organWave = Wave::sine;
            d.characterDefault = 0.6f;
            d.keyClick = 0.6f;
            d.percussionDecay = 0.25f;                    // "fast"
            d.motion = Motion::leslie;
            d.motionDefault = 0.35f;
            d.releaseTau = 0.012f;
            d.gain = 0.2f;
            d.spread = 0.0f;
            list.push_back (d);
        }
        {
            // transistor combo organ: divide-down oscillators give near-square/saw tones, no click, vibrato
            InstrumentDef d;
            d.name = "Vox Continental";
            d.category = "Organ";
            d.physics = "Tranzistoru kombo ērģeles: dalītāju oscilatori dod zāģa/kvadrāta viļņus, nevis sinusus. Bez klikšķa un percussion, bet ar elektronisku vibrato ~6,5 Hz.";
            d.engine = Engine::organ;
            setDrawbars (d, "806800000");
            d.organWave = Wave::saw;
            d.organWaveMix = 0.8f;
            d.characterDefault = 0.0f;
            d.keyClick = 0.0f;
            d.motion = Motion::vibrato;
            d.motionRate = 6.5f;
            d.motionDefault = 0.3f;
            d.releaseTau = 0.02f;
            d.organAttack = 0.004f;
            d.lowpass = 4500.0f;
            d.gain = 0.45f;
            d.spread = 0.0f;
            list.push_back (d);
        }
        {
            // flue pipes: principal chorus, speech transient ("chiff"), each pipe slightly out of tune
            InstrumentDef d;
            d.name = "Pipe Organ";
            d.category = "Organ";
            d.physics = "Flautas stabules (principal 8′+4′+2′ un mixture): katra stabule nedaudz citā skaņojumā, 30–60 ms „chiff” trokšņa transients iestājoties, lēna atlaišana baznīcas telpā.";
            d.engine = Engine::organ;
            setDrawbars (d, "008640320");
            d.organWave = Wave::sine;
            d.characterDefault = 0.0f;
            d.keyClick = 0.0f;
            d.chiff = 0.5f;
            d.pipeDrift = 6.0f;
            d.organAttack = 0.03f;
            d.releaseTau = 0.12f;
            d.gain = 0.2f;
            d.spread = 0.3f;
            list.push_back (d);
        }
        {
            // reed organ: free brass reeds blown by bellows, saw-like spectrum, slow air attack
            InstrumentDef d;
            d.name = "Harmonium";
            d.category = "Organ";
            d.physics = "Plēšu pūstas brīvās mēlītes: zāģveida spektrs, gaisa troksnis, lēns ieskaņojums ~60 ms un neliela augstuma nestabilitāte, kad plēšas elpo.";
            d.engine = Engine::analog;
            d.osc1 = Wave::saw;
            d.osc2 = Wave::saw;
            d.osc2Level = 0.6f;
            d.osc2Cents = 4.0f;
            d.cutoff = 2400.0f;
            d.keytrack = 0.4f;
            d.resonance = 0.1f;
            d.noiseLevel = 0.03f;
            d.attack = 0.06f; d.decay = 0.3f; d.sustain = 1.0f; d.release = 0.12f;
            d.motion = Motion::vibrato;
            d.motionRate = 4.0f;
            d.motionDefault = 0.08f;                      // bellows unsteadiness
            d.vibratoDelay = 0.0f;
            d.characterDefault = 0.1f;
            d.gain = 0.35f;
            d.spread = 0.2f;
            list.push_back (d);
        }

        //==================================================================== STRINGS & WINDS
        {
            InstrumentDef d;
            d.name = "Jazz Guitar";
            d.category = "Strings & Winds";
            d.physics = "Arch-top ar flatwound stīgām un kakla humbucker (LC rezonanse ~2,3 kHz), toņa pote nogriež augšas. Korpusa rezonanses ~110 un 230 Hz.";
            d.noiseFreq = 1800.0f;
            d.engine = Engine::string;
            d.t60 = 4.0f;
            d.stringDamping = 0.5f;
            d.pluckPos = 0.17f;
            d.pluckWidth = 0.03f;
            d.pluckNoise = 0.25f;
            d.pickupPos = 0.26f;
            d.pickupResHz = 2300.0f;
            d.pickupResQ = 1.6f;
            d.bodyMix = 0.2f;
            setBody (d, 0, 110.0f, 4.0f, 0.7f);
            setBody (d, 1, 230.0f, 4.0f, 0.5f);
            d.postLowpass = 3200.0f;
            d.releaseTau = 0.05f;
            d.decayKeytrack = 0.8f;
            d.motion = Motion::vibrato;
            d.motionRate = 5.0f;
            d.characterDefault = 0.45f;
            d.gain = 0.55f;
            d.spread = 0.25f;
            list.push_back (d);
        }
        {
            InstrumentDef d;
            d.name = "Nylon Guitar";
            d.category = "Strings & Winds";
            d.physics = "Neilona stīgas: lielāka iekšējā berze, tāpēc augšas izdziest ātri; pirksta gaļa raustot. Korpuss ar Helmholca ~100 Hz un galda ~200 Hz rezonansēm.";
            d.noiseFreq = 1500.0f;
            d.engine = Engine::string;
            d.t60 = 3.0f;
            d.stringDamping = 0.5f;
            d.pluckPos = 0.24f;
            d.pluckWidth = 0.05f;
            d.pluckNoise = 0.15f;
            d.bodyMix = 0.45f;
            setBody (d, 0, 100.0f, 5.0f, 1.0f);
            setBody (d, 1, 205.0f, 5.0f, 0.7f);
            setBody (d, 2, 420.0f, 4.0f, 0.4f);
            d.postLowpass = 6000.0f;
            d.releaseTau = 0.06f;
            d.decayKeytrack = 0.78f;
            d.characterDefault = 0.4f;
            d.gain = 0.55f;
            d.spread = 0.3f;
            list.push_back (d);
        }
        {
            InstrumentDef d;
            d.name = "Harp";
            d.category = "Strings & Winds";
            d.physics = "Garas zarnu/neilona stīgas, rautas tuvu vidum ar pirksta gaļu: tumšs, apaļš tonis bez spilgtuma, ilgs ring (nav slāpētāju) un liels skaņu dēlis.";
            d.noiseFreq = 900.0f;
            d.engine = Engine::string;
            d.t60 = 6.0f;
            d.stringDamping = 0.55f;
            d.pluckPos = 0.4f;
            d.pluckWidth = 0.08f;
            d.pluckNoise = 0.05f;
            d.bodyMix = 0.35f;
            setBody (d, 0, 150.0f, 3.0f, 0.8f);
            setBody (d, 1, 320.0f, 3.0f, 0.5f);
            d.postLowpass = 5000.0f;
            d.releaseTau = 1.5f;                          // no dampers: notes ring over each other
            d.decayKeytrack = 0.7f;
            d.characterDefault = 0.35f;
            d.gain = 0.6f;
            d.spread = 0.6f;
            list.push_back (d);
        }
        {
            InstrumentDef d;
            d.name = "Soul Strings";
            d.category = "Strings & Winds";
            d.physics = "Stīgu sekcija: daudzi nedaudz atšķirīgi skaņojumi un lociņu fāzes (ensemble), lēns ieskaņojums ~300 ms, vibrato, kas iestājas pēc mirkļa.";
            d.engine = Engine::analog;
            d.osc1 = Wave::saw;
            d.osc2 = Wave::saw;
            d.osc2Level = 0.8f;
            d.osc2Cents = 7.0f;
            d.unison = 3;
            d.unisonDetune = 14.0f;
            d.cutoff = 2600.0f;
            d.keytrack = 0.4f;
            d.resonance = 0.1f;
            d.envAmount = 0.5f;
            d.filterDecay = 1.2f;
            d.attack = 0.3f; d.decay = 1.0f; d.sustain = 0.9f; d.release = 0.7f;
            d.motion = Motion::vibrato;
            d.motionRate = 5.2f;
            d.motionDefault = 0.25f;
            d.vibratoDelay = 0.5f;
            d.characterDefault = 0.15f;
            d.gain = 0.3f;
            d.spread = 0.7f;
            list.push_back (d);
        }
        {
            // low register: the 2nd harmonic rivals the fundamental; noise floor ~-60 dB; breathy onset
            InstrumentDef d;
            d.name = "Flute";
            d.category = "Strings & Winds";
            d.physics = "Gaisa strūkla pret asmeni: zemajā reģistrā 2. harmonika gandrīz tikpat stipra kā pamattonis, elpas troksnis filtrēts ap toni, „chiff” sākumā, vibrato ~5 Hz.";
            d.engine = Engine::analog;
            d.osc1 = Wave::sine;
            d.osc2 = Wave::sine;
            d.osc2Level = 0.45f;
            d.osc2Semis = 12.0f;
            d.breath = 0.35f;
            d.cutoff = 5000.0f;
            d.keytrack = 0.3f;
            d.resonance = 0.05f;
            d.attack = 0.045f; d.decay = 0.3f; d.sustain = 0.85f; d.release = 0.12f;
            d.motion = Motion::vibrato;
            d.motionRate = 5.0f;
            d.motionDefault = 0.35f;
            d.vibratoDelay = 0.35f;
            d.characterDefault = 0.05f;
            d.gain = 0.4f;
            d.spread = 0.2f;
            list.push_back (d);
        }
        {
            // harmon mute: strong formant 1.5-2 kHz (tonebase), lip "blip" at the onset, breath
            InstrumentDef d;
            d.name = "Muted Trumpet";
            d.category = "Strings & Winds";
            d.physics = "Lūpu svārstības dod zāģveida spektru; Harmon surdīne izgriež basu un uzliek formantu ap 1,5–2 kHz. Lūpu „blip” (+1 pustonis 30 ms) ieskaņojoties, vibrato.";
            d.engine = Engine::analog;
            d.osc1 = Wave::saw;
            d.osc2 = Wave::square;
            d.osc2Level = 0.3f;
            d.cutoff = 1800.0f;
            d.keytrack = 0.5f;
            d.resonance = 0.15f;
            d.envAmount = 1.2f;
            d.filterDecay = 0.12f;
            d.breath = 0.08f;
            d.attack = 0.025f; d.decay = 0.25f; d.sustain = 0.85f; d.release = 0.08f;
            d.pitchEnvSemis = 1.0f;
            d.pitchEnvDecay = 0.03f;
            d.formant = { 1750.0f, 2.5f, 1.4f };
            d.motion = Motion::vibrato;
            d.motionRate = 5.5f;
            d.motionDefault = 0.3f;
            d.vibratoDelay = 0.4f;
            d.characterDefault = 0.15f;
            d.mono = true;
            d.glide = 0.015f;
            d.gain = 0.6f;
            d.spread = 0.0f;
            list.push_back (d);
        }

        //==================================================================== BASS
        {
            // A0 ~60 Hz, B1 ~100-125 Hz, bridge ~300 Hz (Zevin, floor-contact study); short pizz sustain
            InstrumentDef d;
            d.name = "Upright Bass";
            d.category = "Bass";
            d.physics = "Kontrabasa pizzicato: resna stīga rauta ar pirksta sānu (platas, tumšas ierosmes), korpusa rezonanses A0 ~60 Hz, B1 ~110 Hz, tilts ~300 Hz, īss sustain, augstums piesitienā nedaudz krīt.";
            d.isBass = true;
            d.mono = true;
            d.noiseFreq = 140.0f;
            d.engine = Engine::string;
            d.t60 = 2.0f;
            d.stringDamping = 0.5f;
            d.pluckPos = 0.2f;
            d.pluckWidth = 0.14f;
            d.pluckNoise = 0.06f;
            d.pitchDrop = 0.025f;
            d.bodyMix = 0.5f;
            setBody (d, 0, 62.0f, 4.0f, 1.0f);
            setBody (d, 1, 112.0f, 5.0f, 0.8f);
            setBody (d, 2, 300.0f, 6.0f, 0.5f);
            d.fingerNoise = 0.55f;
            d.postLowpass = 2200.0f;
            d.releaseTau = 0.06f;
            d.decayKeytrack = 0.8f;
            d.characterDefault = 0.5f;
            d.gain = 0.42f;
            d.spread = 0.0f;
            list.push_back (d);
        }
        {
            // split-coil pickup ~28 % from the bridge, LC resonance ~3 kHz, tone pot rolled to ~1.5 kHz, flats
            InstrumentDef d;
            d.name = "P-Bass Flats";
            d.category = "Bass";
            d.physics = "Precision bass ar flatwound stīgām: comb filtrs no pickup vietas (28 % no tilta), LC rezonanse ~3 kHz (5–7 H, ~250 pF), toņa pote nogriež ~1,5 kHz. Ilgs, apaļš sustain.";
            d.isBass = true;
            d.mono = true;
            d.noiseFreq = 380.0f;
            d.engine = Engine::string;
            d.t60 = 3.5f;
            d.stringDamping = 0.5f;
            d.pluckPos = 0.22f;
            d.pluckWidth = 0.09f;
            d.pluckNoise = 0.05f;
            d.pickupPos = 0.28f;
            d.pickupResHz = 3000.0f;
            d.pickupResQ = 2.0f;
            d.pitchDrop = 0.008f;
            d.fingerNoise = 0.1f;
            d.postLowpass = 1500.0f;
            d.releaseTau = 0.05f;
            d.decayKeytrack = 0.85f;
            d.characterDefault = 0.5f;
            d.gain = 0.6f;
            d.spread = 0.0f;
            list.push_back (d);
        }
        {
            // fretless Jazz Bass: bridge pickup growl, the "mwah" formant that sweeps with the note, glide
            InstrumentDef d;
            d.name = "Fretless Bass";
            d.category = "Bass";
            d.physics = "Fretless Jazz Bass: stīga guļ uz koka, tāpēc „mwah” – formants, kas slīd lejup pēc piesitiena. Tilta pickup (~12 %) dod growl, roundwound stīgas, slīdēšana starp notīm.";
            d.isBass = true;
            d.mono = true;
            d.glide = 0.03f;
            d.noiseFreq = 500.0f;
            d.engine = Engine::string;
            d.t60 = 3.0f;
            d.stringDamping = 0.35f;
            d.pluckPos = 0.15f;
            d.pluckWidth = 0.06f;
            d.pluckNoise = 0.1f;
            d.pickupPos = 0.12f;
            d.pickupResHz = 3600.0f;
            d.pickupResQ = 1.8f;
            d.mwah = 0.7f;
            d.pitchDrop = 0.006f;
            d.fingerNoise = 0.08f;
            d.postLowpass = 3000.0f;
            d.releaseTau = 0.06f;
            d.decayKeytrack = 0.85f;
            d.motion = Motion::vibrato;
            d.motionRate = 4.5f;
            d.motionDefault = 0.15f;
            d.characterDefault = 0.5f;
            d.gain = 0.8f;
            d.spread = 0.0f;
            list.push_back (d);
        }
        {
            // bottom 32 notes of a Rhodes: long tines deflect far, so the pickup growls at any velocity
            InstrumentDef d;
            d.name = "Rhodes Piano Bass";
            d.category = "Bass";
            d.physics = "Rhodes Piano Bass (The Doors): garās basa tines novirzās tālu, tāpēc pickup lauks kropļo jau vidējā sitienā. Dziļš, apaļš tonis ar growl, īsāks sustain nekā vidū.";
            d.isBass = true;
            d.mono = false;
            d.engine = Engine::modal;
            d.pickup = Pickup::magnetic;
            addMode (d, 1.0f, 1.0f, 2.2f, 0.0f);
            addMode (d, 6.27f, 0.35f, 0.015f, 0.6f);
            addMode (d, 17.55f, 0.1f, 0.006f, 0.9f);
            d.tonebarRatio = 0.65f;
            d.transientMs = 14.0f;
            d.pickupOffset = 0.45f;
            d.deflection = 1.1f;
            d.decayKeytrack = 0.6f;
            d.releaseTau = 0.07f;
            d.characterDefault = 0.6f;
            d.noiseAmount = 0.08f;
            d.noiseFreq = 300.0f;
            d.noiseDecay = 0.008f;
            d.lowpass = 2500.0f;                          // bass amp cabinet
            d.gain = 0.4f;
            d.spread = 0.0f;
            list.push_back (d);
        }
        {
            // DX-style FM bass: 1:1 pair with a fast index drop gives the "pluck", a sub carrier the weight
            InstrumentDef d;
            d.name = "FM Bass";
            d.category = "Bass";
            d.physics = "DX7 tipa FM bass: 1:1 operatoru pāris ar strauji krītošu indeksu dod „knikšķa” piesitienu, 2:1 pāris spožumu, sub nesējs oktāvu zemāk dod svaru. Tīrs, digitāls.";
            d.isBass = true;
            d.mono = true;
            d.glide = 0.02f;
            d.engine = Engine::fm;
            d.fm[0] = { 1.0f, 1.0f, 1.0f, 3.5f, 0.35f, 0.12f, 1.6f, 0.9f, 0.0f };
            d.fm[1] = { 1.0f, 2.0f, 0.4f, 1.5f, 0.0f, 0.06f, 0.5f, 1.0f, 0.0f };
            d.fm[2] = { 0.5f, 0.5f, 0.6f, 0.3f, 0.1f, 0.3f, 1.4f, 0.2f, 0.0f };
            d.numFm = 3;
            d.decayKeytrack = 0.85f;
            d.releaseTau = 0.05f;
            d.characterDefault = 0.5f;
            d.gain = 0.6f;
            d.spread = 0.0f;
            list.push_back (d);
        }
        {
            // Minimoog: saw + square an octave down through the 24 dB ladder, filter envelope snaps
            InstrumentDef d;
            d.name = "Moog Bass";
            d.category = "Bass";
            d.physics = "Minimoog: zāģis + kvadrāts caur 24 dB/okt ladder filtru ar rezonansi; filtra envelope atver un ātri aizver (snap), mikseris pārslogo filtru (saturācija).";
            d.isBass = true;
            d.mono = true;
            d.glide = 0.04f;
            d.engine = Engine::analog;
            d.osc1 = Wave::saw;
            d.osc2 = Wave::square;
            d.osc2Level = 0.7f;
            d.osc2Semis = -12.0f;
            d.osc2Cents = 3.0f;
            d.cutoff = 300.0f;
            d.keytrack = 0.3f;
            d.resonance = 0.35f;
            d.envAmount = 3.0f;
            d.filterDecay = 0.22f;
            d.attack = 0.002f; d.decay = 0.6f; d.sustain = 0.75f; d.release = 0.08f;
            d.drive = 1.7f;
            d.characterDefault = 0.35f;
            d.gain = 0.38f;
            d.spread = 0.0f;
            list.push_back (d);
        }
        {
            // TR-808: bridged-T resonant filter at ~50 Hz (manual 56), 6 ms jump to 130 Hz, 50-800 ms decay,
            // slow downward drift. Here the ring frequency follows the key.
            InstrumentDef d;
            d.name = "808 Sub";
            d.category = "Bass";
            d.physics = "TR-808 bass drum kā bass: rezonējošs bridged-T filtrs (~50 Hz, keytrack), 6 ms augstuma lēciens līdz ~2,6×, izdzišana 50–800 ms, lēns augstuma kritums. Pliks sinuss ar saturāciju.";
            d.isBass = true;
            d.mono = true;
            d.glide = 0.0f;
            d.engine = Engine::analog;
            d.osc1 = Wave::sine;
            d.resonantPitchHz = 50.0f;
            d.pitchEnvSemis = 17.0f;                      // 50 -> 130 Hz
            d.pitchEnvDecay = 0.006f;
            d.cutoff = 2000.0f;
            d.keytrack = 0.0f;
            d.resonance = 0.0f;
            d.attack = 0.0005f; d.decay = 0.9f; d.sustain = 0.0f; d.release = 0.2f;
            d.drive = 2.0f;
            d.characterDefault = 0.2f;
            d.gain = 0.45f;
            d.spread = 0.0f;
            list.push_back (d);
        }
        {
            // Juno-60 bass: pulse + sub square, gentle 24 dB filter, stereo BBD chorus
            InstrumentDef d;
            d.name = "Juno Bass";
            d.category = "Bass";
            d.physics = "Roland Juno: pulsa vilnis + sub kvadrāts oktāvu zemāk, maigs 24 dB filtrs bez lielas rezonanses, un stereo BBD chorus, kas dod platumu. Apaļš, silts 80. gadu bass.";
            d.isBass = true;
            d.mono = true;
            d.glide = 0.0f;
            d.engine = Engine::analog;
            d.osc1 = Wave::square;
            d.pulseWidth = 0.35f;
            d.subLevel = 0.6f;
            d.cutoff = 420.0f;
            d.keytrack = 0.4f;
            d.resonance = 0.1f;
            d.envAmount = 1.3f;
            d.filterDecay = 0.3f;
            d.attack = 0.003f; d.decay = 0.9f; d.sustain = 0.8f; d.release = 0.1f;
            d.unison = 1;
            d.unisonDetune = 0.0f;                        // stands in for the chorus
            d.characterDefault = 0.1f;
            d.gain = 0.45f;
            d.spread = 0.0f;
                d.motion = Motion::chorus;
                d.motionRate = 0.6f;
                d.motionDefault = 0.7f;
            list.push_back (d);
        }

        return list;
    }
}

const std::vector<InstrumentDef>& instruments()
{
    static const std::vector<InstrumentDef> list = build();
    return list;
}

} // namespace bk
