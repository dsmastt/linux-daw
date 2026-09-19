# Linux DAW - Development Plan

A custom low-latency Linux DAW/audio engine built in C++.

The project starts as a simple real-time guitar monitoring application and gradually grows into a complete DAW.

---

# 0. Project Principles

These rules apply throughout the project.

## Architecture

* Keep subsystems modular.
* Prefer small, focused classes.
* Keep audio processing separate from GUI code.
* Keep the real-time audio path predictable.
* Avoid unnecessary dependencies.
* Do not implement future features early.
* Do not rewrite working systems without a concrete reason.

## Real-Time Audio Rules

Code running on the real-time audio thread must avoid:

* memory allocation
* deallocation
* filesystem access
* blocking mutexes
* sleeping
* network operations
* console logging
* unpredictable expensive operations

The audio callback must remain deterministic and fast.

## Development Rules

Each milestone must:

1. Have a clearly defined goal.
2. Make the smallest necessary changes.
3. Build successfully.
4. Be manually tested.
5. Be committed to Git.
6. Only then should the next milestone begin.

Do not combine unrelated milestones.

## AI Coding Rules

The AI coding assistant must:

* Read `AGENTS.md` before modifying the project.
* Work only on the current milestone.
* Avoid implementing future features.
* Explain significant architectural decisions.
* Avoid unnecessary refactoring.
* Never silently replace working architecture.
* Verify APIs rather than inventing them.
* Tell the developer when an architectural decision is uncertain.

---

# 1. Foundation

## Milestone 1 - Basic CMake Application

**Status:** Completed

### Goal

Create the smallest possible C++ application that builds successfully with CMake.

### Requirements

* C++20
* CMake
* `src/main.cpp`
* executable target
* Linux build
* application prints a startup message

### Success Criteria

```text
cmake -S . -B build
cmake --build build
./build/linux-daw
```

runs successfully.

### Git Commit

```text
Add initial CMake project
```

---

# 2. JUCE Integration

## Milestone 2 - Add JUCE

**Status:** Completed

### Goal

Integrate JUCE into the CMake project.

### Requirements

* JUCE added through a reproducible CMake setup
* basic JUCE application
* Linux build
* no audio processing yet
* no GUI complexity

### Success Criteria

The project builds and launches successfully using JUCE.

### Git Commit

```text
Integrate JUCE
```

---

# 3. Audio Device Discovery

## Milestone 3 - Enumerate Audio Devices

**Status:** Completed

### Goal

Discover the audio devices available to the application.

### Requirements

Print:

* input devices
* output devices
* device names
* available channels where supported
* useful device information

Do not start audio yet.

### Success Criteria

The application identifies the Audio Array AI-04 and other available audio devices.

### Git Commit

```text
Add audio device discovery
```

---

# 4. Audio Engine

## Milestone 4 - Create AudioEngine

**Status:** Completed

### Goal

Create a dedicated audio engine abstraction.

### Architecture

```text
Application
    |
    v
AudioEngine
    |
    v
JUCE Audio Device Layer
```

### Requirements

Create a clean `AudioEngine` responsible for:

* opening an audio device
* configuring sample rate
* configuring buffer size
* starting audio
* stopping audio
* handling the audio callback

Do not add DSP yet.

### Success Criteria

The application can open the selected audio interface and start its audio device without errors.

### Git Commit

```text
Add audio engine
```

---

# 5. Audio Passthrough

## Milestone 5 - Input to Output

**Status:** Completed

### Goal

Make the first real-time audio path.

```text
AI-04 Input
     |
     v
Audio Callback
     |
     v
AI-04 Output
```

### Requirements

* input audio reaches output
* selectable input/output device
* selectable buffer size if supported
* selectable sample rate if supported
* no unnecessary processing

### Success Criteria

Plug guitar into the AI-04 and hear it through the output.

### Important

Measure practical latency.

Do not optimize prematurely.

### Git Commit

```text
Implement audio passthrough
```

---

# 6. Audio Diagnostics

## Milestone 6 - Audio Debugging Tools

**Status:** Completed

### Goal

Make audio behavior observable.

### Add diagnostics for:

* sample rate
* buffer size
* input channel count
* output channel count
* callback timing
* underruns/overruns where available
* CPU usage where practical

### Add optional debug mode.

Do not continuously print from the real-time audio callback.

### Success Criteria

The application can report its audio configuration and detect obvious instability.

### Git Commit

```text
Add audio diagnostics
```

---

# 7. First DSP

## Milestone 7 - Gain Processor

**Status:** Completed

### Goal

Introduce the first DSP processor.

```text
Input
  |
  v
Gain
  |
  v
Output
```

