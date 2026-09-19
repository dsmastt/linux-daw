#include "Vst3HostProcessor.h"

namespace daw
{
juce::String Vst3HostProcessor::loadPlugin (const juce::PluginDescription& description,
                                           double sampleRateToUse,
                                           int bufferSizeToUse)
{
    unloadPlugin();

    juce::AudioPluginFormatManager formatManager;
    juce::addDefaultFormatsToManager (formatManager);

    const double rate = sampleRateToUse > 0.0 ? sampleRateToUse : 48000.0;
    const int buffer = bufferSizeToUse > 0 ? bufferSizeToUse : 512;

    juce::String error;
    auto plugin = formatManager.createPluginInstance (description, rate, buffer, error);

    if (plugin == nullptr)
        return error.isEmpty() ? juce::String ("Unknown error loading plugin") : error;

    instance = std::move (plugin);
    pluginName = instance->getName();
    pluginDescription = description;
    sampleRate = rate;
    bufferSize = buffer;

    loaded.store (true, std::memory_order_release);

    return {};
}

void Vst3HostProcessor::unloadPlugin() noexcept
{
    loaded.store (false, std::memory_order_release);
    bypassed.store (false, std::memory_order_relaxed);
    failed.store (false, std::memory_order_relaxed);
    instance.reset();
    pluginName.clear();
}

void Vst3HostProcessor::prepare (double sampleRateToUse, int bufferSizeToUse) noexcept
{
    sampleRate = sampleRateToUse;
    bufferSize = bufferSizeToUse;

    if (loaded.load() && instance != nullptr)
    {
        instance->prepareToPlay (sampleRate, bufferSize);
        instance->reset();
    }
}

void Vst3HostProcessor::reset() noexcept
{
    if (loaded.load() && instance != nullptr)
        instance->reset();
}

void Vst3HostProcessor::process (float* const* channels, int numChannels, int numSamples) noexcept
{
    if (!loaded.load() || instance == nullptr || bypassed.load() || failed.load())
        return;

    pluginBuffer.setDataToReferTo (channels, numChannels, numSamples);
    midiBuffer.clear();

    try
    {
        instance->processBlock (pluginBuffer, midiBuffer);
    }
    catch (...)
    {
        failed.store (true, std::memory_order_release);
    }
}

void Vst3HostProcessor::processWithMidi (float* const* channels, int numChannels, int numSamples, juce::MidiBuffer& midiEvents) noexcept
{
    if (!loaded.load() || instance == nullptr || bypassed.load() || failed.load())
        return;

    pluginBuffer.setDataToReferTo (channels, numChannels, numSamples);

    try
    {
        instance->processBlock (pluginBuffer, midiEvents);
    }
    catch (...)
    {
        failed.store (true, std::memory_order_release);
    }
}

std::vector<juce::PluginDescription> scanVst3Plugins (const juce::StringArray& paths)
{
    std::vector<juce::PluginDescription> result;

    juce::VST3PluginFormat format;
    juce::KnownPluginList knownList;

    for (const auto& path : paths)
    {
        if (!juce::File (path).isDirectory())
            continue;

        juce::PluginDirectoryScanner scanner (knownList, format, juce::FileSearchPath (path), true, juce::File(), true);

        juce::String name;

        while (scanner.scanNextFile (true, name))
        {
        }
    }

    const auto& types = knownList.getTypes();

    for (int i = 0; i < types.size(); ++i)
        result.push_back (types.getUnchecked (i));

    return result;
}
} // namespace daw
