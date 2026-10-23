# LA-50 — Linear Arithmetic-style synthesizer (JUCE, AU/VST3/Standalone)

An original synth in the style of the classic LA synthesis architecture:
each of two **Tones** has two **Partials** (saw/square oscillator → resonant 12 dB low-pass TVF → TVA,
each with its own 4-stage envelope, LFO and PWM) plus a short one-shot **attack transient**
(13 synthesized "PCM-style" attack samples: pluck, bell, chiff, thump, marimba, strike, "ah" voice,
hammer, brass blat, click, glass, breath). Partials combine in **Mix / Ring / Sync** structures.
Key modes: **Single / Dual / Split**. Master section: stereo chorus + reverb. 16 voices.
MIDI: notes, velocity, pitch bend, mod wheel (vibrato), sustain pedal (CC64), all-notes-off.
50 original factory presets (Program Change works in Logic).

> It is a structural homage, not a sample-accurate emulation: no Roland ROM samples or
> patch data are used. All attack samples and all 50 presets are original.

## Build (macOS, needed for the Audio Unit)

Requirements: Xcode + command line tools, CMake >= 3.22, internet (CMake fetches JUCE 8.0.4).

    cd LA50
    cmake -B build -G Xcode
    cmake --build build --config Release

The AU is copied to `~/Library/Audio/Plug-Ins/Components/LA-50.component`
(VST3 to `~/Library/Audio/Plug-Ins/VST3/`). Edit `COMPANY_NAME`, `PLUGIN_MANUFACTURER_CODE`
(4 chars, at least one uppercase) and `BUNDLE_ID` in CMakeLists.txt to your own before releasing.

## Use in Logic Pro

1. Validate: `auval -v aumu La50 Lafy` (should end with "AU VALIDATION SUCCEEDED").
2. Reopen Logic (it rescans on launch). If it doesn't appear: Logic Pro > Settings > Plug-in Manager > Reset & Rescan Selection.
3. Software Instrument slot > AU Instruments > LA50AUdio > LA-50.
4. Presets: the plugin's own drop-down, or the Logic preset menu (they are exposed as programs).

## Offline engine test (any OS, no JUCE)

    g++ -std=c++17 -O2 Tests/test_engine.cpp -o la50_test && ./la50_test

Renders every preset with a 4-note chord and checks for finite, audible, non-clipping output.

## Layout

    Source/Engine.h          synth engine (pure C++17)
    Source/Presets.h         the 50 factory presets
    Source/PluginProcessor.* parameters, MIDI, state, reverb
    Source/PluginEditor.*    UI (preset browser + Tone A / Tone B / Global tabs)