### Requirements

* gain parameter
* sample-by-sample or block processing as appropriate
* no allocations inside the audio callback
* parameter changes must be safe for the audio thread

### Success Criteria

Changing gain changes the guitar volume without introducing obvious artifacts.

### Git Commit

```text
Add gain processor
```

---

# 8. DSP Graph

## Milestone 8 - Processing Chain

**Status:** Completed

### Goal

Stop hardcoding DSP into the audio callback.

Create a basic processing architecture:

```text
Input
  |
  v
DSP Graph
  |
  +--> Gain
  |
  +--> Processor
  |
  +--> Processor
  |
  v
Output
```

### Requirements

* processors have a consistent interface
* processors can be ordered
* audio engine processes the graph
* architecture can later support plugins

### Success Criteria

Multiple processors can be chained without modifying the core audio callback.

### Git Commit

```text
Add DSP processing graph
```

---

# 9. Guitar DSP

## Milestone 9 - Noise Gate

**Status:** Completed

Add a real-time noise gate.

```text
Input
  |
  v
Noise Gate
  |
  v
Output
```

Parameters should eventually include:

* threshold
* attack
* release

### Git Commit

```text
Add noise gate
```

---

## Milestone 10 - EQ

**Status:** Completed

Add a basic parametric or multi-band EQ.

### Requirements

* frequency
* gain
* Q where applicable
* multiple bands if architecture permits

### Git Commit

```text
Add EQ processor
```

---

## Milestone 11 - Compressor

**Status:** Completed

Add a real-time compressor.

### Parameters

* threshold
* ratio
* attack
* release
* makeup gain

### Git Commit

```text
Add compressor
```

---

## Milestone 12 - Distortion

**Status:** Completed

Add guitar-oriented distortion/overdrive.

### Requirements

Experiment with:

* soft clipping
* hard clipping
* waveshaping
* gain
* tone

Keep DSP modular so multiple distortion algorithms can exist.

### Git Commit

```text
Add distortion processor
```

---

# 10. Cabinet / IR Processing

## Milestone 13 - Impulse Response Convolution

**Status:** Completed

### Goal

Support cabinet impulse responses.

```text
Distortion
    |
    v
Convolution
    |
    v
Output
```

### Requirements

* load WAV impulse responses
* convolution processing
* efficient real-time implementation
* avoid loading files from the audio thread

### Success Criteria

A guitar signal can be processed through a cabinet IR.

### Git Commit

```text
Add impulse response processor
```

---

# 11. Reverb

## Milestone 14 - Reverb

**Status:** Completed

Add a real-time reverb processor.

### Requirements

* wet/dry
* decay
* size or equivalent parameters
* real-time safe processing

### Git Commit

```text
Add reverb processor
```

---

# 12. Audio Engine Refactor

## Milestone 15 - Separate Engine and DSP

**Status:** Completed

Refactor the architecture into clear layers.

Target architecture:

```text
Application
    |
    +----------------+
    |                |
    v                v
GUI / Control     AudioEngine
                     |
                     v
                 Audio Graph
                     |
          +----------+----------+
          |          |          |
         Gate       EQ      Compressor
                                |
                           Distortion
                                |
                                v
                               IR
                                |
                                v
                              Reverb
```

### Goal

The audio engine should not know about GUI implementation details.

### Git Commit

```text
Separate audio engine and DSP architecture
```

---

# 13. Recording

## Milestone 16 - WAV Recording

**Status:** Completed

### Goal

Record audio from the input.

```text
Input
  |
  +--------> DSP / Monitoring
  |
  v
Recorder
  |
  v
WAV File
```

### Requirements

* start recording
* stop recording
* WAV output
* correct sample rate
* correct channel configuration
* disk writing must not block the audio thread

### Important

Use a producer/consumer architecture or equivalent buffering strategy.

The audio callback must never directly perform arbitrary filesystem operations.

### Git Commit

```text
Add audio recording
```

---

# 14. Playback

## Milestone 17 - Audio File Playback

**Status:** Completed

### Goal

Play recorded WAV files through the engine.

### Requirements

* load WAV
* play
* pause
* stop
* seek eventually
* sample-rate handling

### Git Commit

```text
Add audio file playback
```

---

# 15. GUI Foundation

## Milestone 18 - Basic GUI

**Status:** Completed

### Goal

Create a basic application interface.

Initial UI:

```text
+---------------------------------------+
| Linux DAW                             |
+---------------------------------------+
| Input:  AI-04                         |
| Output: AI-04                         |
| Sample Rate: 48000                    |
| Buffer: 128                           |
|                                       |
| [ Start Audio ] [ Stop Audio ]        |
+---------------------------------------+
```

