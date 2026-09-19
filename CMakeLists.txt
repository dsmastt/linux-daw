cmake_minimum_required(VERSION 3.24)

project(linux_daw VERSION 0.1.0 LANGUAGES C CXX)

include(FetchContent)

set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)

option(LINUX_DAW_BUILD_TEST_PLUGIN "Build the test VST3 plugin used to verify plugin hosting" ON)

FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.2
    GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(JUCE)

set(CORE_SOURCES
    src/AudioEngine.cpp
    src/AudioGraph.cpp
    src/AudioRecorder.cpp
    src/CompressorProcessor.cpp
    src/ConvolutionProcessor.cpp
    src/DistortionProcessor.cpp
    src/GainProcessor.cpp
    src/MidiInputHandler.cpp
    src/NoiseGateProcessor.cpp
    src/ParametricEqProcessor.cpp
    src/PlaybackProcessor.cpp
    src/ProcessorChain.cpp
    src/ReverbProcessor.cpp
    src/Track.cpp
    src/Transport.cpp
    src/Vst3HostProcessor.cpp)

juce_add_gui_app(linux-daw
    PRODUCT_NAME "linux-daw"
    COMPANY_NAME "linux-daw"
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE)

target_sources(linux-daw PRIVATE
    src/main.cpp
    src/AutomationComponent.cpp
    src/MainComponent.cpp
    src/MainWindow.cpp
    src/MeterComponent.cpp
    src/MidiComponent.cpp
    src/MixerComponent.cpp
    src/ProcessorControlsComponent.cpp
    src/TimelineComponent.cpp
    ${CORE_SOURCES})

target_compile_features(linux-daw PRIVATE cxx_std_20)

target_compile_definitions(linux-daw PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_PLUGINHOST_VST3=1)

target_link_libraries(linux-daw
    PRIVATE juce::juce_core juce::juce_events juce::juce_audio_devices juce::juce_audio_formats juce::juce_dsp juce::juce_gui_basics juce::juce_audio_processors
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

if(LINUX_DAW_BUILD_TEST_PLUGIN)
    juce_add_plugin(linux-daw-test-plugin
        PRODUCT_NAME "Linux DAW Test Plugin"
        COMPANY_NAME "linux-daw"
        PLUGIN_MANUFACTURER_CODE Ldaw
        PLUGIN_CODE TsT1
        FORMATS VST3
        NEEDS_MIDI_INPUT TRUE
        NEEDS_WEB_BROWSER FALSE
        NEEDS_CURL FALSE)

    target_sources(linux-daw-test-plugin PRIVATE test_plugin/PluginProcessor.cpp)

    target_compile_features(linux-daw-test-plugin PRIVATE cxx_std_20)

    target_compile_definitions(linux-daw-test-plugin PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

    target_link_libraries(linux-daw-test-plugin
        PRIVATE juce::juce_audio_processors
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endif()

juce_add_console_app(linux-daw-vst3-test
    PRODUCT_NAME "linux-daw-vst3-test"
    COMPANY_NAME "linux-daw"
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE)

target_sources(linux-daw-vst3-test PRIVATE
    test/vst3_host_test.cpp
    ${CORE_SOURCES})

target_include_directories(linux-daw-vst3-test PRIVATE src)

target_compile_features(linux-daw-vst3-test PRIVATE cxx_std_20)

target_compile_definitions(linux-daw-vst3-test PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_PLUGINHOST_VST3=1)

target_link_libraries(linux-daw-vst3-test
    PRIVATE juce::juce_core juce::juce_events juce::juce_audio_devices juce::juce_audio_formats juce::juce_dsp juce::juce_audio_processors asound
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
