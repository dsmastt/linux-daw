#include "NeuralPreset.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <set>

namespace daw
{
namespace
{
class Reader
{
public:
    Reader (const void* dataToUse, size_t sizeToUse)
        : data (static_cast<const unsigned char*> (dataToUse)), size (sizeToUse)
    {
    }

    bool ok() const noexcept { return ! failed; }

    int position() const noexcept { return pos; }
    void setPosition (int newPosition) noexcept { pos = newPosition; }

    unsigned char u8() noexcept
    {
        if (pos >= static_cast<int> (size))
        {
            failed = true;
            return 0;
        }

        return data[pos++];
    }

    std::int32_t i32() noexcept
    {
        if (pos + 4 > static_cast<int> (size))
        {
            failed = true;
            return 0;
        }

        std::int32_t value = 0;
        std::memcpy (&value, data + pos, sizeof (value));
        pos += 4;
        return value;
    }

    std::int64_t i64() noexcept
    {
        if (pos + 8 > static_cast<int> (size))
        {
            failed = true;
            return 0;
        }

        std::int64_t value = 0;
        std::memcpy (&value, data + pos, sizeof (value));
        pos += 8;
        return value;
    }

    double f64() noexcept
    {
        if (pos + 8 > static_cast<int> (size))
        {
            failed = true;
            return 0.0;
        }

        double value = 0.0;
        std::memcpy (&value, data + pos, sizeof (value));
        pos += 8;
        return value;
    }

    void bytes (void* destination, int numBytes) noexcept
    {
        if (numBytes < 0 || static_cast<size_t> (pos) + static_cast<size_t> (numBytes) > size)
        {
            failed = true;
            return;
        }

        std::memcpy (destination, data + pos, (size_t) numBytes);
        pos += numBytes;
    }

