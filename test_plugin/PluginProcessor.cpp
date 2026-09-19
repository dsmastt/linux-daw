#include "PluginProcessor.h"

namespace
{
class TestPluginEditor final : public juce::AudioProcessorEditor
{
public:
    explicit TestPluginEditor (TestPluginProcessor& processorToUse)
        : juce::AudioProcessorEditor (processorToUse)
    {
        setSize (300, 200);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::darkgrey);
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions{}.withHeight (18.0f));
        g.drawText ("Linux DAW Test Plugin", getLocalBounds(), juce::Justification::centred, false);
    }
};
} // namespace

TestPluginProcessor::TestPluginProcessor()
{
    addParameter (gainParameter = new juce::AudioParameterFloat ("gain", "Gain", 0.0f, 2.0f, 1.0f));
}

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
    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();

        if (message.isNoteOn() && gainParameter != nullptr)
            gainParameter->setValueNotifyingHost (static_cast<float> (message.getVelocity()) / 127.0f);
    }

    const float currentGain = gainParameter != nullptr ? gainParameter->get() : 1.0f;

    if (juce::approximatelyEqual (currentGain, 1.0f))
        return;

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        buffer.applyGain (channel, 0, buffer.getNumSamples(), currentGain);
}

juce::AudioProcessorEditor* TestPluginProcessor::createEditor()
{
    return new TestPluginEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TestPluginProcessor();
}
