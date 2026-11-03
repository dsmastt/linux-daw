#include "Preset.h"

#include <algorithm>

#include <juce_audio_processors/juce_audio_processors.h>

namespace daw
{
namespace
{
double readNumber (const juce::var& value, double defaultValue)
{
    if (value.isVoid())
        return defaultValue;

    if (value.isDouble() || value.isInt() || value.isInt64() || value.isBool())
        return static_cast<double> (value);

    return defaultValue;
}

double readNumber (const juce::var& object, const char* property, double defaultValue)
{
    if (! object.isObject())
        return defaultValue;

    return readNumber (object[property], defaultValue);
}

} // namespace

juce::File Preset::getPresetDirectory()
{
    auto directory = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                         .getChildFile ("linux-daw")
                         .getChildFile ("presets");

    directory.createDirectory();
    return directory;
}

juce::Array<juce::File> Preset::listPresets()
{
    auto files = getPresetDirectory().findChildFiles (juce::File::findFiles, false, "*.ldpreset");

    std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b)
    {
        return a.getFileName().compareIgnoreCase (b.getFileName()) < 0;
    });

    return files;
}

juce::String Preset::save (const juce::File& file, const AudioGraph& graph)
{
    const auto& track = graph.getTrack (0);

    juce::DynamicObject::Ptr root = new juce::DynamicObject();
    root->setProperty ("format", "linux-daw-dsp-preset");
    root->setProperty ("version", 1);

    root->setProperty ("gainDb", track.getGainDb());

    juce::DynamicObject::Ptr gate = new juce::DynamicObject();
    gate->setProperty ("thresholdDb", track.getGateThresholdDb());
    gate->setProperty ("attackMs", track.getGateAttackMs());
    gate->setProperty ("releaseMs", track.getGateReleaseMs());
    root->setProperty ("gate", juce::var (gate.get()));

    juce::Array<juce::var> eqBands;

    for (int band = 0; band < track.getNumEqBands(); ++band)
    {
        juce::DynamicObject::Ptr eqBand = new juce::DynamicObject();
        eqBand->setProperty ("frequency", track.getEqBandFrequency (band));
        eqBand->setProperty ("gainDb", track.getEqBandGain (band));
        eqBand->setProperty ("q", track.getEqBandQ (band));
        eqBands.add (juce::var (eqBand.get()));
    }

    root->setProperty ("eqBands", eqBands);

    juce::DynamicObject::Ptr compressor = new juce::DynamicObject();
    compressor->setProperty ("thresholdDb", track.getCompressorThresholdDb());
    compressor->setProperty ("ratio", track.getCompressorRatio());
    compressor->setProperty ("attackMs", track.getCompressorAttackMs());
    compressor->setProperty ("releaseMs", track.getCompressorReleaseMs());
    compressor->setProperty ("makeupDb", track.getCompressorMakeupGainDb());
    root->setProperty ("compressor", juce::var (compressor.get()));

    juce::DynamicObject::Ptr distortion = new juce::DynamicObject();
    distortion->setProperty ("driveDb", track.getDistortionDriveDb());
    distortion->setProperty ("tone", track.getDistortionTone());
    distortion->setProperty ("mode", static_cast<int> (track.getDistortionMode()));
    root->setProperty ("distortion", juce::var (distortion.get()));

    juce::DynamicObject::Ptr reverb = new juce::DynamicObject();
    reverb->setProperty ("wetDry", track.getReverbWetDry());
    reverb->setProperty ("decay", track.getReverbDecay());
    reverb->setProperty ("size", track.getReverbSize());
    root->setProperty ("reverb", juce::var (reverb.get()));

    root->setProperty ("irFile", track.getImpulseResponseFilePath());

    juce::Array<juce::var> bypasses;

    for (int slot = 0; slot < static_cast<int> (ProcessorSlot::count); ++slot)
        bypasses.add (track.isProcessorBypassed (static_cast<ProcessorSlot> (slot)));

    root->setProperty ("bypasses", bypasses);

    if (track.isVst3PluginLoaded())
    {
        const auto description = track.getVst3PluginDescription();

        if (auto xml = description.createXml())
        {
            juce::DynamicObject::Ptr plugin = new juce::DynamicObject();
            plugin->setProperty ("name", description.name);
            plugin->setProperty ("descriptionXml", xml->toString());

            juce::Array<juce::var> parameterValues;

            if (auto* instance = track.getVst3PluginInstance())
            {
                for (auto* parameter : instance->getParameters())
                    if (parameter != nullptr)
                        parameterValues.add (parameter->getValue());

                juce::MemoryBlock state;
                instance->getStateInformation (state);

                if (state.getSize() > 0)
                    plugin->setProperty ("state", state.toBase64Encoding());
            }

            plugin->setProperty ("parameters", parameterValues);
            root->setProperty ("plugin", juce::var (plugin.get()));
        }
    }

    if (! file.getParentDirectory().createDirectory())
        return "Could not create preset directory";

    if (! file.replaceWithText (juce::JSON::toString (juce::var (root.get()), true)))
        return "Could not write preset file";

    return {};
}

