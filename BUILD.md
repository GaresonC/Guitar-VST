# Building Guitar Amp VST

## Requirements
- **CMake** 3.22 or later — https://cmake.org/download/
- **Visual Studio 2019 or 2022** with the "Desktop development with C++" workload
- **Git** (CMake uses it to fetch JUCE automatically)
- Internet connection on first configure (JUCE ~200 MB download)

---

## Configure & Build

```bash
cd D:/repos/Guitar-VST

# Configure (downloads JUCE 7.0.9 automatically)
cmake -B build -G "Visual Studio 17 2022"

# Build Release
cmake --build build --config Release
```

For Visual Studio 2019 use `-G "Visual Studio 16 2019"`.

---

## Output locations

After a successful build:

| Format     | Path |
|------------|------|
| VST3       | `build/GuitarAmp_artefacts/Release/VST3/Guitar Amp.vst3` |
| Standalone | `build/GuitarAmp_artefacts/Release/Standalone/Guitar Amp.exe` |

Copy the `.vst3` folder into your DAW's VST3 scan path, e.g.:
- `C:\Program Files\Common Files\VST3\`

---

## Quick test without a DAW

Run the standalone app directly:

```
build\GuitarAmp_artefacts\Release\Standalone\Guitar Amp.exe
```

---

## Loading a Cabinet IR

Any mono `.wav` file works as a cabinet impulse response.
Free IRs are available from:
- Celestion (celestion.com)
- OwnHammer (ownhammer.com)
- Impulse Record (freefrom community packs)

Click **Load IR...** in the plugin, select the file, then enable it with **IR ON**.

---

## Architecture overview

| File | Role |
|------|------|
| `Source/PluginProcessor.cpp` | JUCE AudioProcessor, APVTS, processBlock |
| `Source/AmpProcessor.cpp`    | Input gain → waveshaper → tone stack → presence → master |
| `Source/Tuner.cpp`           | YIN pitch detection, 2048-sample buffer, atomic results |
| `Source/IRLoader.cpp`        | `juce::dsp::Convolution` wrapper for cabinet simulation |
| `Source/PluginEditor.cpp`    | 720×430 dark UI — tuner display, 6 knobs, channel selector, IR section |