### Requirements

* device selection
* audio start/stop
* sample-rate display
* buffer-size display
* basic status indicators

Do not build the full DAW UI yet.

### Git Commit

```text
Add basic audio interface GUI
```

---

# 16. DSP GUI

## Milestone 19 - Processor Controls

**Status:** Completed

Expose parameters for:

* gain
* gate
* EQ
* compressor
* distortion
* IR
* reverb

### Requirement

GUI changes must communicate safely with the audio thread.

### Git Commit

```text
Add DSP controls
```

---

# 17. Metering

## Milestone 20 - Level Meters

**Status:** Completed

Add:

* input meter
* output meter
* peak detection
* clipping indicator

### Requirements

Do not perform unsafe GUI operations from the audio thread.

Use appropriate lock-free or buffered communication.

### Git Commit

```text
Add audio level meters
```

---

# 18. Plugin Architecture

## Milestone 21 - VST3 Host Foundation

**Status:** Completed

### Goal

Prepare the engine to host external VST3 plugins.

Architecture:

```text
Audio Graph
    |
    +--> Built-in DSP
    |
    +--> VST3 Plugin
    |
    +--> Built-in DSP
```

### Requirements

* integrate VST3 hosting
* discover plugins
* load a plugin
* process audio through it
* unload safely

Do not build plugin UI management yet.

### Git Commit

```text
Add VST3 plugin hosting
```

---

# 19. Plugin UI

## Milestone 22 - Plugin Management

**Status:** Completed

Add:

* plugin browser
* plugin loading
* plugin removal
* plugin bypass
* plugin parameter interaction
* plugin editor windows where supported

### Git Commit

```text
Add VST3 plugin management
```

---

# 20. Mixer

## Milestone 23 - Track Model

**Status:** Completed

Introduce tracks.

```text
Track 1
Track 2
Track 3
Track 4
   |
   v
Master
```

Each track should eventually have:

* audio source
* volume
* pan
* mute
* solo
* DSP chain
* plugin chain
* routing

### Git Commit

```text
Add track architecture
```

---

## Milestone 24 - Mixer

Build the mixer UI.

Each channel:

```text
+-----------+
| Track 1   |
|           |
| [Plugin]  |
| [Plugin]  |
|           |
| Volume    |
|   ||      |
|   ||      |
|   ||      |
|           |
| M S       |
+-----------+
```

### Git Commit

```text
Add mixer
```

---

# 21. Timeline

## Milestone 25 - Transport

Add:

* play
* pause
* stop
* position
* tempo
* sample position

### Git Commit

```text
Add transport system
```

---

## Milestone 26 - Timeline

Create a basic arrangement timeline.

```text
        1       2       3       4
        |-------|-------|-------|

Track 1 [==========]
Track 2          [===========]
Track 3                  [======]
```

### Requirements

* clips
* positioning
* moving clips
* resizing clips
* playback

### Git Commit

```text
Add timeline
```

---

# 22. MIDI

## Milestone 27 - MIDI Input

Add:

* MIDI device discovery
* MIDI input
* note events
* velocity
* controller events

### Git Commit

```text
Add MIDI input
```

---

## Milestone 28 - MIDI Tracks

Allow MIDI data to exist on tracks.

### Git Commit

```text
Add MIDI tracks
```

---

# 23. Automation

## Milestone 29 - Parameter Automation

Support automation of parameters such as:

* volume
* pan
* plugin parameters
* DSP parameters

Example:

```text
Volume
100% |       /\
     |      /  \
 50% |_____/    \____
     |
     +----------------
          Time
```

### Git Commit

```text
Add parameter automation
```

---

# 24. Project Files

## Milestone 30 - Project Format

Create a project format containing:

* tracks
* clips
* plugin state
* DSP parameters
* automation
* routing
* project metadata

Do not rely on absolute paths where avoidable.

### Git Commit

```text
Add project file format
```

---

# 25. Save / Load

## Milestone 31 - Project Persistence

Implement:

* new project
* save
* save as
* load
* project recovery handling

### Git Commit

```text
Add project save and load
```

---

# 26. Undo / Redo

## Milestone 32 - Undo System

Add undo/redo for project-state operations.

Potential operations:

* move clip
* delete clip
* change parameter
* add/remove plugin
* change routing
* track creation/removal

### Git Commit

```text
Add undo and redo
```

---

# 27. Performance

## Milestone 33 - Audio Performance Profiling

Measure:

* callback execution time
* CPU usage
* DSP cost
* plugin cost
* memory usage
* underruns
* latency

Do not optimize based purely on intuition.

### Git Commit

```text
Add audio performance profiling
```

