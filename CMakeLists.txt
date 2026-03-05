cmake_minimum_required(VERSION 3.22)
project(MFAmp VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.9
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(MFAmp
    VERSION                   1.0.0
    COMPANY_NAME              "GaresonC"
    IS_SYNTH                  FALSE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   TRUE
    PLUGIN_MANUFACTURER_CODE  Grsc
    PLUGIN_CODE               GAmp
    FORMATS                   VST3 Standalone
    PRODUCT_NAME              "MF AMP"
)

juce_generate_juce_header(MFAmp)

target_sources(MFAmp
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/AmpProcessor.cpp
        Source/EQProcessor.cpp
        Source/StageProcessor.cpp
        Source/Tuner.cpp
        Source/IRLoader.cpp
        Source/PitchShifter.cpp
)

target_compile_definitions(MFAmp
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_MODAL_LOOPS_PERMITTED=1
)

target_link_libraries(MFAmp
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
