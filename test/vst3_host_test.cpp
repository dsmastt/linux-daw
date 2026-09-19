#include <iostream>

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

#include "AudioEngine.h"
#include "AudioGraph.h"
#include "Vst3HostProcessor.h"

namespace
{
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

    juce::Thread::sleep (2000);

    const auto callbacks = engine.getNumCallbacks();
    const auto xruns = engine.getXRunCount();

    std::cout << "Processed " << callbacks << " callbacks, xruns: " << xruns << std::endl;

    engine.stop();
    graph.unloadVst3Plugin();

    std::cout << "Plugin unloaded" << std::endl;

    if (callbacks == 0 || xruns != 0)
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
