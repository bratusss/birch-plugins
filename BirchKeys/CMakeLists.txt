cmake_minimum_required(VERSION 3.22)
project(BirchKeys VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

set(DSP_SOURCES
    Source/dsp/Instruments.cpp
    Source/dsp/Voice.cpp
    Source/dsp/Part.cpp
    Source/dsp/Effects.cpp
    Source/dsp/KeysSynth.cpp)

juce_add_plugin(BirchKeys
    COMPANY_NAME "Bratus"
    PRODUCT_NAME "Birch Keys"
    PLUGIN_NAME "Birch Keys"
    PLUGIN_MANUFACTURER_CODE Brts
    PLUGIN_CODE Bky1
    FORMATS VST3 Standalone
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES Instrument Synth
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_add_binary_data(BirchKeysData SOURCES
    Resources/SedgwickAveDisplay-Regular.ttf
    Resources/birch_logo.png)

target_sources(BirchKeys PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/BirchStyle.cpp
    Source/Presets.cpp
    ${DSP_SOURCES})

target_compile_definitions(BirchKeys PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(BirchKeys
    PRIVATE
        BirchKeysData
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Offline renderer for checking the instruments without a host
add_executable(BirchKeysRender tools/Render.cpp ${DSP_SOURCES})
