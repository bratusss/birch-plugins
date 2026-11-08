# Birch plugins

Boom bap VST3 plugini FL Studio (JUCE 8, C++), dizains pēc Birch Please logo.

- [BirchBox](BirchBox/): drum bus efekts (Grit, Knock, Thump, Warmth, Dust, Wobble, Pitch, Low Cut, Width, Sidechain, Era).
- [BirchKeys](BirchKeys/): 31 fizikāli modelēti instrumenti 5 kategorijās, ar 8 basiem. Fizika: [BirchKeys/RESEARCH.md](BirchKeys/RESEARCH.md).

## Windows buildi

Katrs push uz `main` palaiž [GitHub Actions](.github/workflows/windows.yml), kas Windows serverī ar Visual Studio 2022 uzbūvē abus VST3 un pievieno tos relīzei **latest** kā divus ZIP: `Birch Box - Windows VST3.zip` un `Birch Keys - Windows VST3.zip`. Katrā ZIP ir `.vst3` mape, `install.bat` un `INSTALL.txt`.

## Linux (izstrāde)

Katrā projektā `./build.sh` uzbūvē Standalone un VST3 (sk. projekta README).
