#include <cmath>
#include <iostream>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include "AudioGraph.h"
#include "Clip.h"
#include "CompressorProcessor.h"
#include "DistortionProcessor.h"
#include "GainProcessor.h"
#include "NoiseGateProcessor.h"
#include "ParametricEqProcessor.h"
#include "Project.h"
#include "ReverbProcessor.h"
#include "Track.h"
#include "Transport.h"

namespace
{
int checks = 0;
int failures = 0;

void check (bool condition, const char* message, int line)
{
    ++checks;

    if (!condition)
    {
        ++failures;
        std::cout << "FAIL line " << line << ": " << message << std::endl;
    }
}

#define CHECK(cond) check ((cond), #cond, __LINE__)

float peakOf (const juce::AudioBuffer<float>& buffer)
{
    float peak = 0.0f;

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        peak = std::max (peak, juce::FloatVectorOperations::findMinAndMax (buffer.getReadPointer (channel), buffer.getNumSamples()).getEnd());

    return peak;
}

void fillSine (juce::AudioBuffer<float>& buffer, float frequency, double sampleRate, float amplitude)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* data = buffer.getWritePointer (channel);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
            data[i] = amplitude * std::sin (2.0f * 3.14159265358979323846f * frequency * i / static_cast<float> (sampleRate));
    }
}

void processThrough (daw::AudioProcessor& processor, juce::AudioBuffer<float>& buffer, double sampleRate)
{
    processor.prepare (sampleRate, buffer.getNumSamples());
    processor.reset();

    const int numChannels = buffer.getNumChannels();

    const float* const* readPointers = buffer.getArrayOfReadPointers();

    juce::AudioBuffer<float> work (numChannels, buffer.getNumSamples());

    for (int channel = 0; channel < numChannels; ++channel)
        work.copyFrom (channel, 0, buffer, channel, 0, buffer.getNumSamples());

    float* const* writePointers = work.getArrayOfWritePointers();

    processor.process (writePointers, numChannels, buffer.getNumSamples());

    for (int channel = 0; channel < numChannels; ++channel)
        buffer.copyFrom (channel, 0, work, channel, 0, buffer.getNumSamples());

    (void) readPointers;
}

void testGainProcessor()
{
    daw::GainProcessor gain;
    gain.setGainDb (-6.0f);

    juce::AudioBuffer<float> buffer (1, 1024);
    fillSine (buffer, 1000.0, 48000.0, 0.5f);

    processThrough (gain, buffer, 48000.0);

    CHECK (std::abs (peakOf (buffer) - 0.5f * 0.5012f) < 0.05f);
    CHECK (std::abs (gain.getGainDb() + 6.0f) < 0.001f);
}

void testGateProcessor()
{
    daw::NoiseGateProcessor gate;
    gate.setThresholdDb (-30.0f);
    gate.setAttackMs (0.1f);
    gate.setReleaseMs (20.0f);

    juce::AudioBuffer<float> buffer (1, 48000);
    fillSine (buffer, 440.0, 48000.0, 0.01f);

    processThrough (gate, buffer, 48000.0);

    const float gatedPeak = peakOf (buffer);
    CHECK (gatedPeak < 0.005f);

    fillSine (buffer, 440.0, 48000.0, 0.5f);
    processThrough (gate, buffer, 48000.0);

    const float openPeak = peakOf (buffer);
    CHECK (openPeak > 0.3f);

    CHECK (std::abs (gate.getThresholdDb() + 30.0f) < 0.001f);
}

void testEqProcessor()
{
    daw::ParametricEqProcessor eq;
    eq.setBandParameters (0, 1000.0, 12.0f, 1.0f);

    juce::AudioBuffer<float> high (1, 8192);
    juce::AudioBuffer<float> low (1, 8192);
    fillSine (high, 1000.0, 48000.0, 0.2f);
    fillSine (low, 100.0, 48000.0, 0.2f);

    processThrough (eq, high, 48000.0);
    processThrough (eq, low, 48000.0);

    const float highPeak = peakOf (high);
    const float lowPeak = peakOf (low);

    CHECK (highPeak > 0.5f);
    CHECK (lowPeak < 0.4f);
    CHECK (highPeak > lowPeak * 2.0f);
}

