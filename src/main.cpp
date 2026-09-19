#include <chrono>
#include <iostream>

#include <juce_core/juce_core.h>
#include <juce_audio_devices/juce_audio_devices.h>

#include "AudioDeviceDiscovery.h"
#include "AudioEngine.h"

namespace
{
juce::AudioIODeviceType* findAlsaType (juce::OwnedArray<juce::AudioIODeviceType>& types)
{
    for (auto* type : types)
        if (type->getTypeName() == "ALSA")
            return type;

    return nullptr;
}

juce::String resolveDeviceName (juce::AudioIODeviceType& type,
                                const juce::String& requested,
                                bool wantInput)
{
    type.scanForDevices();
    const auto names = type.getDeviceNames (wantInput);

    if (requested.isEmpty())
    {
        for (const auto& name : names)
            if (name.contains ("AI-04") && name.contains ("Front output / input"))
                return name;

        for (const auto& name : names)
            if (name.contains ("AI-04"))
                return name;

        return {};
    }

    for (const auto& name : names)
        if (name.equalsIgnoreCase (requested))
            return name;

    for (const auto& name : names)
        if (name.containsIgnoreCase (requested))
            return name;

    return {};
}

int runAudioEngine (const juce::ArgumentList& args)
{
    juce::AudioDeviceManager deviceManager;
    juce::OwnedArray<juce::AudioIODeviceType> types;
    deviceManager.createAudioDeviceTypes (types);

    auto* alsaType = findAlsaType (types);

    if (alsaType == nullptr)
    {
        std::cout << "No ALSA device type found." << std::endl;
        return 1;
    }

    const auto inputDeviceName  = resolveDeviceName (*alsaType, args.getValueForOption ("--input-device"), true);
    const auto outputDeviceName = resolveDeviceName (*alsaType, args.getValueForOption ("--output-device"), false);

    if (inputDeviceName.isEmpty() || outputDeviceName.isEmpty())
    {
        std::cout << "Requested audio device not found." << std::endl;
        std::cout << "Use --list-devices to see the available devices." << std::endl;
        return 1;
    }

    double sampleRate = 48000.0;

    if (const auto value = args.getValueForOption ("--sample-rate"); value.isNotEmpty())
        sampleRate = value.getDoubleValue();

    int bufferSize = 128;

    if (const auto value = args.getValueForOption ("--buffer-size"); value.isNotEmpty())
        bufferSize = value.getIntValue();

    double durationSeconds = 0.0;

    if (const auto value = args.getValueForOption ("--duration"); value.isNotEmpty())
        durationSeconds = value.getDoubleValue();

    float gainDb = 0.0f;

    if (const auto value = args.getValueForOption ("--gain-db"); value.isNotEmpty())
        gainDb = value.getFloatValue();

    float gateThresholdDb = -60.0f;

    if (const auto value = args.getValueForOption ("--gate-threshold-db"); value.isNotEmpty())
        gateThresholdDb = value.getFloatValue();

    float gateAttackMs = 1.0f;

    if (const auto value = args.getValueForOption ("--gate-attack-ms"); value.isNotEmpty())
        gateAttackMs = value.getFloatValue();

    float gateReleaseMs = 100.0f;

    if (const auto value = args.getValueForOption ("--gate-release-ms"); value.isNotEmpty())
        gateReleaseMs = value.getFloatValue();

    juce::StringArray eqBandSpecs;

    if (const auto value = args.getValueForOption ("--eq"); value.isNotEmpty())
        eqBandSpecs = juce::StringArray::fromTokens (value, ",", "\"");

    float compThresholdDb = -10.0f;
    float compRatio = 4.0f;
    float compAttackMs = 5.0f;
    float compReleaseMs = 100.0f;
    float compMakeupDb = 0.0f;

    if (const auto value = args.getValueForOption ("--comp-threshold-db"); value.isNotEmpty())
        compThresholdDb = value.getFloatValue();

    if (const auto value = args.getValueForOption ("--comp-ratio"); value.isNotEmpty())
        compRatio = value.getFloatValue();

    if (const auto value = args.getValueForOption ("--comp-attack-ms"); value.isNotEmpty())
        compAttackMs = value.getFloatValue();

    if (const auto value = args.getValueForOption ("--comp-release-ms"); value.isNotEmpty())
        compReleaseMs = value.getFloatValue();

    if (const auto value = args.getValueForOption ("--comp-makeup-db"); value.isNotEmpty())
        compMakeupDb = value.getFloatValue();

    float distDriveDb = 12.0f;
    float distTone = 0.5f;
    int distMode = 0;

    if (const auto value = args.getValueForOption ("--dist-drive-db"); value.isNotEmpty())
        distDriveDb = value.getFloatValue();

    if (const auto value = args.getValueForOption ("--dist-tone"); value.isNotEmpty())
        distTone = value.getFloatValue();

    if (const auto value = args.getValueForOption ("--dist-mode"); value.isNotEmpty())
        distMode = value.getIntValue();

    float reverbWetDry = 0.2f;
    float reverbDecay = 0.5f;
    float reverbSize = 0.5f;

    if (const auto value = args.getValueForOption ("--reverb-wet-dry"); value.isNotEmpty())
        reverbWetDry = value.getFloatValue();

    if (const auto value = args.getValueForOption ("--reverb-decay"); value.isNotEmpty())
        reverbDecay = value.getFloatValue();

    if (const auto value = args.getValueForOption ("--reverb-size"); value.isNotEmpty())
        reverbSize = value.getFloatValue();

    std::cout << "Input device:  " << inputDeviceName << std::endl;
    std::cout << "Output device: " << outputDeviceName << std::endl;
    std::cout << "Requested sample rate: " << static_cast<int> (sampleRate) << std::endl;
    std::cout << "Requested buffer size: " << bufferSize << std::endl;
    std::cout << "Gain: " << gainDb << " dB" << std::endl;
    std::cout << "Gate: threshold " << gateThresholdDb << " dB, attack " << gateAttackMs
              << " ms, release " << gateReleaseMs << " ms" << std::endl;

    daw::AudioEngine engine;

    auto& graph = engine.getAudioGraph();

    graph.setGainDb (gainDb);
    graph.setGateThresholdDb (gateThresholdDb);
    graph.setGateAttackMs (gateAttackMs);
    graph.setGateReleaseMs (gateReleaseMs);

    for (int band = 0; band < eqBandSpecs.size() && band < graph.getNumEqBands(); ++band)
    {
        const auto parts = juce::StringArray::fromTokens (eqBandSpecs[band], ":", "\"");

        if (parts.size() >= 3)
            graph.setEqBandParameters (band,
                                       parts[0].getDoubleValue(),
                                       parts[1].getFloatValue(),
                                       parts[2].getFloatValue());
    }

    for (int band = 0; band < graph.getNumEqBands(); ++band)
        if (const auto spec = band < eqBandSpecs.size() ? eqBandSpecs[band] : juce::String(); spec.isNotEmpty())
            std::cout << "EQ band " << band + 1 << ": " << spec << std::endl;

    graph.setCompressorThresholdDb (compThresholdDb);
    graph.setCompressorRatio (compRatio);
    graph.setCompressorAttackMs (compAttackMs);
    graph.setCompressorReleaseMs (compReleaseMs);
    graph.setCompressorMakeupGainDb (compMakeupDb);

    std::cout << "Compressor: threshold " << compThresholdDb << " dB, ratio " << compRatio
              << ":1, attack " << compAttackMs << " ms, release " << compReleaseMs
              << " ms, makeup " << compMakeupDb << " dB" << std::endl;

    graph.setDistortionDriveDb (distDriveDb);
    graph.setDistortionTone (distTone);
    graph.setDistortionMode (static_cast<daw::DistortionMode> (distMode));

    graph.setReverbWetDry (reverbWetDry);
    graph.setReverbDecay (reverbDecay);
    graph.setReverbSize (reverbSize);

    std::cout << "Reverb: wet/dry " << reverbWetDry << ", decay " << reverbDecay
              << ", size " << reverbSize << std::endl;

    std::cout << "Distortion: mode " << distMode << ", drive " << distDriveDb
              << " dB, tone " << distTone << std::endl;

    juce::String irPath;

    if (const auto value = args.getValueForOption ("--ir"); value.isNotEmpty())
        irPath = value;

    if (irPath.isNotEmpty())
    {
        const auto error = graph.loadImpulseResponse (juce::File (irPath));

        if (error.isNotEmpty())
        {
            std::cout << "Failed to load impulse response: " << error << std::endl;
            return 1;
        }

        std::cout << "IR: " << irPath << " (" << graph.getImpulseResponseLength() << " samples)" << std::endl;
    }

    juce::String recordPath;

    if (const auto value = args.getValueForOption ("--record"); value.isNotEmpty())
        recordPath = value;

    juce::String playPath;
    double seekSeconds = -1.0;

    if (const auto value = args.getValueForOption ("--play"); value.isNotEmpty())
        playPath = value;

    if (const auto value = args.getValueForOption ("--seek"); value.isNotEmpty())
        seekSeconds = value.getDoubleValue();

    if (playPath.isNotEmpty())
    {
        const auto loadError = graph.loadPlaybackFile (juce::File (playPath));

        if (loadError.isNotEmpty())
        {
            std::cout << "Failed to load playback file: " << loadError << std::endl;
            return 1;
        }

        if (seekSeconds >= 0.0)
            graph.setPlaybackPositionSeconds (seekSeconds);
    }

    const auto error = engine.start (inputDeviceName, outputDeviceName, sampleRate, bufferSize);

    if (error.isNotEmpty())
    {
        std::cout << "Failed to start audio: " << error << std::endl;
        return 1;
    }

    if (recordPath.isNotEmpty())
    {
        const auto recordError = engine.startRecording (juce::File (recordPath));

        if (recordError.isNotEmpty())
        {
            std::cout << "Failed to start recording: " << recordError << std::endl;
            engine.stop();
            return 1;
        }

        std::cout << "Recording to: " << recordPath << std::endl;
    }

    if (playPath.isNotEmpty())
    {
        graph.playPlayback();

        std::cout << "Playback file: " << playPath << " ("
                  << graph.getPlaybackLengthSeconds() << " s)";

        if (seekSeconds >= 0.0)
            std::cout << ", seek " << seekSeconds << " s";

        std::cout << std::endl;
    }

    const auto totalLatencySamples = engine.getInputLatencySamples() + engine.getOutputLatencySamples();
    const auto latencyMs = (1000.0 * totalLatencySamples) / engine.getSampleRate();

    std::cout << "Audio running: sample rate " << engine.getSampleRate()
              << ", buffer size " << engine.getBufferSize()
              << ", input channels " << engine.getNumInputChannels()
              << ", output channels " << engine.getNumOutputChannels()
              << ", processors " << graph.getNumProcessors() << std::endl;
    std::cout << "Latency: " << engine.getInputLatencySamples() << " in + "
              << engine.getOutputLatencySamples() << " out samples = " << latencyMs << " ms" << std::endl;

    const bool debug = args.containsOption ("--debug");

    const auto reportStats = [&engine, &graph]
    {
        const auto callbacks = engine.getNumCallbacks();
        const auto avgCallbackMs = callbacks > 0 ? engine.getTotalCallbackTimeMs() / callbacks : 0.0;
        const auto periodMs = (1000.0 * engine.getBufferSize()) / engine.getSampleRate();
        const auto duty = periodMs > 0.0 ? (100.0 * avgCallbackMs / periodMs) : 0.0;

        std::cout << "Stats: callbacks " << callbacks
                  << ", avg " << avgCallbackMs << " ms, max " << engine.getMaxCallbackTimeMs()
                  << " ms, audio thread duty " << duty << " %, xruns " << engine.getXRunCount();

        if (graph.isPlaybackPlaying())
            std::cout << ", playback " << graph.getPlaybackPositionSeconds()
                      << " / " << graph.getPlaybackLengthSeconds() << " s";

        std::cout << std::endl;
    };

    const auto deadline = durationSeconds > 0.0
                            ? std::chrono::steady_clock::now() + std::chrono::milliseconds (static_cast<int> (durationSeconds * 1000))
                            : std::chrono::steady_clock::time_point::max();

    while (engine.isRunning() && std::chrono::steady_clock::now() < deadline)
    {
        juce::Thread::sleep (debug ? 1000 : 200);

        if (debug)
            reportStats();
    }

    reportStats();

    if (engine.isRecording())
    {
        engine.stopRecording();
        std::cout << "Recording finished: " << recordPath << " ("
                  << engine.getFramesWritten() << " frames, "
                  << engine.getDroppedRecordingBlocks() << " dropped blocks)" << std::endl;
    }

    if (engine.isRunning())
    {
        engine.stop();
        std::cout << "Audio stopped." << std::endl;
    }

    return 0;
}
} // namespace

int main (int argc, char* argv[])
{
    juce::ConsoleApplication app;

    app.addHelpCommand ("--help|-h", "linux-daw usage:", true);
    app.addVersionCommand ("--version|-v", "linux-daw 0.1.0");

    app.addCommand ({ "--list-devices|-l",
                      "",
                      "Lists available audio devices",
                      "",
                      [] (const juce::ArgumentList&)
                      {
                          daw::printAudioDevices();
                      }});

    int exitCode = 0;

    app.addDefaultCommand ({ "",
                             "",
                             "Starts the application",
                             "",
                             [&exitCode] (const juce::ArgumentList& args)
                             {
                                 std::cout << "Application started." << std::endl;
                                 exitCode = runAudioEngine (args);
                             }});

    const auto result = app.findAndRunCommand (argc, argv);

    return exitCode != 0 ? exitCode : result;
}
