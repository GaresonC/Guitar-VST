# MF AMP — Guitar VST Plugin

> **Memory note:** Do not update hidden (auto) memory for this project. Use this repo's CLAUDE.md instead.



A JUCE-based VST3 guitar amp plugin with neural amp modeling, cabinet IR simulation, pitch shifting, and a real-time tuner. Aimed at high-gain/metal tones (Diezel 5150 voicing via neural model).

---

## Build

**Requirements:**
- CMake 3.22+
- Visual Studio 2022 (or 2019) with "Desktop development with C++" workload
- Git (CMake uses FetchContent to download JUCE and RTNeural automatically on first configure)
- Internet connection on first configure (~200 MB JUCE download)

**Commands:**
```bash
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

Use `-G "Visual Studio 16 2019"` for VS 2019.

**Output:**
- VST3: `build/GuitarAmp_artefacts/Release/VST3/Guitar Amp.vst3`
- Standalone: `build/GuitarAmp_artefacts/Release/Standalone/Guitar Amp.exe`

**Deploy:** Copy the `.vst3` folder to `C:\Program Files\Common Files\VST3\`

**Quick test (no DAW):** Run the standalone exe directly.

---

## Architecture

**Plugin info:**
- Plugin name: `MF AMP`, Code: `GAmp`, Company: `GaresonC`
- Formats: VST3 + Standalone
- I/O: Mono input → Stereo output (channel 1 copied from channel 0 post-convolution)
- UI size: 1900×420 px

**Signal chain (processBlock order):**
1. **Tuner** — YIN pitch detection on dry signal (2048-sample buffer, atomic results to UI)
2. **Noise Gate** — Threshold-based gate with attack/release envelopes
3. **Pitch Shifter** — Phase vocoder (±12 semitones, 256-point FFT, 4× overlap)
4. **Pre-Amp Stage** — 3-band EQ + soft-knee compressor
5. **Neural Amp** — LSTM-based amp modeling (RTNeural) with input/output gain
6. **Post-Amp Stage** — 3-band EQ + soft-knee compressor
7. **IR Loader** — Cabinet convolution via `juce::dsp::Convolution`
8. **Post-IR EQ** — 8-band parametric EQ (80 Hz – 16 kHz)
9. **Mute** — Silence output (tuner stays active)

---

## Key Files

| File | Role |
|------|------|
| `Source/PluginProcessor.h/cpp` | JUCE AudioProcessor, APVTS parameter layout, processBlock routing, state save/load |
| `Source/PluginEditor.h/cpp` | Full UI (720×430 dark theme, all controls, preset manager, file dialogs) |
| `Source/NeuralAmpProcessor.h/cpp` | RTNeural LSTM model loading and inference |
| `Source/StageProcessor.h/cpp` | 3-band EQ + soft-knee compressor (used for pre & post amp stages) |
| `Source/EQProcessor.h/cpp` | 8-band parametric EQ (post-IR) |
| `Source/IRLoader.h/cpp` | `juce::dsp::Convolution` wrapper for cabinet IRs |
| `Source/Tuner.h/cpp` | YIN pitch detection algorithm |
| `Source/PitchShifter.h/cpp` | Phase vocoder pitch shifter |
| `CMakeLists.txt` | Build config, FetchContent for JUCE 7.0.9 and RTNeural |

**Assets:**
- `Models/6505Plus_Red_DirectOut.json` — Default neural amp model (Diezel 5150 voicing)
- `IRs/` — 17 bundled cabinet IRs (Djammincabs), compiled into binary via `juce_add_binary_data`

---

## Technology Stack

- **JUCE 7.0.9** — Audio plugin framework, DSP utilities, GUI
- **RTNeural** (main branch) — Real-time neural network inference (LSTM + Dense layers)
- **C++17**
- **CMake** with FetchContent (no manual dependency setup needed)

---

## Key Patterns & Conventions

- **Parameters:** All plugin parameters are registered via APVTS (`createParameterLayout()` in `PluginProcessor.cpp`). Always use `apvts.getRawParameterValue()` or `apvts.getParameter()` to access them — never raw member variables.
- **Thread safety:** Audio thread uses atomic variables to pass tuner results to the UI (polled at 15 Hz via timer). Neural model loading uses a SpinLock.
- **State persistence:** `getStateInformation` / `setStateInformation` serialize the full APVTS XML plus IR path and neural model path.
- **IR loading:** Dual-mode — user file (WAV) or bundled binary IRs. Bundled IRs are accessed via the `BinaryData` namespace.
- **Neural model format:** Supports GuitarML (PyTorch export, detected by `model_data` + `state_dict` keys) and native RTNeural JSON.
- **UI style:** Dark theme. Background `#141414`, panels `#222222`, accent `#ff6600` (orange), text `#e0e0e0`/`#888888`.
- **Font:** Uses JUCE default system font. A custom `InstrumentSerif` font was added in `b2d24f7` and reverted — do not re-add it.

---

## Known Issues

- Parameter display values are showing incorrectly (as of last commit `6c65f9f`) — unfixed.