void testCompressorProcessor()
{
    daw::CompressorProcessor compressor;
    compressor.setThresholdDb (-20.0f);
    compressor.setRatio (4.0f);
    compressor.setAttackMs (0.1f);
    compressor.setReleaseMs (50.0f);

    juce::AudioBuffer<float> buffer (1, 96000);
    fillSine (buffer, 440.0, 48000.0, 0.9f);

    processThrough (compressor, buffer, 48000.0);

    const float compressedPeak = peakOf (buffer);
    CHECK (compressedPeak < 0.7f);

    CHECK (std::abs (compressor.getRatio() - 4.0f) < 0.001f);
    CHECK (std::abs (compressor.getThresholdDb() + 20.0f) < 0.001f);
}

void testDistortionProcessor()
{
    daw::DistortionProcessor distortion;
    distortion.setMode (daw::DistortionMode::hard);
    distortion.setDriveDb (20.0f);

    juce::AudioBuffer<float> buffer (1, 4096);
    fillSine (buffer, 200.0, 48000.0, 0.5f);

    processThrough (distortion, buffer, 48000.0);

    const float clippedPeak = peakOf (buffer);
    CHECK (clippedPeak <= 1.05f);

    CHECK (static_cast<int> (distortion.getMode()) == static_cast<int> (daw::DistortionMode::hard));
}

void testReverbProcessor()
{
    daw::ReverbProcessor reverb;
    reverb.setWetDry (0.5f);
    reverb.setDecay (0.8f);
    reverb.setSize (0.5f);

    juce::AudioBuffer<float> buffer (2, 48000);

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        buffer.setSample (0, i, i == 0 ? 1.0f : 0.0f);
        buffer.setSample (1, i, i == 0 ? 1.0f : 0.0f);
    }

    processThrough (reverb, buffer, 48000.0);

    CHECK (std::abs (buffer.getSample (0, 0) - 0.5f) < 0.01f);

    float tailPeak = 0.0f;

    for (int i = 1000; i < 10000; ++i)
        tailPeak = std::max (tailPeak, std::abs (buffer.getSample (0, i)));

    CHECK (tailPeak > 0.001f);
}

void testTransport()
{
    daw::Transport transport;
    CHECK (!transport.isPlaying());
    CHECK (transport.getPositionSamples() == 0);

    transport.play();
    CHECK (transport.isPlaying());

    transport.advance (128);
    transport.advance (128);
    CHECK (transport.getPositionSamples() == 256);

    transport.pause();
    CHECK (!transport.isPlaying());
    CHECK (transport.getPositionSamples() == 256);

    transport.play();
    transport.advance (128);
    CHECK (transport.getPositionSamples() == 384);

    transport.stop();
    CHECK (transport.getPositionSamples() == 0);
    CHECK (!transport.isPlaying());

    transport.setTempoBpm (135.0);
    CHECK (std::abs (transport.getTempoBpm() - 135.0) < 0.001);
}

void testClipManipulation()
{
    daw::Track track;

    CHECK (track.addClip (100, 1000, 0) == 0);
    CHECK (track.addClip (5000, 500, 10) == 1);
    CHECK (track.getNumClips() == 2);

    track.moveClip (0, 250);
    CHECK (track.getClip (0).startSamples.load() == 250);

    track.resizeClip (0, 2000);
    CHECK (track.getClip (0).lengthSamples.load() == 2000);

    track.removeClip (0);
    CHECK (track.getNumClips() == 1);
    CHECK (track.getClip (0).startSamples.load() == 5000);
}

