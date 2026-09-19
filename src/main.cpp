#include <iostream>

#include <juce_core/juce_core.h>
#include <juce_audio_devices/juce_audio_devices.h>

#include "AudioDeviceDiscovery.h"
#include "AudioEngine.h"

namespace
{
juce::String findAI04Device (juce::AudioIODeviceType& type, bool wantInput)
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

int runAudioEngine()
{
    juce::AudioDeviceManager deviceManager;
    juce::OwnedArray<juce::AudioIODeviceType> types;
    deviceManager.createAudioDeviceTypes (types);

    juce::AudioIODeviceType* alsaType = nullptr;

    for (auto* type : types)
        if (type->getTypeName() == "ALSA")
            alsaType = type;

    if (alsaType == nullptr)
    {
        std::cout << "No ALSA device type found." << std::endl;
        return 1;
    }

    const auto inputDeviceName  = findAI04Device (*alsaType, true);
    const auto outputDeviceName = findAI04Device (*alsaType, false);

    if (inputDeviceName.isEmpty() || outputDeviceName.isEmpty())
    {
        std::cout << "AI-04 device not found." << std::endl;
        return 1;
    }

    std::cout << "Input device:  " << inputDeviceName << std::endl;
    std::cout << "Output device: " << outputDeviceName << std::endl;

    daw::AudioEngine engine;

    const auto error = engine.start (inputDeviceName, outputDeviceName, 48000.0, 128);

    if (error.isNotEmpty())
    {
        std::cout << "Failed to start audio: " << error << std::endl;
        return 1;
    }

    std::cout << "Audio running: sample rate " << engine.getSampleRate()
              << ", buffer size " << engine.getBufferSize() << std::endl;

    juce::Thread::sleep (3000);

    engine.stop();
    std::cout << "Audio stopped." << std::endl;

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
                             [&exitCode] (const juce::ArgumentList&)
                             {
                                 std::cout << "Application started." << std::endl;
                                 exitCode = runAudioEngine();
                             }});

    const auto result = app.findAndRunCommand (argc, argv);

    return exitCode != 0 ? exitCode : result;
}
