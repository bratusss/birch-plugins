# Birch Keys: izpēte un dizains (2. versija)

Šis dokuments apraksta, **kā fiziski rodas** katra instrumenta skaņa, ar **izmērītiem skaitļiem** no literatūras, un **kā Birch Keys to sintezē**. Visi 31 instrumenti ir sintezēti (fizikālā/modālā modelēšana, viļņvadi, aditīvā, analogā un FM sintēze), bez ierakstītiem sempliem. Katrs instruments ir **cits mehānisms**, nevis viens dzinējs ar citiem efektiem.

Dzinēji: **Modal** (tine, mēlīte, stienis, stīga kā virstoņu kopa), **String** (digitālais viļņvads), **Organ** (aditīvs ar ģeneratoriem), **Analog** (oscilatori + ladder filtrs), **FM** (DX7 tipa operatoru pāri).

---

## 0. Boombap konteksts

90. gadu boombap bīti ir sempli no 60.–70. gadu **džeza, soul un funk** ierakstiem: Rhodes, Wurlitzer, klavieres, vibrafons, Hammond, kontrabass, P-bass ar flatwound stīgām, Moog bass, pūšamie ([output.com](https://output.com/blog/best-boom-bap-vst-plugins)). Ceļš līdz bītam: **vinils** (troksnis, wow/flutter), **semplers** E-mu SP-1200 ar **26,04 kHz un 12 bitu** formātu un **SSM2044 4-polu low-pass** filtriem ([Wikipedia](https://en.wikipedia.org/wiki/E-mu_SP-1200), [Rossum](https://www.rossum-electro.com/pages/education-sp-1200)), un **„bass no sempla”** triks (Marley Marl, Masta Ace „Music Man”, [MPC Forums](https://www.mpc-forums.com/viewtopic.php?p=131851)). Birch Keys to atveido ar semplera ķēdi pēc instrumenta: Grit (Era '88/'94/'98), SP Filter + Resonance, Warmth, Wobble, Dust, Room.

---

## KEYS

### Rhodes Mk I (Modal + magnētiskais pickup)

**Mērījumi.** ISMA 2014 (Münster & Pfeifle, ātrgaitas kamera 38 000 kadri/s, piezo):
- Āmura kontakts ar tine ilgst **6,42 ms**.
- Pēc **10–14 ms** tine svārstās **kā tīrs sinuss** bez virstoņiem. Augstākas modas parādās tikai transientā.
- **Tonebar** eigenfrekvence ir **vairākus simtus līdz >1400 centu zem nots** (piem., tonebar 105 Hz zem C 263 Hz; 138 Hz zem G 393 Hz). Tine „paverdzina” tonebar, kas svārstās tine frekvencē fāzē vai pretfāzē. Tonebar skan tikai transientā un dod „glockenspiel” piesitienu.
- **Growl** rodas no **pickup lauka nelinearitātes**, visvairāk basā, kur tines novirzās vairāk.
- DAGA 2017 (Pfeifle & Münster): magnētiskā lauka sadalījums pie magnēta gala ir **zvana līkne**; tine centrā pret magnētu dod **dubultu frekvenci** (oktāvu), nobīdīta dod asimetrisku viļņu ar vairāk harmonikām.

**Sintēze.** Tine = sinuss ar T60 ~7 s (vidū). Modas 6,27× un 17,55× (konsoles sija) ar laika konstanti **≤13 ms**; tonebar moda 0,4× f0 (keytrack) tikai transientā. Pickup: plūsma **Φ(x) = exp(−(x−p)²)**, izeja **dΦ/dt**. `PICKUP` poga = p (0 = centrā → oktāva, 1 = malā). Novirze ∝ velocity × (261/f0)^0,45, tāpēc bass growl rodas dabiski. Pie p ≈ 0,55 otrā harmonika matemātiski izzūd (Gausa līknes otrais atvasinājums maina zīmi). Suitcase stereo tremolo 4,2 Hz, skaļruņa low-pass 7 kHz.

### Wurlitzer 200A (Modal + elektrostatiskais pickup)

**Mērījumi.** DAGA 2017: mēlīte svārstās gandrīz sinusoidāli; **kapacitātes pickup** (mēlīte = kondensatora elektrods, C ∝ 1/attālums) ir asimetrisks, tāpēc **stiprāks sitiens = bagātākas harmonikas**, **ne-eksponenciāla izdzišana un augsto virstoņu pukstēšana**. OpenWurli: kapacitatīvā 1/(1−y) nelinearitāte ir galvenais „bark” avots, pēc tam RC high-pass; tremolo **~5,6 Hz** (Twin-T oscilators).

**Sintēze.** Mēlīte sinuss T60 ~3,5 s + pukstošs partneris (1,0012×) + īsas modas 6,1× un 15,8× (lodes masa galā pazemina 6,27). Pickup: **i = dC/dt, C = x/(1−x)**, RC high-pass 150 Hz, mazo skaļruņu low-pass 5,5 kHz. Tremolo 5,6 Hz.

### Pianet (Modal + elektrostatiskais pickup)
Hohner Pianet mēlīti **atrauj lipīgs spilventiņš**, nevis sit āmurs: nav trokšņa, mīksts 6 ms ieskaņojums, maza novirze (gandrīz tīrs tonis), īss sustain (~2 s).

### Grand Piano un Upright Piano (Modal, stīgu virstoņi)

**Fizika.** Stīgas stingums: **fₙ = n·f₀·√(1+Bn²)** ([Smith, DSPRelated](https://www.dsprelated.com/freebooks/pasp/Piano_Synthesis.html)); B ≈ 3·10⁻⁴ (koncertflīģelis, C4) līdz ~10⁻³ (īsas pianīno stīgas). **Divpakāpju izdzišana** no 2–3 nedaudz nesaskaņotām unisona stīgām ([coupled strings](https://image-ppubs.uspto.gov/dirsearch-public/print/downloadPdf/5777255), [Cheng et al. ICASSP 2015](https://webspace.eecs.qmul.ac.uk/s.e.dixon/pub/2015/ChengDixonMauch-ICASSP2015-Decay.pdf)). Āmura **kontakta laiks 1–4 ms**, īsāks stiprāk sitot un augšā; garāks kontakts = zemāks spektra centroīds ([IRCAM SMC04](http://recherche.ircam.fr/equipes/repmus/SMC04/scm04actes/P46.pdf)); basa āmuram griezuma frekvence ceļas ~1800 Hz, ātrumam augot no 1 līdz 3,3 m/s ([patents](https://image-ppubs.uspto.gov/dirsearch-public/print/downloadPdf/5537862)).

**Sintēze.** 24 virstoņi × 2 stīgas (ātrā „prompt” un lēnā „aftersound”). Āmura filtrs: griezums = (300 + 3f₀) × (0,6 + 2,4·v^1,3) Hz. Sitiena vieta 0,118 (flīģelis) / 0,125 (pianīno). **Grand**: B = 3,2·10⁻⁴, 0,9 centu detune, T60 ~9 s pie C4, skaņas galda rezonanses 95/180 Hz. **Upright**: B = 9·10⁻⁴, 3,5 centu detune (honky), T60 ~5 s, kastes rezonanses 210/380 Hz, stiprāks thump, low-pass 9 kHz.

### Clavinet D6 (String + pickup)
EURASIP 2013 ([Gabrielli et al.](https://link.springer.com/article/10.1186/1687-6180-2013-103)): **T60 līdz 20 s**, tangents dod trīsstūrveida impulsus, **divi pickup** (tilta spožs, kakla silts) ar comb filtru un fāzes dzēšanu, **klikšķis atlaižot** (tangents atsitas pret balstu). Sintēze: viļņvads T60 12 s, tangents 7 % no gala, pickup 14 %, **LC rezonanse 4,2 kHz (Q 2,2)**, klikšķis atlaižot.

### DX7 E.Piano (FM)
Yamaha DX7 „E.PIANO 1”: **algoritms 5**, trīs nesēja/modulatora pāri ([One Two Testing 1983](https://www.muzines.co.uk/articles/yamaha-dx7/4733), [E&MM 1984](https://www.muzines.co.uk/articles/understanding-the-dx7/1476)). Tine „ping” ir augsta attiecība (10–14) ar ātri krītošu indeksu un lielu velocity jutību. Sintēze: pāris 1:1 (ķermenis, indekss 1,6→0,25, 0,9 s), pāris 1:14 (ping, indekss 1,8→0, 70 ms), pāris 0,5:0,5 (sub).

## MALLETS

Stieņu modu attiecības (euphonics.org mērījumi, [Rossing](https://www.researchgate.net/publication/243492369_Basic_physics_of_xylophone_and_marimba_bars), [patenti](https://image-ppubs.uspto.gov/dirsearch-public/print/downloadPdf/5902945)):

| Instruments | Modas | Materiāls, izdzišana | Sintēzē |
|---|---|---|---|
| **Vibraphone** | 1 : 4 : 10 (izgrebts) | alumīnijs, **6–30 s** ([patents](https://image-ppubs.uspto.gov/dirsearch-public/print/downloadPdf/4411187)) | T60 ~16 s vidū, rezonatora caurule, motors 4,5 Hz |
| **Marimba** | 1 : 3,92 : 9,24 (mērīts) | rožkoks, **2 s → 20 ms** pa 4 oktāvām | T60 2 s basā, strauji īsāk augšā |
| **Xylophone** | 1 : 3 : 6,16 : 10,29 (kvintas skaņojums, mērīts) | ciets koks, ļoti īss | T60 0,8 s, cieta vāle |
| **Glockenspiel** | 1 : 2,756 : 5,404 : 8,933 (brīva sija) + torsijas moda | tērauds, ilgs | 6 modas, T60 ~8 s |
| **Celesta** | kā glockenspiel (plāksnes) | filca āmuri + koka kastes | mazāk augšu, rezonanse 420 Hz, slāpētāji |
| **Kalimba** | ~1 : 5,9 : 16 (konsoles mēlīte) | metāls uz koka kastes | augstās modas 30 ms, Helmholca kaste 240 Hz |

Vāles cietība (`CHARACTER`): cik daudz velocity vajag, lai ierosinātu augstākās modas.

## ORGAN

- **Hammond B3**: tonewheel ģeneratori ir gandrīz sinusi ([Wikipedia](https://en.wikipedia.org/wiki/Hammond_organ)); 9 drawbar pēdas 16′ 5⅓′ 8′ 4′ 2⅔′ 2′ 1⅗′ 1⅓′ 1′ = attiecības 0,5 1,5 1 2 3 4 5 6 8; **9 kontakti neaizveras vienlaicīgi → key click** ([LAU](https://lists.linuxaudio.org/archives/linux-audio-user/2009-May/060092.html)); percussion (2. vai 3. harmonika, fast/slow) tikai frāzes pirmajai notij; augšējie tonewheel „salokās” oktāvu zemāk virs ~5,9 kHz. **Leslie**: rags ~400/48, rotors ~340/40 apgr./min, rags paātrinās ~1 s, rotors vairākas sekundes ([Wikipedia](https://en.wikipedia.org/wiki/Leslie_speaker), [Logic manuālis](https://help.apple.com/logicpro/mac/9.1.6/en/logicpro/effects/chapter_9_section_8.html)).
- **Vox Continental**: tranzistoru **dalītāju oscilatori** dod zāģa/kvadrāta viļņus, nevis sinusus; nav klikšķa; elektronisks vibrato ~6,5 Hz. Sintēzē katrs „drawbar” ir 80 % zāģis.
- **Pipe Organ**: flautas stabules ar **„chiff”** (30 ms trokšņa transients ap 2.–3. virstoni), katra stabule ±6 centi, 30 ms ieskaņojums, lēna atlaišana.
- **Harmonium**: brīvās mēlītes ar zāģveida spektru, plēšu gaisa troksnis, 60 ms ieskaņojums, neliela augstuma šūpošanās.

## STRINGS & WINDS

- **Jazz Guitar**: arch-top, flatwound, kakla humbucker **LC rezonanse ~2,3 kHz**, toņa pote; korpuss 110/230 Hz.
- **Nylon Guitar**: neilons = lielāka iekšējā berze (ātrāk zūd augšas); Helmholca ~100 Hz, galds ~205 Hz.
- **Harp**: garas stīgas rautas pie vidus (0,4), tumšs tonis, bez slāpētājiem (release 1,5 s).
- **Soul Strings**: ensemble (3 balsis ±14 centi), 300 ms attack, vibrato pēc 0,5 s.
- **Flute**: zemajā reģistrā **2. harmonika gandrīz tikpat stipra kā pamattonis** ([Tufts](https://mma.pages.tufts.edu/musiceng/research/flutepage/titlepage.htm)); trokšņa grīda ~−60 dB ([Sevilla 2002](https://documentacion.sea-acustica.es/storage/publicaciones/Sevilla02_mus01004.pdf)). Sintēzē: sinuss + 45 % oktāva, elpas troksnis filtrēts ap toni, chiff, vibrato 5 Hz.
- **Muted Trumpet**: Harmon surdīne izstaro galvenokārt **1,5–2 kHz** ([tonebase](https://trumpet.tonebase.co/blog/what-is-a-harmon-mute-on-the-trumpet)); sintēzē formants 1750 Hz (Q 2,5), lūpu „blip” +1 pustonis 30 ms, mono ar glide.

## BASS (8 atšķirīgi mehānismi)

| Bass | Mehānisms | Mērījumi/avoti | Atšķirības sintēzē |
|---|---|---|---|
| **Upright Bass** | viļņvads + korpuss | Helmholca **A0 ~60 Hz**, **B1 ~100–125 Hz**, tilta rezonanse **~300 Hz** ([Zevin](https://courses.physics.illinois.edu/phys406/sp2017/Student_Projects/Spring12/Michael_Zevin_P406_Project_Report_Sp12.pdf), [floor study](https://www.researchgate.net/publication/268040106_An_in-depth_analysis_of_the_double_bass_to_stage_floor_contact)); pizz īsāks par elektrobasu | plats pirksts (14 % stīgas), T60 ~2 s, body 62/112/300 Hz, thump 140 Hz, pitch drop 2,5 % |
| **P-Bass Flats** | viļņvads + magnētiskais pickup | pickup **5–7 H**, ~250 pF → **LC rezonanse ~2,8–3 kHz** ([talkbass](https://www.talkbass.com/threads/pickup-resonant-frequency.1447635/)); flats = silts, izteikts pamattonis ([Guitar World](https://www.guitarworld.com/features/the-best-flatwound-bass-guitar-strings-how-to-choose-the-right-flatwounds-for-you)) | pickup comb 28 %, rezonanse 3 kHz Q 2, toņa pote 1,5 kHz, T60 3,5 s |
| **Fretless Bass** | viļņvads + „mwah” | stīga brīvi kustas uz koka → „mwah”; tilta pickup un raustīšana pie kakla gala pastiprina ([talkbass](https://www.talkbass.com/threads/what-makes-the-mwah-sound-on-a-fretless.1115244/)) | formants slīd 1,4 kHz → 450 Hz 250 ms, tilta pickup 12 %, glide |
| **Rhodes Piano Bass** | Rhodes tine basa reģistrā | 32 apakšējās notis, Doors ([Wikipedia](https://en.wikipedia.org/wiki/Rhodes_piano)); garās tines novirzās vairāk → growl | tas pats pickup modelis, novirze 1,1×, tonebar 0,65×, cabinet 2,5 kHz |
| **FM Bass** | 2-op FM pāri | DX tipa bass: 1:1 pāris ar ātri krītošu indeksu | indekss 3,5→0,35 120 ms, sub nesējs 0,5× |
| **Moog Bass** | zāģis + kvadrāts, ladder | 24 dB/okt ladder ar rezonansi, filtra envelope „snap”, mikseris pārslogo ([SOS](https://www.soundonsound.com/reviews/moog-ladder-media), [MusicRadar](https://www.musicradar.com/how-to/4-legendary-bass-synths-find-out-how-they-work-and-get-their-sound)) | cutoff 300 Hz + 3 okt env 220 ms, tanh katrā posmā |
| **808 Sub** | rezonējošs filtrs | bridged-T **~49–56 Hz**, **6 ms lēciens līdz 130 Hz**, izdzišana **50–800 ms**, lēns kritums ([Baratatronix](https://www.baratatronix.com/blog/808-bd-synthesis), [arXiv](https://arxiv.org/pdf/2502.07524)) | sinuss, +17 pustoņu env 6 ms, sag −0,2 pustoņi, sustain 0 |
| **Juno Bass** | pulss + sub + BBD chorus | Juno-60: pulsa vilnis, sub oktāvu zemāk, chorus | pulss 35 %, sub 60 %, cutoff 420 Hz, chorus 0,6 Hz ±2,5 ms |

---

## Pārbaude (tools/analyze2.py)

Katram instrumentam no testa ierakstiem mēra: RMS, pīķi, augstuma kļūdu centos, H2/H1 un H3/H1 dB, spektra centroīdu 10 ms un 300 ms pēc piesitiena, kritumu pēc 0,5 s. Rezultāti rāda atšķirīgus „pirkstu nospiedumus”: piemēram, Rhodes transients 3,9 kHz → ķermenis 820 Hz ar H2 −9 dB; Wurlitzer 1,4 kHz → 660 Hz ar H2 −13 dB; Upright Bass centroīds 130 Hz un −17 dB pēc 0,5 s; P-Bass −10 dB; Fretless H2 −1,5 dB (growl); 808 tīrs sinuss (H2 < −90 dB) ar −7 centu kritumu.
