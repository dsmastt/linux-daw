#include <iostream>

#include <juce_core/juce_core.h>

#include "AudioDeviceDiscovery.h"

int main (int argc, char* argv[])
{
    juce::ConsoleApplication app;

    app.addHelpCommand ("--help|-h", "linux-daw usage:", true);
    app.addVersionCommand ("--version|-v", "linux-daw 0.1.0");

    app.addDefaultCommand ({ "",
                             "",
                             "Starts the application",
                             "",
                             [] (const juce::ArgumentList&)
                             {
                                 std::cout << "Application started." << std::endl;
                                 daw::printAudioDevices();
                             }});

    return app.findAndRunCommand (argc, argv);
}
