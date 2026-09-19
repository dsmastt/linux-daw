#include "Project.h"

#include <algorithm>

#include "PlaybackProcessor.h"
#include "Vst3HostProcessor.h"

namespace daw
{
namespace
{
juce::String relativeOrAbsolute (const juce::File& projectFile, const juce::String& path)
{
    if (path.isEmpty() || projectFile.getFullPathName().isEmpty())
        return path;

    const juce::File file (path);
    const auto relative = file.getRelativePathFrom (projectFile.getParentDirectory());

    return relative.startsWith ("..") ? file.getFullPathName() : relative;
}

juce::String resolvePath (const juce::File& projectFile, const juce::String& path)
{
    if (path.isEmpty())
        return path;

    return projectFile.getSiblingFile (path).getFullPathName();
}

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
    if (!object.isObject())
        return defaultValue;

    return readNumber (object[property], defaultValue);
}

bool readBool (const juce::var& object, const char* property, bool defaultValue)
{
    if (!object.isObject())
        return defaultValue;

    const auto& value = object[property];

    return value.isVoid() ? defaultValue : static_cast<bool> (value);
}
} // namespace

juce::String Project::save (const juce::File& file, AudioGraph& graph)
{
    const auto root = graphToVar (file, graph);

    const auto result = file.replaceWithText (juce::JSON::toString (root, true));

    return result ? juce::String() : juce::String ("Could not write project file: " + file.getFullPathName());
}

juce::String Project::load (const juce::File& file, AudioGraph& graph)
{
    if (!file.existsAsFile())
        return "Project file not found: " + file.getFullPathName();

    const auto root = juce::JSON::parse (file.loadFileAsString());

    if (root.isVoid())
        return "Could not parse project file: " + file.getFullPathName();

    return applyVar (file, root, graph);
}

juce::var Project::graphToVar (const juce::File& projectFile, const AudioGraph& graph)
{
    juce::DynamicObject::Ptr root = new juce::DynamicObject();

    root->setProperty ("format", "linux-daw-project");
    root->setProperty ("version", 1);
    root->setProperty ("tempoBpm", graph.getTransport().getTempoBpm());
    root->setProperty ("clipFile", relativeOrAbsolute (projectFile, juce::File (graph.getPlaybackFilePath()).getFullPathName()));

    juce::Array<juce::var> tracksArray;

    for (int i = 0; i < AudioGraph::numTracks; ++i)
        tracksArray.add (trackToVar (graph.getTrack (i)));

    root->setProperty ("tracks", tracksArray);

    return juce::var (root.get());
}

