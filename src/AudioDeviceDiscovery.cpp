#include "AudioDeviceDiscovery.h"

#include <iostream>

#include <juce_audio_devices/juce_audio_devices.h>

namespace daw
{
namespace
{
void printChannelList (const juce::StringArray& channels)
{
    std::cout << channels.joinIntoString (", ");
}

void printDeviceDetails (juce::AudioIODeviceType& type,
                         const juce::String& outputDeviceName,
                         const juce::String& inputDeviceName)
{
    std::unique_ptr<juce::AudioIODevice> device (type.createDevice (outputDeviceName, inputDeviceName));

    if (device == nullptr)
    {
        std::cout << "      could not create device" << std::endl;
        return;
    }

    const auto sampleRates = device->getAvailableSampleRates();
    const auto bufferSizes = device->getAvailableBufferSizes();

    std::cout << "      available sample rates: ";
    if (sampleRates.isEmpty())
    {
        std::cout << "unknown";
    }
    else
    {
        for (int i = 0; i < sampleRates.size(); ++i)
        {
            if (i > 0)
                std::cout << ", ";

            std::cout << static_cast<int> (sampleRates.getReference (i));
        }
    }
    std::cout << std::endl;

    std::cout << "      available buffer sizes: ";
    if (bufferSizes.isEmpty())
    {
        std::cout << "unknown";
    }
    else
    {
        for (int i = 0; i < bufferSizes.size(); ++i)
        {
            if (i > 0)
                std::cout << ", ";

            std::cout << bufferSizes.getReference (i);
        }
    }
    std::cout << std::endl;

    std::cout << "      default buffer size: " << device->getDefaultBufferSize() << std::endl;

    juce::BigInteger inputChannels;
    juce::BigInteger outputChannels;
    inputChannels.setRange (0, 32, true);
    outputChannels.setRange (0, 32, true);

    const auto sampleRateToProbe = sampleRates.isEmpty() ? 44100.0 : sampleRates.getFirst();

    const auto error = device->open (inputChannels, outputChannels,
                                     sampleRateToProbe,
                                     device->getDefaultBufferSize());

    if (error.isEmpty())
    {
        const auto inNames  = device->getInputChannelNames();
        const auto outNames = device->getOutputChannelNames();

        std::cout << "      input channels (" << inNames.size() << "): ";
        printChannelList (inNames);
        std::cout << std::endl;

        std::cout << "      output channels (" << outNames.size() << "): ";
        printChannelList (outNames);
        std::cout << std::endl;

        device->close();
    }
    else
    {
        std::cout << "      channels: unavailable (device could not be opened: " << error << ")"
                  << std::endl;
    }
}
} // namespace

void printAudioDevices()
{
    juce::AudioDeviceManager deviceManager;
    juce::OwnedArray<juce::AudioIODeviceType> types;
    deviceManager.createAudioDeviceTypes (types);

    for (auto* type : types)
    {
        type->scanForDevices();

        std::cout << "Device type: " << type->getTypeName() << std::endl;

        const auto outputNames = type->getDeviceNames (false);
        const auto inputNames  = type->getDeviceNames (true);

        std::cout << "  Output devices (" << outputNames.size() << "):" << std::endl;
        for (const auto& name : outputNames)
            std::cout << "    - " << name << std::endl;

        std::cout << "  Input devices (" << inputNames.size() << "):" << std::endl;
        for (const auto& name : inputNames)
            std::cout << "    - " << name << std::endl;

        std::cout << "  Details:" << std::endl;
        juce::StringArray detailed;

        for (const auto& name : outputNames)
        {
            if (detailed.contains (name))
                continue;

            detailed.add (name);
            std::cout << "    " << name << ":" << std::endl;
            printDeviceDetails (*type, name, name);
        }

        for (const auto& name : inputNames)
        {
            if (detailed.contains (name))
                continue;

            detailed.add (name);
            std::cout << "    " << name << ":" << std::endl;
            printDeviceDetails (*type, name, name);
        }
    }
}
} // namespace daw
