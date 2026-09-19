#include "PluginProcessor.h"

void TestPluginProcessor::prepareToPlay (double, int)
{
}

void TestPluginProcessor::releaseResources()
{
}

bool TestPluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::disabled()
        && layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void TestPluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    const float currentGain = gain.load (std::memory_order_relaxed);

    if (juce::approximatelyEqual (currentGain, 1.0f))
        return;

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        buffer.applyGain (channel, 0, buffer.getNumSamples(), currentGain);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TestPluginProcessor();
}