juce::var Project::trackToVar (const Track& track)
{
    juce::DynamicObject::Ptr trackObject = new juce::DynamicObject();

    trackObject->setProperty ("volumeDb", track.getVolumeDb());
    trackObject->setProperty ("pan", track.getPan());
    trackObject->setProperty ("muted", track.isMuted());
    trackObject->setProperty ("soloed", track.isSoloed());

    trackObject->setProperty ("gainDb", track.getGainDb());

    juce::DynamicObject::Ptr gate = new juce::DynamicObject();
    gate->setProperty ("thresholdDb", track.getGateThresholdDb());
    gate->setProperty ("attackMs", track.getGateAttackMs());
    gate->setProperty ("releaseMs", track.getGateReleaseMs());
    trackObject->setProperty ("gate", juce::var (gate.get()));

    juce::Array<juce::var> eqBands;

    for (int band = 0; band < track.getNumEqBands(); ++band)
    {
        juce::DynamicObject::Ptr eqBand = new juce::DynamicObject();
        eqBand->setProperty ("frequency", track.getEqBandFrequency (band));
        eqBand->setProperty ("gainDb", track.getEqBandGain (band));
        eqBand->setProperty ("q", track.getEqBandQ (band));
        eqBands.add (juce::var (eqBand.get()));
    }

    trackObject->setProperty ("eqBands", eqBands);

    juce::DynamicObject::Ptr compressor = new juce::DynamicObject();
    compressor->setProperty ("thresholdDb", track.getCompressorThresholdDb());
    compressor->setProperty ("ratio", track.getCompressorRatio());
    compressor->setProperty ("attackMs", track.getCompressorAttackMs());
    compressor->setProperty ("releaseMs", track.getCompressorReleaseMs());
    compressor->setProperty ("makeupDb", track.getCompressorMakeupGainDb());
    trackObject->setProperty ("compressor", juce::var (compressor.get()));

    juce::DynamicObject::Ptr distortion = new juce::DynamicObject();
    distortion->setProperty ("driveDb", track.getDistortionDriveDb());
    distortion->setProperty ("tone", track.getDistortionTone());
    distortion->setProperty ("mode", static_cast<int> (track.getDistortionMode()));
    trackObject->setProperty ("distortion", juce::var (distortion.get()));

    juce::DynamicObject::Ptr reverb = new juce::DynamicObject();
    reverb->setProperty ("wetDry", track.getReverbWetDry());
    reverb->setProperty ("decay", track.getReverbDecay());
    reverb->setProperty ("size", track.getReverbSize());
    trackObject->setProperty ("reverb", juce::var (reverb.get()));

    trackObject->setProperty ("irFile", track.getImpulseResponseFilePath());

    if (track.isVst3PluginLoaded())
    {
        const auto description = track.getVst3PluginDescription();

        juce::DynamicObject::Ptr plugin = new juce::DynamicObject();
        plugin->setProperty ("name", description.name);
        plugin->setProperty ("descriptionXml", description.createXml()->toString());
        plugin->setProperty ("bypassed", track.isVst3PluginBypassed());

        juce::Array<juce::var> parameterValues;

        if (auto* instance = track.getVst3PluginInstance())
        {
            for (auto* parameter : instance->getParameters())
                if (parameter != nullptr)
                    parameterValues.add (parameter->getValue());
        }

        plugin->setProperty ("parameters", parameterValues);
        trackObject->setProperty ("plugin", juce::var (plugin.get()));
    }

    juce::Array<juce::var> clips;

    for (int i = 0; i < track.getNumClips(); ++i)
    {
        const auto& clip = track.getClip (i);

        juce::DynamicObject::Ptr clipObject = new juce::DynamicObject();
        clipObject->setProperty ("start", static_cast<juce::int64> (clip.startSamples.load()));
        clipObject->setProperty ("length", static_cast<juce::int64> (clip.lengthSamples.load()));
        clipObject->setProperty ("offset", static_cast<juce::int64> (clip.sourceOffsetSamples.load()));
        clips.add (juce::var (clipObject.get()));
    }

    trackObject->setProperty ("clips", clips);

    juce::Array<juce::var> midiEvents;

    for (int i = 0; i < track.getNumMidiEvents(); ++i)
    {
        const auto& event = track.getMidiEvent (i);

        juce::String bytesHex;

        for (int b = 0; b < event.numBytes; ++b)
            bytesHex << juce::String::toHexString (event.bytes[static_cast<size_t> (b)]) << " ";

        juce::DynamicObject::Ptr eventObject = new juce::DynamicObject();
        eventObject->setProperty ("time", static_cast<juce::int64> (event.timeSamples.load()));
        eventObject->setProperty ("bytesHex", bytesHex.trimEnd());
        midiEvents.add (juce::var (eventObject.get()));
    }

    trackObject->setProperty ("midiEvents", midiEvents);

    juce::Array<juce::var> automationCurves;

    for (int curveIndex = 0; curveIndex < track.getNumAutomationCurves(); ++curveIndex)
    {
        const auto& curve = track.getAutomationCurve (curveIndex);
        const int pointCount = curve.pointCount.load();

        juce::DynamicObject::Ptr curveObject = new juce::DynamicObject();
        curveObject->setProperty ("parameter", static_cast<int> (curve.parameter));
        curveObject->setProperty ("pluginParameterIndex", curve.pluginParameterIndex);

        juce::Array<juce::var> points;

        for (int p = 0; p < pointCount; ++p)
        {
            juce::DynamicObject::Ptr pointObject = new juce::DynamicObject();
            pointObject->setProperty ("t", static_cast<juce::int64> (curve.points[static_cast<size_t> (p)].timeSamples.load()));
            pointObject->setProperty ("v", static_cast<double> (curve.points[static_cast<size_t> (p)].value.load()));
            points.add (juce::var (pointObject.get()));
        }

        curveObject->setProperty ("points", points);
        automationCurves.add (juce::var (curveObject.get()));
    }

    trackObject->setProperty ("automation", automationCurves);

    return juce::var (trackObject.get());
}

