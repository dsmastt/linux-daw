#include <iostream>

#include <cmath>
#include <cstring>
#include <unistd.h>

#include <alsa/asoundlib.h>

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

#include "AudioEngine.h"
#include "AudioGraph.h"
#include "Vst3HostProcessor.h"

namespace
{
bool sendMidiViaAlsa (const char* deviceName)
{
    snd_seq_t* seq = nullptr;

    if (snd_seq_open (&seq, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0)
        return false;

    snd_seq_set_client_name (seq, "linux-daw-test-sender");

    const int srcPort = snd_seq_create_simple_port (seq, "test-out",
                                                    SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
                                                    SND_SEQ_PORT_TYPE_APPLICATION);

    if (srcPort < 0)
    {
        snd_seq_close (seq);
        return false;
    }

    int dstClient = -1;
    int dstPort = -1;

    snd_seq_client_info_t* clientInfo = nullptr;
    snd_seq_client_info_alloca (&clientInfo);
    snd_seq_client_info_set_client (clientInfo, -1);

    while (snd_seq_query_next_client (seq, clientInfo) >= 0)
    {
        const int client = snd_seq_client_info_get_client (clientInfo);

        if (client == snd_seq_client_id (seq))
            continue;

        snd_seq_port_info_t* portInfo = nullptr;
        snd_seq_port_info_alloca (&portInfo);
        snd_seq_port_info_set_client (portInfo, client);
        snd_seq_port_info_set_port (portInfo, -1);

        while (snd_seq_query_next_port (seq, portInfo) >= 0)
        {
            const char* portName = snd_seq_port_info_get_name (portInfo);

            if (portName != nullptr
                && std::strcmp (portName, deviceName) == 0
                && (snd_seq_port_info_get_capability (portInfo) & SND_SEQ_PORT_CAP_SUBS_WRITE))
            {
                dstClient = client;
                dstPort = snd_seq_port_info_get_port (portInfo);
                break;
            }
        }

        if (dstClient >= 0)
            break;
    }

    if (dstClient < 0)
    {
        snd_seq_close (seq);
        return false;
    }

    snd_seq_connect_to (seq, srcPort, dstClient, dstPort);

    const auto sendEvent = [&] (int type, int param1, int param2)
    {
        snd_seq_event_t ev;
        snd_seq_ev_clear (&ev);

        if (type == SND_SEQ_EVENT_CONTROLLER)
        {
            ev.type = SND_SEQ_EVENT_CONTROLLER;
            ev.data.control.channel = 0;
            ev.data.control.param = param1;
            ev.data.control.value = param2;
        }
        else
        {
            ev.type = static_cast<unsigned char> (type);
            ev.data.note.channel = 0;
            ev.data.note.note = static_cast<unsigned char> (param1);
            ev.data.note.velocity = static_cast<unsigned char> (param2);
        }

        snd_seq_ev_set_source (&ev, srcPort);
        snd_seq_ev_set_subs (&ev);
        snd_seq_ev_set_direct (&ev);
        snd_seq_event_output (seq, &ev);
        snd_seq_drain_output (seq);
    };

    sendEvent (SND_SEQ_EVENT_NOTEON, 60, 100);
    usleep (50000);
    sendEvent (SND_SEQ_EVENT_CONTROLLER, 7, 64);
    usleep (50000);
    sendEvent (SND_SEQ_EVENT_NOTEOFF, 60, 0);
    usleep (50000);

    snd_seq_close (seq);
    return true;
}
juce::File createClipTestFile()
{
    const auto file = juce::File ("/tmp/kilo/linux-daw-clip-test.wav");

    if (auto stream = file.createOutputStream())
    {
        std::unique_ptr<juce::OutputStream> outputStream = std::move (stream);

        juce::WavAudioFormat format;

        const auto options = juce::AudioFormatWriter::Options{}
                                 .withSampleRate (48000.0)
                                 .withNumChannels (1)
                                 .withBitsPerSample (16)
                                 .withQualityOptionIndex (0);

        if (auto writer = format.createWriterFor (outputStream, options))
        {
            constexpr int numSamples = 48000 * 2;

            juce::AudioBuffer<float> buffer (1, numSamples);
            auto* data = buffer.getWritePointer (0);

            for (int i = 0; i < numSamples; ++i)
                data[i] = 0.5f * std::sin (2.0f * 3.14159265358979323846f * 440.0f * i / 48000.0f);

            const float* pointers[1] = { data };
            writer->writeFromFloatArrays (pointers, 1, numSamples);
            writer->flush();
        }
    }

    return file;
}
juce::AudioIODeviceType* findAlsaType (juce::OwnedArray<juce::AudioIODeviceType>& types)
{
    for (auto* type : types)
        if (type->getTypeName() == "ALSA")
            return type;

    return nullptr;
}

juce::String resolveAI04 (juce::AudioIODeviceType& type, bool wantInput)
{
    type.scanForDevices();

    for (const auto& name : type.getDeviceNames (wantInput))
        if (name.contains ("AI-04") && name.contains ("Front output / input"))
            return name;

    for (const auto& name : type.getDeviceNames (wantInput))
        if (name.contains ("AI-04"))
            return name;

    return {};
}

int runTest (const juce::ArgumentList& args)
{
    juce::StringArray paths;

    if (const auto value = args.getValueForOption ("--plugin-path"); value.isNotEmpty())
        paths.add (value);
    else
        paths.add ("build/linux-daw-test-plugin_artefacts/VST3");

    paths.add (juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile (".vst3").getFullPathName());

    const auto plugins = daw::scanVst3Plugins (paths);

    std::cout << "Found " << plugins.size() << " VST3 plugins" << std::endl;

    for (const auto& plugin : plugins)
        std::cout << "  - " << plugin.name << " (" << plugin.fileOrIdentifier << ")" << std::endl;

    if (plugins.empty())
    {
        std::cout << "No VST3 plugins found." << std::endl;
        return 1;
    }

    daw::AudioEngine engine;
    auto& graph = engine.getAudioGraph();

    const auto loadError = graph.loadVst3Plugin (plugins.front(), 48000.0, 128);

    if (loadError.isNotEmpty())
    {
        std::cout << "Failed to load plugin: " << loadError << std::endl;
        return 1;
    }

    std::cout << "Loaded plugin: " << graph.getVst3PluginName() << std::endl;

    if (auto* instance = graph.getVst3PluginInstance())
    {
        const auto& parameters = instance->getParameters();

        if (!parameters.isEmpty())
        {
            parameters.getUnchecked (0)->setValue (0.25f);
            std::cout << "Set parameter '" << parameters.getUnchecked (0)->getName (64)
                      << "' to " << parameters.getUnchecked (0)->getValue() << std::endl;
        }
    }

    graph.setVst3PluginBypassed (true);
    std::cout << "Bypassed: " << (graph.isVst3PluginBypassed() ? "yes" : "no") << std::endl;
    graph.setVst3PluginBypassed (false);

    const auto clipFile = createClipTestFile();
    const auto clipLoadError = graph.loadPlaybackFile (clipFile);

    if (clipLoadError.isNotEmpty())
    {
        std::cout << "Failed to load clip file: " << clipLoadError << std::endl;
        graph.unloadVst3Plugin();
        return 1;
    }

    std::cout << "Clip file loaded" << std::endl;

    juce::AudioDeviceManager deviceManager;
    juce::OwnedArray<juce::AudioIODeviceType> types;
    deviceManager.createAudioDeviceTypes (types);

    auto* alsaType = findAlsaType (types);

    if (alsaType == nullptr)
    {
        std::cout << "No ALSA device type found." << std::endl;
        graph.unloadVst3Plugin();
        return 1;
    }

    const auto inputDeviceName = resolveAI04 (*alsaType, true);
    const auto outputDeviceName = resolveAI04 (*alsaType, false);

    if (inputDeviceName.isEmpty() || outputDeviceName.isEmpty())
    {
        std::cout << "AI-04 device not found." << std::endl;
        graph.unloadVst3Plugin();
        return 1;
    }

    const auto startError = engine.start (inputDeviceName, outputDeviceName, 48000.0, 128);

    if (startError.isNotEmpty())
    {
        std::cout << "Failed to start audio: " << startError << std::endl;
        graph.unloadVst3Plugin();
        return 1;
    }

    std::cout << "Audio running with plugin in the chain..." << std::endl;

    graph.playTransport();

    juce::Thread::sleep (2000);

    const auto transportPosition = graph.getTransport().getPositionSamples();

    std::cout << "Transport position after 2 s: " << transportPosition << " samples" << std::endl;

    graph.stopTransport();

    const auto callbacks = engine.getNumCallbacks();
    const auto xruns = engine.getXRunCount();

    std::cout << "Processed " << callbacks << " callbacks, xruns: " << xruns << std::endl;

    const auto expectedPosition = 48000LL * 2;
    const auto transportOk = transportPosition > expectedPosition - 2000 && transportPosition < expectedPosition + 2000;

    // Timeline clip playback test: add a clip, and verify audio reaches the
    // output while the transport plays.
    const auto clipFrames = graph.getPlaybackLengthFrames();

    if (clipFrames <= 0)
    {
        std::cout << "Clip file was not prepared" << std::endl;
        engine.stop();
        graph.unloadVst3Plugin();
        return 1;
    }

    graph.getTrack (0).addClip (0, clipFrames, 0);

    std::cout << "Clip added: " << clipFrames << " frames" << std::endl;

    graph.playTransport();
    juce::Thread::sleep (1000);
    const float clipPeak = engine.consumeOutputPeak();
    std::cout << "Output peak during clip playback: " << clipPeak << std::endl;
    graph.stopTransport();

    graph.getTrack (0).moveClip (0, 24000);
    graph.playTransport();
    juce::Thread::sleep (1000);
    const float movedPeak = engine.consumeOutputPeak();
    std::cout << "Output peak after moving clip: " << movedPeak << std::endl;
    graph.stopTransport();

    graph.getTrack (0).resizeClip (0, 24000);
    graph.playTransport();
    juce::Thread::sleep (1000);
    const float resizedPeak = engine.consumeOutputPeak();
    std::cout << "Output peak after resizing clip: " << resizedPeak << std::endl;
    graph.stopTransport();

    const bool clipsOk = clipPeak > 0.2f && movedPeak > 0.2f && resizedPeak > 0.2f;

    // MIDI round-trip test via virtual devices.
    const auto midiOpenError = engine.openVirtualMidiDevice ("linux-daw-test-midi");

    if (midiOpenError.isNotEmpty())
    {
        std::cout << "Failed to open virtual MIDI input: " << midiOpenError << std::endl;
        engine.stop();
        graph.unloadVst3Plugin();
        return 1;
    }

    std::cout << "Virtual MIDI input open" << std::endl;

    if (!sendMidiViaAlsa ("linux-daw-test-midi"))
    {
        std::cout << "Failed to send MIDI via ALSA" << std::endl;
        engine.stop();
        graph.unloadVst3Plugin();
        return 1;
    }

    std::cout << "MIDI events sent via ALSA" << std::endl;

    juce::Thread::sleep (100);

    const auto& midiHandler = engine.getMidiHandler();
    const auto midiOk = midiHandler.getNoteOnCount() == 1
                        && midiHandler.getNoteOffCount() == 1
                        && midiHandler.getCcCount() == 1
                        && midiHandler.getLastNoteNumber() == 60
                        && midiHandler.getLastVelocity() == 100
                        && midiHandler.getLastControllerNumber() == 7
                        && midiHandler.getLastControllerValue() == 64;

    std::cout << "MIDI: noteOns " << midiHandler.getNoteOnCount()
              << ", noteOffs " << midiHandler.getNoteOffCount()
              << ", ccs " << midiHandler.getCcCount()
              << ", last note " << midiHandler.getLastNoteNumber()
              << ", last cc " << midiHandler.getLastControllerNumber()
              << "=" << midiHandler.getLastControllerValue() << std::endl;

    engine.closeMidiDevice();

    if (callbacks == 0 || xruns != 0 || !transportOk || !clipsOk || !midiOk)
    {
        std::cout << "VST3 host test FAILED" << std::endl;
        return 1;
    }

    std::cout << "VST3 host test passed" << std::endl;
    return 0;
}
} // namespace

int main (int argc, char* argv[])
{
    juce::ConsoleApplication app;

    app.addHelpCommand ("--help|-h", "linux-daw-vst3-test usage:", true);

    int exitCode = 0;

    app.addDefaultCommand ({ "",
                             "",
                             "Runs the VST3 host test",
                             "",
                             [&exitCode] (const juce::ArgumentList& args)
                             {
                                 exitCode = runTest (args);
                             }});

    const auto result = app.findAndRunCommand (argc, argv);

    return exitCode != 0 ? exitCode : result;
}