---

# 28. Multithreading

## Milestone 34 - Background Processing

Move appropriate non-real-time work off the audio thread.

Potential candidates:

* file loading
* waveform generation
* plugin scanning
* project loading
* sample preparation
* analysis

Do not blindly add threads to the audio callback.

### Git Commit

```text
Add background processing architecture
```

---

# 29. Waveforms

## Milestone 35 - Waveform Generation

Display audio waveforms for clips.

### Requirements

* background waveform generation
* cached waveform data
* efficient rendering
* zooming

### Git Commit

```text
Add waveform rendering
```

---

# 30. Advanced Routing

## Milestone 36 - Audio Routing

Support:

```text
Track
  |
  +--> Bus
  |
  +--> Send
  |
  +--> Master
```

Eventually support:

* buses
* sends
* returns
* sidechains where practical

### Git Commit

```text
Add audio routing
```

---

# 31. Low-Latency Optimization

## Milestone 37 - Latency Optimization

Measure and optimize:

* input latency
* processing latency
* output latency
* buffer size
* callback scheduling
* DSP overhead
* unnecessary copies

Investigate:

* SIMD
* efficient memory layouts
* lock-free communication
* real-time scheduling where appropriate
* PipeWire/ALSA configuration

Do not introduce GPU processing merely because the GPU exists.

The GPU is not a magical latency fairy.

### Git Commit

```text
Optimize real-time audio path
```

---

# 32. Crash Safety

## Milestone 38 - Error Handling

Handle:

* device disappearance
* device changes
* invalid audio configuration
* plugin failures
* corrupted project files
* missing audio files
* unsupported plugin formats

The application should fail gracefully instead of taking the entire session with it.

### Git Commit

```text
Improve audio and project error handling
```

---

# 33. Testing

## Milestone 39 - Automated Tests

Create tests for:

* DSP algorithms
* parameter handling
* project serialization
* clip manipulation
* transport
* routing
* audio processing correctness

### Git Commit

```text
Add automated tests
```

---

# 34. Final Architecture Cleanup

## Milestone 40 - Architecture Review

Review the entire application.

Identify:

* unnecessary dependencies
* duplicated code
* unsafe real-time operations
* unclear ownership
* excessive coupling
* performance bottlenecks
* inconsistent APIs

Refactor only where justified.

### Git Commit

```text
Refine application architecture
```

---

# 35. Release Preparation

## Milestone 41 - Packaging

Create a reproducible Linux build.

Eventually support:

* release build
* debug build
* installation
* desktop launcher
* application icon
* configuration directory
* user data directory

### Git Commit

```text
Add Linux release packaging
```

---

# 36. Version 0.1

## Milestone 42 - First Usable Release

Version 0.1 should be capable of:

* opening an audio device
* monitoring audio
* applying built-in DSP
* recording
* playing audio
* using VST3 plugins
* basic tracks
* basic mixer
* saving/loading projects

This is the first point at which the project can reasonably be called a usable DAW.

---

# Future Features

These are intentionally NOT part of the initial roadmap.

Possible future systems:

* advanced MIDI editing
* piano roll
* sampler
* synthesizers
* built-in instruments
* spectral editing
* advanced routing
* surround audio
* control surfaces
* plugin sandboxing
* crash recovery
* collaborative projects
* scripting
* AI-assisted tools
* GPU-accelerated offline processing
* Linux-native plugin management
* custom audio backend
* Windows support
* macOS support

These should only be added after the core architecture is stable.

---

# Current Milestone

## Milestone 24 - Mixer

The track model is in place: four tracks, each with volume, pan, mute, solo, and its own DSP and plugin chain, mixed to a master.

The next task is to build the mixer UI with a channel strip per track: name, plugin list, volume, and mute/solo controls.

Do not skip ahead.

---

# Standard Development Loop

For every milestone:

```text
1. Read this plan
        ↓
2. Identify current milestone
        ↓
3. Ask AI to implement ONLY that milestone
        ↓
4. Review changes
        ↓
5. Build
        ↓
6. Test manually
        ↓
7. Fix problems
        ↓
8. Commit
        ↓
9. Update this plan if necessary
        ↓
10. Move to next milestone
```

Standard Git workflow:

```bash
git status
git diff
git add .
git commit -m "Description of change"
git push
```

Never begin the next milestone while the current milestone is known to be broken.

---

# Definition of Done

A milestone is considered complete only when:

* the code builds
* the intended feature works
* obvious errors are handled
* the architecture is understandable
* no unrelated features were added
* the working tree is committed
* the commit message describes the change

The goal is not to produce the largest amount of code.

The goal is to continuously produce a working system.
