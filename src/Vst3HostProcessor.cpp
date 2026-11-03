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

    double rate = sampleRateToUse > 0.0 ? sampleRateToUse : 48000.0;
    int buffer = bufferSizeToUse > 0 ? bufferSizeToUse : 512;

    // If the engine has already been prepared, use its actual format. This
    // matters when a project is loaded while audio is running, since the
    // project itself does not know the device's current sample rate.
    {
        const juce::SpinLock::ScopedLockType lock (instanceLock);

        if (sampleRate > 0.0)
            rate = sampleRate;

        if (bufferSize > 0)
            buffer = bufferSize;
    }

    juce::String error;
    auto plugin = formatManager.createPluginInstance (description, rate, buffer, error);

    if (plugin == nullptr)
        return error.isEmpty() ? juce::String ("Unknown error loading plugin") : error;

    // The plugin must be prepared before the audio thread can see it,
    // otherwise its processBlock() calls are ignored by JUCE.
    try
    {
        plugin->prepareToPlay (rate, buffer);
        plugin->reset();
    }
    catch (...)
    {
        return "Could not prepare plugin for playback";
    }

    {
        const juce::SpinLock::ScopedLockType lock (instanceLock);

        instance = std::move (plugin);
        pluginName = instance->getName();
        pluginDescription = description;
        sampleRate = rate;
        bufferSize = buffer;

        loaded.store (true, std::memory_order_release);
    }

    return {};
}

void Vst3HostProcessor::unloadPlugin() noexcept
{
    std::unique_ptr<juce::AudioPluginInstance> oldInstance;

    {
        const juce::SpinLock::ScopedLockType lock (instanceLock);

        loaded.store (false, std::memory_order_release);
        setBypassed (false);
        failed.store (false, std::memory_order_relaxed);

        oldInstance = std::move (instance);
        pluginName.clear();
    }

    // Destroy the instance outside the lock; plugin teardown can be slow.
}

void Vst3HostProcessor::prepare (double sampleRateToUse, int bufferSizeToUse) noexcept
{
    const juce::SpinLock::ScopedLockType lock (instanceLock);

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
    const juce::SpinLock::ScopedLockType lock (instanceLock);

    if (loaded.load() && instance != nullptr)
        instance->reset();
}

void Vst3HostProcessor::process (float* const* channels, int numChannels, int numSamples) noexcept
{
    if (!loaded.load (std::memory_order_acquire))
        return;

    const juce::SpinLock::ScopedTryLockType lock (instanceLock);

    if (!lock.isLocked())
        return;

    if (!loaded.load (std::memory_order_relaxed) || instance == nullptr
        || isBypassed() || failed.load())
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
    if (!loaded.load (std::memory_order_acquire))
        return;

    const juce::SpinLock::ScopedTryLockType lock (instanceLock);

    if (!lock.isLocked())
        return;

    if (!loaded.load (std::memory_order_relaxed) || instance == nullptr
        || isBypassed() || failed.load())
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

std::vector<juce::PluginDescription> scanVst3Plugins (const juce::StringArray& paths,
                                                      const juce::File& deadMansPedalFile)
{
    std::vector<juce::PluginDescription> result;

    juce::VST3PluginFormat format;
    juce::KnownPluginList knownList;

    for (const auto& path : paths)
    {
        if (!juce::File (path).isDirectory())
            continue;

        juce::PluginDirectoryScanner scanner (knownList, format, juce::FileSearchPath (path), true,
                                              deadMansPedalFile, true);

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
