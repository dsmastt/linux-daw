# Project Instructions

## Project

This project is a custom low-latency audio application written in C++.

The agent is authorized to autonomously progress through DEVELOPMENT_PLAN.md. It should not wait for user confirmation between milestones unless a genuine blocker is encountered.

The long-term goal is to evolve it into a Linux DAW with:

- low-latency audio I/O
- real-time DSP
- guitar monitoring
- recording
- MIDI
- VST3 hosting
- mixer
- timeline/arrangement
- automation

## Current Stage

We are currently building the smallest possible working audio engine.

The first milestone is:

Audio input -> audio processing -> audio output

Do not implement DAW features prematurely.

## Development Rules

- Keep the architecture modular.
- Prefer small source files with clear responsibilities.
- Do not introduce unnecessary dependencies.
- Do not rewrite working code without a reason.
- Do not silently change build configuration.
- Do not invent APIs. Verify library APIs when necessary.
- Explain important architectural decisions before making large changes.
- Keep the real-time audio callback safe.

## Real-Time Audio Rules

Code running inside the audio callback must avoid:

- memory allocation
- filesystem operations
- blocking mutexes
- sleeping
- logging to the console
- network operations

The audio callback must execute predictably and quickly.

## Git Rules

Make small, meaningful commits.

Commit messages should describe the change, for example:

- Add initial CMake project
- Add audio device enumeration
- Implement audio passthrough
- Add gain processor

Never create giant unrelated commits.

## Coding Style

Prefer readable modern C++.

Use RAII.

Use const correctness.

Avoid global mutable state.

Keep interfaces small and explicit.

## AI Behavior

Before making major architectural changes, explain the proposed design.

When uncertain about a library API, say so and verify it rather than guessing.

Do not add features that were not requested.