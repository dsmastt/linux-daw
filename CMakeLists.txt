cmake_minimum_required(VERSION 3.24)

project(linux_daw VERSION 0.1.0 LANGUAGES C CXX)

include(FetchContent)

set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.2
    GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(JUCE)

juce_add_console_app(linux-daw
    PRODUCT_NAME "linux-daw"
    COMPANY_NAME "linux-daw"
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE)

target_sources(linux-daw PRIVATE
    src/main.cpp
    src/AudioDeviceDiscovery.cpp
    src/AudioEngine.cpp
    src/CompressorProcessor.cpp
    src/ConvolutionProcessor.cpp
    src/DistortionProcessor.cpp
    src/GainProcessor.cpp
    src/NoiseGateProcessor.cpp
    src/ParametricEqProcessor.cpp
    src/ProcessorChain.cpp
    src/ReverbProcessor.cpp)

target_compile_features(linux-daw PRIVATE cxx_std_20)

target_compile_definitions(linux-daw PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)

target_link_libraries(linux-daw
    PRIVATE juce::juce_core juce::juce_audio_devices juce::juce_audio_formats juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
