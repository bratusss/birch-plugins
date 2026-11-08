# Birch Keys: boom bap instruments

VST3 instruments (JUCE 8, C++) ar **31 fizikāli modelētu instrumentu** 5 kategorijās. Nav sempļu: katrs instruments ir cits mehānisms (tine + pickup, mēlīte + kondensators, stīgu virstoņi, viļņvads, tonewheel, oscilatori + ladder, FM operatori). Fizika un avoti: [RESEARCH.md](RESEARCH.md).

| Kategorija | Instrumenti |
|---|---|
| **KEYS** | Rhodes Mk I, Wurlitzer 200A, Pianet, Grand Piano, Upright Piano, Clavinet D6, DX7 E.Piano |
| **MALLETS** | Vibraphone, Marimba, Xylophone, Glockenspiel, Celesta, Kalimba |
| **ORGAN** | Hammond B3, Vox Continental, Pipe Organ, Harmonium |
| **STRINGS** | Jazz Guitar, Nylon Guitar, Harp, Soul Strings, Flute, Muted Trumpet |
| **BASS** | Upright Bass, P-Bass Flats, Fretless Bass, Rhodes Piano Bass, FM Bass, Moog Bass, 808 Sub, Juno Bass |

Spēlē viens instruments vienlaicīgi. Basi ir mono (kā basists), ar glide tur, kur tas dabiski (fretless, Moog).

## Pielāgošana

- **SOUND** (visiem): Level, Tone, Decay, Release, Character, Motion, Rate, Velocity, Width, Attack, Glide, Tune, Mono, Octave, Bend. Character/Motion nosaukumi mainās pēc instrumenta: BARK, HARDNESS, PLUCK, PERC, RESO, BRIGHT / TREMOLO, MOTOR, LESLIE, VIBRATO, CHORUS.
- **ENGINE** (pēc dzinēja): Tine & Pickup (Bell, Thump, **Pickup** nobīde), Strings & Hammer (Hammer, Thump, Detune, Stretch), Bars & Mallet, String & Body (Position, Body, Finger, Pickup, Pitch Drop, **Mwah**), Drawbars (9 drawbars, Percussion 2nd/3rd, Perc Decay, Click, **Chiff**), Synth (2 oscilatori, Sub, Noise, Breath, Cutoff, Env, F.Decay, Sustain, Drive, Unison), FM Operators (3 pāri × Ratio, Index, Index Decay).
- **SAMPLER & VINYL**: Era ('88 / '94 SP-1200 / '98), Grit, SP Filter, Reso, Warmth, Wobble, Dust, Room, Volume.
- **Preseti** ir instrumentu *iestatījumi* (pickup vieta, āmuri, vāles, reģistrācijas, stīgas, oscilatori), nevis efekti. Katra instrumenta pirmais preset ir instruments kā izstrādāts. **SAVE** saglabā savējos (`~/.config/Birch Keys/Presets`, Windows `%APPDATA%\Birch Keys\Presets`).

## Būvēšana un testēšana (Linux)

```bash
./build.sh
```

```bash
"./build/BirchKeys_artefacts/Release/Standalone/Birch Keys"
```

Spēlē ar peli, datora klaviatūru (A W S E D F T G Y H U J K) vai MIDI. **Options** maina audio/MIDI ierīces.

Skaņas pārbaude bez DAW: `build/BirchKeysRender renders` ieraksta katru instrumentu WAV failā, `python3 tools/analyze2.py renders` izdrukā spektra „pirkstu nospiedumu” (līmenis, augstums centos, H2/H3, centroīds, izdzišana).

## FL Studio

Uzbūvē uz Windows (Visual Studio 2022 + CMake: `cmake -S . -B build`, `cmake --build build --config Release`) un `Birch Keys.vst3` iekopē `C:\Program Files\Common Files\VST3`.

## Licences

JUCE 8 (AGPLv3 / komerciālā), fonts Sedgwick Ave Display (OFL).
