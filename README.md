# Linux DAW

A custom low-latency Linux DAW / guitar rig built in C++20 with JUCE 9.

## Version 0.1

The first usable release: open an audio device, monitor audio with a full guitar
signal chain, record, play audio, host VST3 plugins, mix tracks, and save/load
projects.

## Features

- Low-latency ALSA audio I/O with selectable device, sample rate, and buffer size
- Guitar DSP chain per track: gain, noise gate, 4-band parametric EQ, compressor,
  distortion (soft/hard/cubic/asymmetric), VST3 plugin slot, cabinet IR convolution,
  and reverb
- Four mixer tracks with volume, pan, mute, solo, bus routing, and sends
- Timeline with audio clips (move, resize, play) and waveform display
- Transport with play/pause/stop, sample position, and tempo
- MIDI input with device discovery, note/velocity/controller events, and MIDI data on tracks
- Parameter automation for volume, pan, DSP, and plugin parameters
- WAV recording with a lock-free producer/consumer design
- Project save/load (JSON) with relative paths, plus undo/redo
- Live diagnostics: meters, clipping indicator, per-processor DSP profiling, memory usage

## Building

Requirements: CMake 3.24+, a C++20 compiler, and the development packages for
ALSA, X11, FreeType, and FFTW3f.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target linux-daw
./build/linux-daw_artefacts/Release/linux-daw
```

Debug build: configure without `-DCMAKE_BUILD_TYPE` (defaults to debug).

Installation:

```bash
cmake --install build --prefix /usr/local
```

This installs the binary, a desktop launcher, and an application icon.

## Tests

```bash
cmake --build build --target linux-daw-tests
./build/linux-daw-tests_artefacts/linux-daw-tests

cmake --build build --target linux-daw-vst3-test
./build/linux-daw-vst3-test_artefacts/linux-daw-vst3-test   # requires an audio device
```

## Usage

- Start the app, pick input/output devices, sample rate, and buffer size, then Start Audio.
- Tune the guitar chain in the DSP tab, control the mix in the Mixer tab.
- Load a clip file and arrange clips in the Timeline tab; press Play.
- Load cabinet impulse responses in the Cab tab, host VST3 plugins in the Plugin tab.
- Save and load projects from the File menu (Ctrl+S / Ctrl+O), undo with Ctrl+Z.