juce::String Preset::load (const juce::File& file, AudioGraph& graph)
{
    if (! file.existsAsFile())
        return "Preset file not found";

    const auto root = juce::JSON::parse (file.loadFileAsString());

    if (! root.isObject() || root["format"].toString() != "linux-daw-dsp-preset")
        return "Not a linux-daw DSP preset";

    auto& track = graph.getTrack (0);

    track.setGainDb (static_cast<float> (readNumber (root, "gainDb", 0.0)));

    if (const auto gate = root["gate"]; gate.isObject())
    {
        track.setGateThresholdDb (static_cast<float> (readNumber (gate, "thresholdDb", -60.0)));
        track.setGateAttackMs (static_cast<float> (readNumber (gate, "attackMs", 1.0)));
        track.setGateReleaseMs (static_cast<float> (readNumber (gate, "releaseMs", 100.0)));
    }

    if (const auto eqBands = root["eqBands"]; eqBands.isArray())
    {
        for (int band = 0; band < eqBands.size() && band < track.getNumEqBands(); ++band)
        {
            const auto eqBand = eqBands[band];
            track.setEqBandParameters (band,
                                       readNumber (eqBand, "frequency", 1000.0),
                                       static_cast<float> (readNumber (eqBand, "gainDb", 0.0)),
                                       static_cast<float> (readNumber (eqBand, "q", 0.7)));
        }
    }

    if (const auto compressor = root["compressor"]; compressor.isObject())
    {
        track.setCompressorThresholdDb (static_cast<float> (readNumber (compressor, "thresholdDb", -10.0)));
        track.setCompressorRatio (static_cast<float> (readNumber (compressor, "ratio", 4.0)));
        track.setCompressorAttackMs (static_cast<float> (readNumber (compressor, "attackMs", 5.0)));
        track.setCompressorReleaseMs (static_cast<float> (readNumber (compressor, "releaseMs", 100.0)));
        track.setCompressorMakeupGainDb (static_cast<float> (readNumber (compressor, "makeupDb", 0.0)));
    }

    if (const auto distortion = root["distortion"]; distortion.isObject())
    {
        track.setDistortionDriveDb (static_cast<float> (readNumber (distortion, "driveDb", 12.0)));
        track.setDistortionTone (static_cast<float> (readNumber (distortion, "tone", 0.5)));
        track.setDistortionMode (static_cast<DistortionMode> (static_cast<int> (readNumber (distortion, "mode", 0.0))));
    }

    if (const auto reverb = root["reverb"]; reverb.isObject())
    {
        track.setReverbWetDry (static_cast<float> (readNumber (reverb, "wetDry", 0.2)));
        track.setReverbDecay (static_cast<float> (readNumber (reverb, "decay", 0.5)));
        track.setReverbSize (static_cast<float> (readNumber (reverb, "size", 0.5)));
    }

    juce::String error;

    if (const auto irFile = root["irFile"].toString(); irFile.isNotEmpty())
    {
        const auto irError = graph.loadImpulseResponse (juce::File (irFile));

        if (irError.isNotEmpty())
            error = "Could not load impulse response: " + irError;
    }
    else
    {
        graph.clearImpulseResponse();
    }

    if (const auto plugin = root["plugin"]; plugin.isObject())
    {
        juce::PluginDescription description;

        if (auto xml = juce::parseXML (plugin["descriptionXml"].toString()))
            description.loadFromXml (*xml);
        else
            return "Could not parse plugin description";

        if (description.fileOrIdentifier.isNotEmpty())
        {
            const auto pluginError = track.loadVst3Plugin (description,
                                                           graph.getPreparedSampleRate(),
                                                           graph.getPreparedBufferSize());

            if (pluginError.isNotEmpty())
            {
                error = "Could not load plugin '" + description.name + "': " + pluginError;
            }
            else if (auto* instance = track.getVst3PluginInstance())
            {
                const auto parameterValues = plugin["parameters"];

                if (parameterValues.isArray())
                {
                    const auto& parameters = instance->getParameters();

                    for (int p = 0; p < parameterValues.size() && p < parameters.size(); ++p)
                        parameters.getUnchecked (p)->setValue (static_cast<float> (readNumber (parameterValues[p], 0.0)));
                }

                const auto stateText = plugin["state"].toString();

                if (stateText.isNotEmpty())
                {
                    juce::MemoryBlock state;

                    if (state.fromBase64Encoding (stateText) && state.getSize() > 0)
                        instance->setStateInformation (state.getData(), static_cast<int> (state.getSize()));
                }
            }
        }
    }
    else
    {
        track.unloadVst3Plugin();
    }

    if (const auto bypasses = root["bypasses"]; bypasses.isArray())
    {
        for (int slot = 0; slot < static_cast<int> (ProcessorSlot::count); ++slot)
        {
            const bool shouldBypass = slot < bypasses.size() && static_cast<bool> (bypasses[slot]);
            track.setProcessorBypassed (static_cast<ProcessorSlot> (slot), shouldBypass);
        }
    }
    else
    {
        for (int slot = 0; slot < static_cast<int> (ProcessorSlot::count); ++slot)
            track.setProcessorBypassed (static_cast<ProcessorSlot> (slot), false);
    }

    return error;
}
} // namespace daw
