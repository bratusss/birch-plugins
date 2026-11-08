# Birch Box: boom bap drum bus

VST3 efekts (JUCE 8, C++), kas bungām piešķir 90. gadu semplera raksturu. Dizains veidots pēc Birch Please logo.

| Poga | Ko dara |
|---|---|
| **Grit** | Samazina bitu dziļumu un sample rate (12 bitu semplera raksturs) |
| **Knock** | Transientu pastiprinātājs un kompresors (GR metrs rāda, cik tas spiež) |
| **Thump** | Saturācija tikai zemajām frekvencēm (zem 110 Hz) |
| **Warmth** | Lentes saturācija visam signālam |
| **Dust** | Vinila troksnis un sprakšķi, kas noklust bungu sitienu brīžos |
| **Wobble** | Lentes wow & flutter |
| **Pitch** | Maina skaņas augstumu (±12 pustoņi), tempu nemainot. **45→33** dod klasisko -5,2 pustoņu triku |
| **Low Cut** | Nogriež subbasu un dūkoņu pirms apstrādes |
| **Width / Mono Bass** | Stereo platums; zem 120 Hz viss mono |
| **Sidechain** | Knock kompresors un Dust ducking klausās sidechain ieeju (piem., kick) |
| **Era** | `'88 / '94 / '98` maina visu pogu raksturu |
| **Mix / Out / Bypass** | Paralēlā apstrāde, izejas skaļums, A/B salīdzināšana |

12 gatavi preseti (Golden Era, Dusty Basement, Hard Knock, SP Crunch, Crate Digger 45-33, Vinyl Rip, Lo-Fi Tape, Clean Thump, Parallel Smash, Mono Basement, Kick Pump u.c.) parādās arī FL Studio presetu izvēlnē.

## Testēšana uz Linux bez DAW

Vienreiz uzinstalē būvēšanas rīkus (Fedora):

```bash
sudo dnf install -y gcc-c++ cmake ninja-build git alsa-lib-devel freetype-devel fontconfig-devel libX11-devel libXext-devel libXinerama-devel libXrandr-devel libXcursor-devel libXcomposite-devel libXrender-devel mesa-libGL-devel mesa-libGLU-devel
```

Uzbūvē un palaid:

```bash
./build.sh
```

```bash
"./build/BirchBox_artefacts/Release/Standalone/Birch Box"
```

Avotu var pārslēgt ar pogām apakšā: **Demo bīts** (90 BPM ar swing), **Fails...** (savs loop, to var arī ievilkt logā) vai **Ieeja**. Demo bīta režīmā sidechain signāls ir demo kick, tāpēc **Sidechain** var izmēģināt arī bez DAW (preset „Kick Pump”). Audio izeju var nomainīt ar pogu **Options**.

## FL Studio

FL Studio darbojas uz Windows un macOS, tāpēc Linux `.vst3` tur neder. Uzbūvē uz Windows ar Visual Studio 2022 un CMake (`cmake -S . -B build`, tad `cmake --build build --config Release`), un `Birch Box.vst3` iekopē mapē `C:\Program Files\Common Files\VST3`.

Sidechain FL Studio: mikserī uz kick kanāla izvēlies Birch Box kanāla maršrutēšanas bultiņu un *Sidechain to this track*, tad plugina *Wrapper → Processing* piešķir sidechain ieeju.

## Licences

- JUCE 8: AGPLv3 vai komerciālā licence. Pārdošanai vajag JUCE licenci (bezmaksas „Starter” līmenis ir līdz noteiktam ieņēmumu apjomam).
- Fonts Sedgwick Ave Display: SIL Open Font License (`Resources/OFL.txt`).

Izstrādei: `BIRCHBOX_SNAPSHOT=/ceļš/fails.png` saglabā loga attēlu pēc ~1,5 sekundēm.