void testRoutingOffline()
{
    daw::AudioGraph graph;

    juce::MidiBuffer midi;
    juce::AudioBuffer<float> input (2, 128);
    juce::AudioBuffer<float> output (2, 128);
    input.clear();
    output.clear();

    const int numSamples = 128;
    float* inputPointers[2] = { input.getWritePointer (0), input.getWritePointer (1) };
    float* outputPointers[2] = { output.getWritePointer (0), output.getWritePointer (1) };

    graph.prepare (48000.0, numSamples);
    graph.reset();

    for (int block = 0; block < 64; ++block)
    {
        output.clear();

        for (int i = 0; i < numSamples; ++i)
        {
            inputPointers[0][i] = 0.25f * std::sin (2.0f * 3.14159265358979323846f * 440.0f * (block * numSamples + i) / 48000.0f);
            inputPointers[1][i] = inputPointers[0][i];
        }

        graph.processWithInput (const_cast<const float**> (inputPointers), outputPointers, 2, numSamples, midi);
    }

    const float masterPeak = peakOf (output);
    CHECK (masterPeak > 0.02f);

    graph.getTrack (0).setRouteTarget (0);

    for (int block = 0; block < 64; ++block)
    {
        output.clear();

        for (int i = 0; i < numSamples; ++i)
        {
            inputPointers[0][i] = 0.25f * std::sin (2.0f * 3.14159265358979323846f * 440.0f * (block * numSamples + i) / 48000.0f);
            inputPointers[1][i] = inputPointers[0][i];
        }

        graph.processWithInput (const_cast<const float**> (inputPointers), outputPointers, 2, numSamples, midi);
    }

    const float busPeak = peakOf (output);
    CHECK (busPeak > 0.02f);

    graph.getTrack (0).setMuted (true);

    for (int block = 0; block < 64; ++block)
    {
        output.clear();
        graph.processWithInput (const_cast<const float**> (inputPointers), outputPointers, 2, numSamples, midi);
    }

    CHECK (peakOf (output) < 0.001f);

    graph.getTrack (0).setMuted (false);
}

void testMidiEmission()
{
    daw::Track track;

    const std::uint8_t noteOn[] = { 0x90, 60, 100 };
    track.addMidiEvent (4800, noteOn, 3);

    juce::MidiBuffer midi;

    float outData[2][256] = {};
    float* out[2] = { outData[0], outData[1] };

    track.prepare (48000.0, 128);

    juce::MidiBuffer empty;

    for (int block = 0; block < 40; ++block)
    {
        track.process (nullptr, out, 2, 128, false, block * 128, {}, empty, true,
                       daw::RoutingBuffers { { out[0], out[1] }, {} });
    }

    CHECK (empty.getNumEvents() == 1);

    for (const auto metadata : empty)
    {
        const auto message = metadata.getMessage();
        CHECK (message.isNoteOn());
        CHECK (message.getNoteNumber() == 60);
        CHECK (message.getVelocity() == 100);
        CHECK (metadata.samplePosition == 64);
    }
}

void testProjectRoundTrip()
{
    daw::AudioGraph graph;

    graph.getTransport().setTempoBpm (140.0);
    graph.getTrack (0).setGainDb (4.5f);
    graph.getTrack (0).addClip (1000, 2000, 0);

    const std::uint8_t note[] = { 0x90, 64, 90 };
    graph.getTrack (0).addMidiEvent (2400, note, 3);

    const auto file = juce::File ("/tmp/kilo/linux-daw-unit-project.ldaw");

    const auto saveError = daw::Project::save (file, graph);
    CHECK (saveError.isEmpty());

    daw::AudioGraph loaded;

    const auto loadError = daw::Project::load (file, loaded);
    CHECK (loadError.isEmpty());

    CHECK (std::abs (loaded.getTransport().getTempoBpm() - 140.0) < 0.001);
    CHECK (std::abs (loaded.getTrack (0).getGainDb() - 4.5f) < 0.001f);
    CHECK (loaded.getTrack (0).getNumClips() == 1);
    CHECK (loaded.getTrack (0).getNumMidiEvents() == 1);
    CHECK (loaded.getTrack (0).getClip (0).startSamples.load() == 1000);

    // Corrupted project file handling.
    const auto corruptFile = juce::File ("/tmp/kilo/linux-daw-corrupt.ldaw");
    corruptFile.replaceWithText ("{ this is not valid json");
    CHECK (daw::Project::load (corruptFile, loaded).isNotEmpty());

    // Missing file handling.
    CHECK (daw::Project::load (juce::File ("/tmp/kilo/does-not-exist.ldaw"), loaded).isNotEmpty());
}
} // namespace

int main (int argc, char* argv[])
{
    juce::ignoreUnused (argc, argv);

    std::cout << "Running linux-daw unit tests..." << std::endl;

    testGainProcessor();
    testGateProcessor();
    testEqProcessor();
    testCompressorProcessor();
    testDistortionProcessor();
    testReverbProcessor();
    testTransport();
    testClipManipulation();
    testRoutingOffline();
    testMidiEmission();
    testProjectRoundTrip();

    std::cout << checks << " checks, " << failures << " failures" << std::endl;

    return failures == 0 ? 0 : 1;
}
