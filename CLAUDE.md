# MF AMP — Guitar VST Plugin

> **Memory note:** Do not update hidden (auto) memory for this project. Use this repo's CLAUDE.md instead.



A JUCE-based VST3 guitar amp plugin with neural amp modeling, cabinet IR simulation, and a real-time tuner. Aimed at high-gain/metal tones (Diezel 5150 voicing via neural model).

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
1. **Input Trim** — dB gain applied to raw input
2. **Tuner** — YIN pitch detection on dry signal (2048-sample buffer, atomic results to UI)
3. **Noise Gate** — `juce::dsp::NoiseGate`, threshold-based with attack/release envelopes
4. **Pre-Amp Stage** — 3-band EQ (low shelf / mid peak / high shelf) + soft-knee compressor with parallel blend
5. **Neural Amp** — LSTM-based amp modeling (RTNeural) with input/output gain
6. **Post-Amp Stage** — 3-band EQ + soft-knee compressor (same StageProcessor class, different default freqs)
7. **IR Loader** — Cabinet convolution via `juce::dsp::Convolution` (mono, trimmed, normalised)
8. **Stereo copy** — ch0 copied to ch1 for stereo output (happens here, before reverb)
9. **Reverb** — CloudSeed (third-party, MIT). Dry/wet mix done manually in processBlock, not via CloudSeed's own mix params. Only Mix/Decay/Size exposed to user; ~30 other params hard-coded in prepareToPlay().
10. **Post-IR EQ** — 8-band parametric EQ (band 0: low shelf, 1–6: peak Q=1.4, 7: high shelf)
11. **Output Volume** — final dB gain
12. **Mute** — Silence output (tuner stays active)

Each stage (except Input Trim, Output Volume, and Mute) has a bypass parameter. Bypass is implemented by zeroing the EQ gains and setting compressor ratio to 1 / blend to 0, rather than skipping the processor entirely — so the filters stay warm.

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
| `Source/KnobRangeSet.h` | Per-parameter display range struct (min/max/skew), user-editable |
| `Source/SectionColourSet.h` | Per-section border colour struct, user-editable |
| `Source/KnobRangesDialog.h/cpp` | Modal dialog for editing knob display ranges |
| `Source/SectionColoursDialog.h/cpp` | Modal dialog for editing section border colours |
| `Source/CloudSeed/` | Third-party reverb library (Ghost Note Engineering, MIT). Header-only DSP in `DSP/` subfolder |
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
- **State persistence:** `getStateInformation` / `setStateInformation` serialize APVTS XML plus these extra fields: `irFilePath`, `bundledIRName`, `ampModelPath`, `<KnobRanges>` (per-param min/max), `<SectionImages>` (base64 PNG per section), `<SectionColours>` (hex strings). UI rebuilds (slider attachments, images, colours) are dispatched async to the message thread after restore.
- **IR loading:** Dual-mode — user file (WAV) or bundled binary IRs. Bundled IRs are accessed via the `BinaryData` namespace. Default IR fallback in prepareToPlay: tries a hard-coded file path first, then falls back to the first `_wav` BinaryData resource.
- **Neural model format:** Supports GuitarML (PyTorch export, detected by `model_data` + `state_dict` keys) and native RTNeural JSON. GuitarML requires weight transposition from PyTorch's [4H,I] layout to RTNeural's [I,4H], plus combining ih+hh biases (see `loadGuitarMLModel()` in NeuralAmpProcessor.cpp). Default model is bundled as BinaryData and loaded in prepareToPlay if no model is active.
- **CloudSeed integration:** ReverbController is created fresh in prepareToPlay() — not persistent across prepare cycles. All CloudSeed params are normalised 0–1 (ScaleParam maps to real units). The dry/wet mix is applied manually in processBlock using a saved dry buffer, not via CloudSeed's own DryOut/WetOut.
- **Bypass strategy:** EQ bypass zeroes the gain dB values; compressor bypass sets ratio=1, makeup=0, blend=0. The DSP modules still process audio (filters stay warm / no transient on re-enable).
- **UI style:** Dark theme. Background `#141414`, panels `#222222`, accent `#ff6600` (orange), text `#e0e0e0`/`#888888`. Per-section colours and background images are user-customisable.
- **Font:** Uses JUCE default system font. A custom `InstrumentSerif` font was added in `b2d24f7` and reverted — do not re-add it.
- **PitchShifter:** Removed — the phase vocoder implementation never worked correctly. Would like to re-include if a working approach can be found.
- **Release target:** Building toward a GitHub release pipeline eventually. Portability matters — avoid hard-coded local paths (the default IR path `C:/Users/Gary/Documents/Cab IR/Custom IRs/` in PluginProcessor.cpp is a leftover that should be cleaned up before release).

---

## Known Issues

- **Knob range defaults not applied:** e.g. amp gain knob should default to -20..10 range per KnobRangeSet, but the display range doesn't reflect this on load.

---

## Planned / Requested Features

- **Reverb shimmer knob:** expose a blend control for CloudSeed's shimmer effect (pitch-shifted feedback in the late reverb tail).
