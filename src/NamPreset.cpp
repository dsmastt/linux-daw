#include "NamPreset.h"

#include <cmath>
#include <cstring>

namespace daw
{
namespace
{
struct Version
{
    int major = 0;
    int minor = 0;
    int patch = 0;
};

Version parseVersion (const juce::String& text)
{
    Version version;
    const auto parts = juce::StringArray::fromTokens (text, ".", "");

    if (parts.size() > 0) version.major = parts[0].getIntValue();
    if (parts.size() > 1) version.minor = parts[1].getIntValue();
    if (parts.size() > 2) version.patch = parts[2].getIntValue();

    return version;
}

bool atLeast (const Version& version, int major, int minor, int patch)
{
    if (version.major != major) return version.major > major;
    if (version.minor != minor) return version.minor > minor;
    return version.patch >= patch;
}

void writeString (juce::MemoryOutputStream& output, const juce::String& text)
{
    const auto utf8 = text.toRawUTF8();
    const int length = static_cast<int> (std::strlen (utf8));

    output.writeInt (length);
    output.write (utf8, static_cast<size_t> (length));
}

juce::String toWinePath (const juce::File& file)
{
    return "Z:" + file.getFullPathName().replaceCharacter ('/', '\\');
}

juce::String storeInWinePrefix (const juce::File& source, juce::String& error)
{
    const auto home = juce::File::getSpecialLocation (juce::File::userHomeDirectory);
    const auto wineDrive = home.getChildFile (".wine/drive_c");

    if (! wineDrive.isDirectory())
        return toWinePath (source);

    auto directory = wineDrive.getChildFile ("ProgramData/linux-daw/nam");

    if (! directory.createDirectory())
    {
        error = "Could not create the NAM sample directory";
        return {};
    }

    auto destination = directory.getChildFile (source.getFileName());

    if (source == destination)
        return "C:\\ProgramData\\linux-daw\\nam\\" + destination.getFileName();

    if (! destination.existsAsFile() || destination.getSize() != source.getSize())
    {
        destination.deleteFile();

        if (! source.copyFileTo (destination))
        {
            error = "Could not copy the file into the Wine prefix";
            return {};
        }
    }

    return "C:\\ProgramData\\linux-daw\\nam\\" + destination.getFileName();
}

juce::AudioProcessorParameter* findParameter (juce::AudioPluginInstance& instance,
                                              const juce::String& normalisedName)
{
    for (auto* parameter : instance.getParameters())
    {
        if (parameter == nullptr)
            continue;

        juce::String normalised;

        for (auto character : parameter->getName (80))
            if (juce::CharacterFunctions::isLetterOrDigit (character))
                normalised << juce::CharacterFunctions::toLowerCase (character);

        if (normalised == normalisedName)
            return parameter;
    }

    return nullptr;
}

double readParameterValue (juce::AudioPluginInstance& instance,
                           const juce::String& name,
                           double defaultValue)
{
    auto* parameter = findParameter (instance, name);

    if (parameter == nullptr)
        return defaultValue;

    const auto text = parameter->getText (parameter->getValue(), 64).trim();
    const auto lower = text.toLowerCase();

    if (lower == "on" || lower == "true" || lower == "active")
        return 1.0;

    if (lower == "off" || lower == "false" || lower == "inactive")
        return 0.0;

    if (lower == "raw")
        return 0.0;

    if (lower == "normalized")
        return 1.0;

    if (lower == "calibrated")
        return 2.0;

    if (text.containsAnyOf ("0123456789"))
        return static_cast<double> (text.getFloatValue());

    return defaultValue;
}

juce::String buildAndApplyState (juce::AudioPluginInstance& instance,
                                 const juce::String& pluginVersion,
                                 const juce::String& modelWindowsPath,
                                 const juce::String& irWindowsPath,
                                 bool forceIrOn)
{
    Version version = parseVersion (pluginVersion);

    if (version.major == 0 && version.minor == 0 && version.patch == 0)
        version = parseVersion ("0.7.14");

    const auto versionText = juce::String (version.major) + "." + juce::String (version.minor)
                             + "." + juce::String (version.patch);

    const double input = readParameterValue (instance, "input", 0.0);
    const double threshold = readParameterValue (instance, "threshold", -80.0);
    const double bass = readParameterValue (instance, "bass", 5.0);
    const double middle = readParameterValue (instance, "middle", 5.0);
    const double treble = readParameterValue (instance, "treble", 5.0);
    const double output = readParameterValue (instance, "output", 0.0);
    const double noiseGate = readParameterValue (instance, "noisegateactive", 1.0);
    const double toneStack = readParameterValue (instance, "tonestack", 1.0);
    const double irToggle = forceIrOn ? 1.0 : readParameterValue (instance, "irtoggle", 1.0);
    const double calibrateInput = readParameterValue (instance, "calibrateinput", 0.0);
    const double inputCalibration = readParameterValue (instance, "inputcalibrationlevel", 12.0);
    const double outputMode = readParameterValue (instance, "outputmode", 1.0);
    const double slim = readParameterValue (instance, "slim", 0.0);

    juce::MemoryOutputStream raw;
    writeString (raw, "###NeuralAmpModeler###");
    writeString (raw, versionText);
    writeString (raw, modelWindowsPath);
    writeString (raw, irWindowsPath);

    if (atLeast (version, 0, 7, 14))
    {
        for (double value : { input, threshold, bass, middle, treble, output,
                              noiseGate, toneStack, irToggle, calibrateInput,
                              inputCalibration, outputMode, slim })
            raw.writeDouble (value);
    }
    else if (atLeast (version, 0, 7, 12))
    {
        for (double value : { input, threshold, bass, middle, treble, output,
                              noiseGate, toneStack, irToggle, calibrateInput,
                              inputCalibration, outputMode })
            raw.writeDouble (value);
    }
    else
    {
        for (double value : { input, threshold, bass, middle, treble, output,
                              noiseGate, toneStack, outputMode, irToggle })
            raw.writeDouble (value);
    }

    raw.writeInt (0);

    juce::XmlElement root ("VST3PluginState");
    root.createNewChildElement ("IComponent")->addTextElement (raw.getMemoryBlock().toBase64Encoding());

    juce::MemoryBlock wrapped;
    juce::AudioProcessor::copyXmlToBinary (root, wrapped);

    try
    {
        instance.setStateInformation (wrapped.getData(), static_cast<int> (wrapped.getSize()));
    }
    catch (...)
    {
        return "The NAM plugin rejected the state";
    }

    return {};
}

juce::String loadFile (juce::AudioPluginInstance& instance,
                       const juce::String& pluginVersion,
                       const juce::File& file,
                       const juce::String& otherPath,
                       bool isModel,
                       juce::String& storedWindowsPath)
{
    if (! file.existsAsFile())
        return "File not found";

    juce::String error;
    const auto windowsPath = storeInWinePrefix (file, error);

    if (windowsPath.isEmpty())
        return error;

    const auto result = isModel
                            ? buildAndApplyState (instance, pluginVersion, windowsPath, otherPath, false)
                            : buildAndApplyState (instance, pluginVersion, otherPath, windowsPath, true);

    if (result.isEmpty())
        storedWindowsPath = windowsPath;

    return result;
}
} // namespace

juce::String NamPreset::loadModel (juce::AudioPluginInstance& instance,
                                   const juce::String& pluginVersion,
                                   const juce::File& modelFile,
                                   const juce::String& currentIrWindowsPath,
                                   juce::String& newModelWindowsPath)
{
    return loadFile (instance, pluginVersion, modelFile, currentIrWindowsPath, true, newModelWindowsPath);
}

juce::String NamPreset::loadIr (juce::AudioPluginInstance& instance,
                                const juce::String& pluginVersion,
                                const juce::File& irFile,
                                const juce::String& currentModelWindowsPath,
                                juce::String& newIrWindowsPath)
{
    return loadFile (instance, pluginVersion, irFile, currentModelWindowsPath, false, newIrWindowsPath);
}
} // namespace daw
