cmake_minimum_required(VERSION 3.22)
project(BirchBox VERSION 0.2.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(BirchBox
    COMPANY_NAME "Bratus"
    PRODUCT_NAME "Birch Box"
    PLUGIN_NAME "Birch Box"
    PLUGIN_MANUFACTURER_CODE Brts
    PLUGIN_CODE Bbx1
    FORMATS VST3 Standalone
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES Fx Distortion
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_add_binary_data(BirchBoxData SOURCES
    Resources/SedgwickAveDisplay-Regular.ttf
    Resources/birch_logo.png)

target_sources(BirchBox PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/BirchEngine.cpp)

target_compile_definitions(BirchBox PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_USE_MP3AUDIOFORMAT=1)

target_link_libraries(BirchBox
    PRIVATE
        BirchBoxData
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