juce::String Project::applyVar (const juce::File& projectFile, const juce::var& root, AudioGraph& graph)
{
    if (!root.isObject())
        return "Invalid project file structure";

    if (root["format"].toString() != "linux-daw-project")
        return "Not a linux-daw project file";

    graph.getTransport().setTempoBpm (root["tempoBpm"].isVoid() ? 120.0 : static_cast<double> (root["tempoBpm"]));

    const auto clipPath = root["clipFile"].toString();

    if (clipPath.isNotEmpty())
    {
        const auto resolved = resolvePath (projectFile, clipPath);
        const auto error = graph.loadPlaybackFile (juce::File (resolved));

        if (error.isNotEmpty())
            return "Could not load clip file: " + error;
    }

    const auto tracks = root["tracks"];

    if (!tracks.isArray())
        return "Invalid tracks section";

    const double sampleRate = 48000.0;
    const int bufferSize = 128;

    for (int i = 0; i < tracks.size() && i < AudioGraph::numTracks; ++i)
    {
        const auto error = applyTrack (tracks[i], graph.getTrack (i), sampleRate, bufferSize);

        if (error.isNotEmpty())
            return error;
    }

    return {};
}


juce::String Project::applyTrack (const juce::var& trackVar, Track& track, double sampleRate, int bufferSize)
{
    if (!trackVar.isObject())
        return "Invalid track structure";

    track.setVolumeDb (static_cast<float> (readNumber (trackVar, "volumeDb", 0.0)));
    track.setPan (static_cast<float> (readNumber (trackVar, "pan", 0.0)));
    track.setMuted (readBool (trackVar, "muted", false));
    track.setSoloed (readBool (trackVar, "soloed", false));

    track.setGainDb (static_cast<float> (readNumber (trackVar, "gainDb", 0.0)));

    if (const auto gate = trackVar["gate"]; gate.isObject())
    {
        track.setGateThresholdDb (static_cast<float> (readNumber (gate, "thresholdDb", -60.0)));
        track.setGateAttackMs (static_cast<float> (readNumber (gate, "attackMs", 1.0)));
        track.setGateReleaseMs (static_cast<float> (readNumber (gate, "releaseMs", 100.0)));
    }

    if (const auto eqBands = trackVar["eqBands"]; eqBands.isArray())
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

    if (const auto compressor = trackVar["compressor"]; compressor.isObject())
    {
        track.setCompressorThresholdDb (static_cast<float> (readNumber (compressor, "thresholdDb", -10.0)));
        track.setCompressorRatio (static_cast<float> (readNumber (compressor, "ratio", 4.0)));
        track.setCompressorAttackMs (static_cast<float> (readNumber (compressor, "attackMs", 5.0)));
        track.setCompressorReleaseMs (static_cast<float> (readNumber (compressor, "releaseMs", 100.0)));
        track.setCompressorMakeupGainDb (static_cast<float> (readNumber (compressor, "makeupDb", 0.0)));
    }

    if (const auto distortion = trackVar["distortion"]; distortion.isObject())
    {
        track.setDistortionDriveDb (static_cast<float> (readNumber (distortion, "driveDb", 12.0)));
        track.setDistortionTone (static_cast<float> (readNumber (distortion, "tone", 0.5)));
        track.setDistortionMode (static_cast<DistortionMode> (static_cast<int> (readNumber (distortion, "mode", 0.0))));
    }

    if (const auto reverb = trackVar["reverb"]; reverb.isObject())
    {
        track.setReverbWetDry (static_cast<float> (readNumber (reverb, "wetDry", 0.2)));
        track.setReverbDecay (static_cast<float> (readNumber (reverb, "decay", 0.5)));
        track.setReverbSize (static_cast<float> (readNumber (reverb, "size", 0.5)));
    }

    if (const auto irFile = trackVar["irFile"].toString(); irFile.isNotEmpty())
    {
        const auto error = track.loadImpulseResponse (juce::File (irFile));

        if (error.isNotEmpty())
            return "Could not load impulse response: " + error;
    }

    if (const auto plugin = trackVar["plugin"]; plugin.isObject())
    {
        juce::PluginDescription description;

        if (auto xml = juce::parseXML (plugin["descriptionXml"].toString()))
            description.loadFromXml (*xml);
        else
            return "Could not parse plugin description";

        if (description.fileOrIdentifier.isNotEmpty())
        {
            const auto error = track.loadVst3Plugin (description, sampleRate, bufferSize);

            if (error.isNotEmpty())
                return "Could not load plugin '" + description.name + "': " + error;

            if (auto* instance = track.getVst3PluginInstance())
            {
                const auto parameterValues = plugin["parameters"];

                if (parameterValues.isArray())
                {
                    const auto& parameters = instance->getParameters();

                    for (int p = 0; p < parameterValues.size() && p < parameters.size(); ++p)
                        parameters.getUnchecked (p)->setValue (static_cast<float> (readNumber (parameterValues[p], "", 0.0)));
                }
            }

            track.setVst3PluginBypassed (readBool (plugin, "bypassed", false));
        }
    }

    if (const auto clips = trackVar["clips"]; clips.isArray())
    {
        for (int i = 0; i < clips.size(); ++i)
        {
            const auto clip = clips[i];

            track.addClip (static_cast<std::int64_t> (readNumber (clip, "start", 0.0)),
                           static_cast<std::int64_t> (readNumber (clip, "length", 0.0)),
                           static_cast<std::int64_t> (readNumber (clip, "offset", 0.0)));
        }
    }

    if (const auto midiEvents = trackVar["midiEvents"]; midiEvents.isArray())
    {
        for (int i = 0; i < midiEvents.size(); ++i)
        {
            const auto event = midiEvents[i];
            const auto time = static_cast<std::int64_t> (readNumber (event, "time", 0.0));
            const auto bytesHex = event["bytesHex"].toString();

            if (bytesHex.isNotEmpty())
            {
                const auto tokens = juce::StringArray::fromTokens (bytesHex, " ", "");

                std::uint8_t bytes[4] = {};
                int numBytes = 0;

                for (const auto& token : tokens)
                {
                    if (numBytes >= 4)
                        break;

                    bytes[static_cast<size_t> (numBytes)] = static_cast<std::uint8_t> (token.getHexValue32() & 0xFF);
                    ++numBytes;
                }

                track.addMidiEvent (time, bytes, numBytes);
            }
        }
    }

    if (const auto automationCurves = trackVar["automation"]; automationCurves.isArray())
    {
        for (int curveIndex = 0; curveIndex < automationCurves.size(); ++curveIndex)
        {
            const auto curve = automationCurves[curveIndex];

            const auto parameter = static_cast<AutomationParameter> (static_cast<int> (readNumber (curve, "parameter", 0.0)));
            const auto pluginParameterIndex = static_cast<int> (readNumber (curve, "pluginParameterIndex", -1.0));

            if (const auto points = curve["points"]; points.isArray())
            {
                for (int p = 0; p < points.size(); ++p)
                {
                    const auto point = points[p];

                    track.addAutomationPoint (parameter,
                                              pluginParameterIndex,
                                              static_cast<std::int64_t> (readNumber (point, "t", 0.0)),
                                              static_cast<float> (readNumber (point, "v", 0.0)));
                }
            }
        }
    }

    return {};
}
} // namespace daw