    juce::String zeroTerminatedString() noexcept
    {
        const int start = pos;

        while (pos < (int) size && data[pos] != 0)
            ++pos;

        if (pos >= (int) size)
        {
            failed = true;
            return {};
        }

        const auto result = juce::String::fromUTF8 (reinterpret_cast<const char*> (data + start), pos - start);
        ++pos;
        return result;
    }

private:
    const unsigned char* data = nullptr;
    size_t size = 0;
    int pos = 0;
    bool failed = false;
};

struct ParsedValue
{
    bool isNumber = false;
    bool isString = false;
    double number = 0.0;
    juce::String text;
};

bool readProperty (Reader& reader, juce::String& name, ParsedValue& value)
{
    name = reader.zeroTerminatedString();

    if (! reader.ok() || reader.u8() != 1)
        return false;

    const int recordSize = reader.u8();
    const int type = reader.u8();

    if (! reader.ok() || recordSize < 1)
        return false;

    const int payloadSize = recordSize - 1;
    value = {};

    switch (type)
    {
        case 1:
            value.isNumber = true;
            value.number = static_cast<double> (reader.i32());
            return reader.ok();

        case 2:
            value.isNumber = true;
            value.number = payloadSize > 0 ? (reader.u8() != 0 ? 1.0 : 0.0) : 0.0;
            return reader.ok();

        case 4:
            value.isNumber = true;
            value.number = reader.f64();
            return reader.ok();

        case 3:
            value.isNumber = true;
            value.number = payloadSize > 0 ? (reader.u8() != 0 ? 1.0 : 0.0) : 1.0;
            return reader.ok();

        case 5:
        {
            std::vector<char> bytes (static_cast<size_t> (payloadSize) + 1, 0);
            reader.bytes (bytes.data(), payloadSize);
            value.isString = true;
            value.text = juce::String::fromUTF8 (bytes.data());
            return reader.ok();
        }

        case 6:
            value.isNumber = true;
            value.number = static_cast<double> (reader.i64());
            return reader.ok();

        default:
            reader.bytes (nullptr, 0);
            return false;
    }
}

juce::String normalise (const juce::String& input)
{
    juce::String result;

    for (auto character : input)
        if (juce::CharacterFunctions::isLetterOrDigit (character))
            result << juce::CharacterFunctions::toLowerCase (character);

    return result;
}

bool parseTextNumber (const juce::String& text, double& result)
{
    auto trimmed = text.trim();

    if (trimmed.isEmpty() || ! trimmed.containsAnyOf ("0123456789"))
        return false;

    const auto lower = trimmed.toLowerCase();

    if (lower.contains ("inf") || lower.contains ("nan"))
        return false;

    result = static_cast<double> (trimmed.getFloatValue());

    if (lower.contains ("khz") || lower.endsWith (" k"))
        result *= 1000.0;

    if (lower.contains ("%"))
        result *= 0.01;

    return true;
}

struct ParameterRef
{
    juce::AudioProcessorParameter* parameter = nullptr;
    juce::String normalisedName;
    int index = 0;
};

bool valueToNormalised (juce::AudioProcessorParameter& parameter,
                        double nativeValue,
                        double& normalisedValue)
{
    const int numSteps = parameter.getNumSteps();

    // VST3 reports a step count for continuous parameters as well. Only
    // treat small step counts as enumerations/booleans.
    if (numSteps > 1 && numSteps <= 32)
    {
        if (numSteps == 2)
            normalisedValue = nativeValue > 0.5 ? 1.0 : 0.0;
        else
            normalisedValue = juce::jlimit (0.0, 1.0, nativeValue / static_cast<double> (numSteps - 1));

        return true;
    }

    double textAtZero = 0.0;
    double textAtOne = 0.0;

    if (parseTextNumber (parameter.getText (0.0f, 64), textAtZero)
        && parseTextNumber (parameter.getText (1.0f, 64), textAtOne))
    {
        const double minimum = juce::jmin (textAtZero, textAtOne);
        const double maximum = juce::jmax (textAtZero, textAtOne);

        if (nativeValue >= minimum - 1.0e-4 && nativeValue <= maximum + 1.0e-4)
        {
            const bool increasing = textAtOne >= textAtZero;
            double low = 0.0;
            double high = 1.0;

            for (int iteration = 0; iteration < 28; ++iteration)
            {
                const double middle = (low + high) * 0.5;
                double textAtMiddle = 0.0;

                if (! parseTextNumber (parameter.getText (static_cast<float> (middle), 64), textAtMiddle))
                    break;

                if (increasing ? textAtMiddle < nativeValue : textAtMiddle > nativeValue)
                    low = middle;
                else
                    high = middle;
            }

            normalisedValue = juce::jlimit (0.0, 1.0, (low + high) * 0.5);
            return true;
        }
    }

    if (nativeValue >= 0.0 && nativeValue <= 1.0)
    {
        normalisedValue = nativeValue;
        return true;
    }

    return false;
}

struct AmpPrefix
{
    const char* prefix;
    int group;
};

constexpr AmpPrefix ampPrefixes[] =
{
    { "clean", 0 }, { "crunch", 1 }, { "rhythm", 1 }, { "lead", 2 }, { "rust", 1 }, { "hot", 2 },
    { "amp1", 0 }, { "amp2", 1 }, { "amp3", 2 }, { "amp4", 3 }
};

const ParameterRef* findAmpGroupParameter (const std::vector<ParameterRef>& parameters,
                                           const juce::String& suffix,
                                           int group,
                                           const juce::String& ampLabel)
{
    std::vector<const ParameterRef*> labelledMatches;
    std::vector<const ParameterRef*> allMatches;

    for (const auto& parameter : parameters)
    {
        if (parameter.normalisedName.endsWith (suffix))
        {
            allMatches.push_back (&parameter);

            if (ampLabel.isNotEmpty() && parameter.normalisedName.startsWith (ampLabel))
                labelledMatches.push_back (&parameter);
        }
    }

    if (! labelledMatches.empty())
        return labelledMatches.front();

    if (ampLabel.isNotEmpty())
    {
        for (const auto& parameter : parameters)
        {
            if (parameter.normalisedName.startsWith (ampLabel)
                && parameter.normalisedName.contains (suffix))
                return &parameter;
        }
    }

    if (group < 0 || group >= static_cast<int> (allMatches.size()))
        return nullptr;

    return allMatches[static_cast<size_t> (group)];
}

juce::StringArray getAmpLabels (const std::vector<ParameterRef>& parameters,
                                const std::map<juce::String, int>& exactMatches)
{
    juce::StringArray labels;

    const auto exact = exactMatches.find ("amptype");

    if (exact == exactMatches.end())
        return labels;

    auto* parameter = parameters[static_cast<size_t> (exact->second)].parameter;

    if (parameter == nullptr)
        return labels;

    const int steps = parameter->getNumSteps();

    if (steps < 2 || steps > 16)
        return labels;

    for (int step = 0; step < steps; ++step)
    {
        const auto text = parameter->getText (static_cast<float> (step) / static_cast<float> (steps - 1), 64);
        const auto firstWord = text.upToFirstOccurrenceOf (" ", false, false);
        labels.add (normalise (firstWord.isNotEmpty() ? firstWord : text));
    }

    return labels;
}

juce::String translateAmpSuffix (juce::String suffix)
{
    if (suffix.endsWith ("ampbass"))
        suffix = suffix.dropLastCharacters (4) + "low";

    if (suffix.endsWith ("amptreble"))
        suffix = suffix.dropLastCharacters (6) + "high";

    return suffix;
}

const ParameterRef* findParameter (const std::vector<ParameterRef>& parameters,
                                   const std::map<juce::String, int>& exactMatches,
                                   const juce::StringArray& ampLabels,
                                   const juce::String& nativeId)
{
    auto normalisedId = normalise (nativeId);

    if (normalisedId.endsWith ("id"))
        normalisedId = normalisedId.dropLastCharacters (2);

    if (const auto exact = exactMatches.find (normalisedId); exact != exactMatches.end())
        return &parameters[static_cast<size_t> (exact->second)];

    static const std::pair<const char*, const char*> aliases[] =
    {
        { "ampsactive", "Amp Section Active" }, { "activeamps", "Amp Section Active" },
        { "activeampsid", "Amp Section Active" }, { "cabactive", "Cab Section Active" },
        { "selectedamp", "Amp Type" }, { "selectedcab", "Cab Type" },
        { "ampcablinkedstate", "Cab/Amp Linked" },
        { "activeampsection", "Amp Section Active" }, { "activecabsection", "Cab Section Active" },
        { "activeeqsection", "EQ Section Active" }, { "activepedalsection", "Pedal Section Active" },
        { "activetimesection", "FX Section Active" },
        { "eqactive", "EQ Section Active" }, { "activeeq", "EQ Section Active" },
        { "fxactive", "FX Section Active" },
        { "pitchactive", "Pitch Section Active" }, { "pedalsactive", "Pedal Section Active" },
        { "gatethreshold", "Gate Amount" },
        { "whammyactive", "WOW Active" }, { "whammytype", "WOW Type" },
        { "whammyposition", "WOW Position" }, { "whammymix", "WOW Dry/Wet" },
        { "octaveractive", "OCT Active" }, { "octaverdirectlevel", "OCT Direct Level" },
        { "octaveroct1level", "OCT Oct 1 Level" }, { "octaveroct2level", "OCT Oct 2 Level" },
        { "pedal2active", "OD Active" }, { "pedal2overdrive", "OD Dist" },
        { "pedal2tone", "OD Tone" }, { "pedal2level", "OD Level" },
        { "pedal3active", "DRT Active" }, { "pedal3distortion", "DRT Dist" },
        { "pedal3filter", "DRT Filter" }, { "pedal3volume", "DRT Vol" },
        { "pedal4active", "PHSR Active" }, { "pedal4lforate", "PHSR Rate" },
        { "pedal5active", "CHR Active" }, { "pedal5depth", "CHR Depth" },
        { "pedal5feedback", "CHR Feedback" }, { "pedal5mix", "CHR Mix" }, { "pedal5rate", "CHR Rate" },
        { "delayactive", "DLY Active" }, { "delaysyncactive", "DLY Sync Active" },
        { "delaypingpong", "DLY Ping-Pong" }, { "delaymodactive", "DLY Mod Active" },
        { "delaydrywet", "DLY Dry/Wet" }, { "delayfeedback", "DLY Feedback" },
        { "delaytapesaturation", "DLY Tape Saturation" }, { "delaytempo", "DLY Tempo" },
        { "delaysyncnote", "DLY Sync Note" }, { "delaytone", "DLY Tone" },
        { "delaytaptempo", "DLY Tap Tempo" },
        { "reverbactive", "REV Active" }, { "reverbmode", "REV Mode" },
        { "reverbdrywet", "REV Dry/Wet" }, { "reverbtime", "REV Time" },
        { "reverblowcut", "REV Low Cut" }, { "reverbhighcut", "REV High Cut" }
    };

    for (const auto& alias : aliases)
    {
        if (normalisedId != alias.first)
            continue;

        if (const auto exact = exactMatches.find (normalise (alias.second)); exact != exactMatches.end())
            return &parameters[static_cast<size_t> (exact->second)];

        break;
    }

    for (const auto& prefix : ampPrefixes)
    {
        if (! normalisedId.startsWith (prefix.prefix))
            continue;

        auto suffix = normalisedId.substring (static_cast<int> (std::strlen (prefix.prefix)));

        if (suffix.endsWith ("volume"))
            suffix = suffix.dropLastCharacters (6) + "level";

        if (suffix.endsWith ("output") || suffix.isEmpty())
            return nullptr;

        const auto label = prefix.group < ampLabels.size() ? ampLabels[prefix.group] : juce::String();

        if (const auto* match = findAmpGroupParameter (parameters, suffix, prefix.group, label))
            return match;

        const auto translated = translateAmpSuffix (suffix);

        if (translated != suffix)
            return findAmpGroupParameter (parameters, translated, prefix.group, label);

        return nullptr;
    }

    return nullptr;
}
} // namespace

std::vector<NeuralPresetEntry> NeuralPreset::findPresets (const juce::String& pluginName)
{
    std::vector<juce::File> roots;

    auto addRoot = [&roots] (const juce::File& root)
    {
        if (root.isDirectory())
            for (const auto& existing : roots)
                if (existing == root)
                    return;

        if (root.isDirectory())
            roots.push_back (root);
    };

    const auto home = juce::File::getSpecialLocation (juce::File::userHomeDirectory);
    const auto userName = home.getFileName();

    addRoot (home.getChildFile (".wine/drive_c/ProgramData/Neural DSP"));
    addRoot (home.getChildFile (".wine/drive_c/users").getChildFile (userName).getChildFile ("AppData/Roaming/Neural DSP"));
    addRoot (home.getChildFile (".wine/drive_c/users").getChildFile (userName).getChildFile ("Documents/Neural DSP"));
    addRoot (home.getChildFile ("Documents/Neural DSP"));

    const auto mounts = juce::File ("/mnt").findChildFiles (juce::File::findDirectories, false);

    for (const auto& mount : mounts)
    {
        addRoot (mount.getChildFile ("windows/ProgramData/Neural DSP"));
        addRoot (mount.getChildFile ("ProgramData/Neural DSP"));
        addRoot (mount.getChildFile ("windows/Users").getChildFile (userName).getChildFile ("AppData/Roaming/Neural DSP"));
        addRoot (mount.getChildFile ("windows/Users").getChildFile (userName).getChildFile ("Documents/Neural DSP"));
    }

    std::vector<NeuralPresetEntry> entries;
    juce::StringArray seenFiles;

    for (const auto& root : roots)
    {
        auto pluginDirectory = root.getChildFile (pluginName);

        if (! pluginDirectory.isDirectory())
        {
            pluginDirectory = juce::File();

            for (const auto& candidate : root.findChildFiles (juce::File::findDirectories, false))
            {
                if (candidate.getFileName().equalsIgnoreCase (pluginName))
                {
                    pluginDirectory = candidate;
                    break;
                }
            }
        }

        if (! pluginDirectory.isDirectory())
            continue;

        for (const auto& file : pluginDirectory.findChildFiles (juce::File::findFiles, true, "*.xml"))
        {
            if (file.getFileName().containsIgnoreCase ("midi"))
                continue;

            const auto path = file.getFullPathName();

            if (seenFiles.contains (path))
                continue;

            seenFiles.add (path);

            auto relative = file.getRelativePathFrom (pluginDirectory).replaceCharacter ('\\', '/');
            relative = relative.upToLastOccurrenceOf (".", false, false);

            entries.push_back ({ file, relative, relative });
        }
    }

    std::sort (entries.begin(), entries.end(), [] (const NeuralPresetEntry& a, const NeuralPresetEntry& b)
    {
        return a.displayName.compareIgnoreCase (b.displayName) < 0;
    });

    return entries;
}

juce::String NeuralPreset::load (const juce::File& file,
                                 juce::AudioPluginInstance& instance,
                                 int& matchedParameters,
                                 int& totalParameters)
{
    matchedParameters = 0;
    totalParameters = 0;

    juce::MemoryBlock contents;

    if (! file.loadFileAsData (contents))
        return "Could not read preset file";

    Reader reader (contents.getData(), contents.getSize());

    const auto rootType = reader.zeroTerminatedString();

    if (! reader.ok() || rootType.isEmpty() || reader.u8() != 1)
        return "Unsupported preset format";

    const int propertyCount = reader.u8();

    if (! reader.ok())
        return "Truncated preset file";

    juce::String presetName;

    for (int property = 0; property < propertyCount; ++property)
    {
        juce::String name;
        ParsedValue value;

        if (! readProperty (reader, name, value))
            return "Invalid preset property";

        if ((name == "presetNameProp" || name == "name") && value.isString)
            presetName = value.text;
    }

    std::vector<ParameterRef> parameters;
    const auto& pluginParameters = instance.getParameters();

    for (int index = 0; index < pluginParameters.size(); ++index)
    {
        auto* parameter = pluginParameters[index];

        if (parameter == nullptr)
            continue;

        const auto name = parameter->getName (80);
        const auto normalised = normalise (name);

        if (normalised.isNotEmpty())
            parameters.push_back ({ parameter, normalised, index });
    }

    std::map<juce::String, int> exactMatches;

    for (int index = 0; index < static_cast<int> (parameters.size()); ++index)
        exactMatches.emplace (parameters[static_cast<size_t> (index)].normalisedName, index);

    const auto ampLabels = getAmpLabels (parameters, exactMatches);

    std::map<juce::String, double> nativeParameters;
    const auto* rawData = static_cast<const unsigned char*> (contents.getData());
    const auto rawSize = contents.getSize();

    // Old Neural DSP presets store parameters in PARAM child nodes.
    for (size_t position = 0; position + 6 <= rawSize; ++position)
    {
        if (std::memcmp (rawData + position, "PARAM", 5) != 0 || rawData[position + 5] != 0)
            continue;

        Reader parameterReader (contents.getData(), contents.getSize());
        parameterReader.setPosition (static_cast<int> (position + 6));

        if (parameterReader.u8() != 1)
            continue;

        const int parameterPropertyCount = parameterReader.u8();

        if (! parameterReader.ok() || parameterPropertyCount <= 0)
            continue;

        juce::String id;
        double value = 0.0;
        bool hasId = false;
        bool hasValue = false;

        for (int property = 0; property < parameterPropertyCount; ++property)
        {
            juce::String name;
            ParsedValue parsedValue;

            if (! readProperty (parameterReader, name, parsedValue))
                break;

            if (name == "id" && parsedValue.isString)
            {
                id = parsedValue.text;
                hasId = true;
            }
            else if (name == "value" && parsedValue.isNumber)
            {
                value = parsedValue.number;
                hasValue = true;
            }
        }

        if (hasId && hasValue && id.isNotEmpty())
            nativeParameters[id] = value;
    }

    // Newer Neural DSP presets store parameters as named properties spread
    // over several sections. Scan every string that is followed by a value
    // record and keep the ones that match a plugin parameter.
    for (size_t start = 0; start + 1 < rawSize; ++start)
    {
        if (! (rawData[start] >= 32 && rawData[start] < 127))
            continue;

        size_t end = start;

        while (end < rawSize && rawData[end] != 0
               && (rawData[end] >= 32 && rawData[end] < 127)
               && end - start < 96)
            ++end;

        if (end >= rawSize || rawData[end] != 0 || end == start)
            continue;

        const auto name = juce::String::fromUTF8 (reinterpret_cast<const char*> (rawData + start),
                                                  static_cast<int> (end - start));

        Reader valueReader (contents.getData(), contents.getSize());
        valueReader.setPosition (static_cast<int> (end + 1));

        if (valueReader.u8() != 1)
        {
            start = end;
            continue;
        }

        const int recordSize = valueReader.u8();
        const int type = valueReader.u8();

        if (! valueReader.ok() || recordSize < 1)
        {
            start = end;
            continue;
        }

        const int payloadSize = recordSize - 1;
        std::vector<char> payload (static_cast<size_t> (payloadSize) + 1, 0);
        valueReader.bytes (payload.data(), payloadSize);

        if (! valueReader.ok())
        {
            start = end;
            continue;
        }

        double value = 0.0;
        bool hasValue = false;

        switch (type)
        {
            case 1:
                if (payloadSize == 4)
                {
                    std::int32_t intValue = 0;
                    std::memcpy (&intValue, payload.data(), sizeof (intValue));
                    value = static_cast<double> (intValue);
                    hasValue = true;
                }
                break;

            case 2:
                value = payloadSize > 0 ? (payload[0] != 0 ? 1.0 : 0.0) : 0.0;
                hasValue = true;
                break;

            case 3:
                value = payloadSize > 0 ? (payload[0] != 0 ? 1.0 : 0.0) : 1.0;
                hasValue = true;
                break;

            case 4:
                if (payloadSize == 8)
                {
                    std::memcpy (&value, payload.data(), sizeof (value));
                    hasValue = true;
                }
                break;

            case 5:
            {
                const juce::String text (payload.data());
                const auto lower = text.trim().toLowerCase();

                if (lower == "true" || lower == "on")
                {
                    value = 1.0;
                    hasValue = true;
                }
                else if (lower == "false" || lower == "off")
                {
                    value = 0.0;
                    hasValue = true;
                }
                else
                {
                    hasValue = parseTextNumber (text, value);
                }

                break;
            }

            case 6:
                if (payloadSize == 8)
                {
                    std::int64_t intValue = 0;
                    std::memcpy (&intValue, payload.data(), sizeof (intValue));
                    value = static_cast<double> (intValue);
                    hasValue = true;
                }
                break;

            default:
                break;
        }

        if (hasValue && findParameter (parameters, exactMatches, ampLabels, name) != nullptr)
            nativeParameters[name] = value;

        start = end;
    }

    if (nativeParameters.empty())
        return "No parameters found in preset";

    totalParameters = static_cast<int> (nativeParameters.size());

    for (const auto& native : nativeParameters)
    {
        const auto* match = findParameter (parameters, exactMatches, ampLabels, native.first);

        if (match == nullptr)
            continue;

        double normalisedValue = 0.0;

        if (! valueToNormalised (*match->parameter, native.second, normalisedValue))
            continue;

        match->parameter->setValue (static_cast<float> (normalisedValue));
        ++matchedParameters;
    }

    if (matchedParameters == 0)
        return "No matching plugin parameters found in preset";

    juce::ignoreUnused (presetName);
    return {};
}
} // namespace daw
