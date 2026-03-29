cmake_minimum_required(VERSION 3.22)
project(MFAmp VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

option(MFAMP_ASIO "Enable ASIO support (requires ASIO SDK)" ON)
option(MFAMP_INSTALL_LOCAL "Copy plugin to local VST3 directories after build" ON)

# Path to the extracted ASIO SDK folder (must contain "common/asio.h")
# Download from https://www.steinberg.net/asiosdk
set(JUCE_ASIO_SUPPORT_PATH "C:/SDKs/ASIOSDK" CACHE PATH "Path to ASIO SDK")

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.9
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(JUCE)

FetchContent_Declare(
    RTNeural
    GIT_REPOSITORY https://github.com/jatinchowdhury18/RTNeural.git
    GIT_TAG        main
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(RTNeural)

juce_add_plugin(MFAmp
    VERSION                   1.0.0
    COMPANY_NAME              "GaresonC"
    IS_SYNTH                  FALSE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   ${MFAMP_INSTALL_LOCAL}
    VST3_COPY_DIR             "C:/Program Files/Steinberg/VstPlugins"
    PLUGIN_MANUFACTURER_CODE  Grsc
    PLUGIN_CODE               GAmp
    FORMATS                   VST3 Standalone
    PRODUCT_NAME              "MF AMP"
)

juce_generate_juce_header(MFAmp)

# Bundle all IR WAV files from the IRs/ directory into the binary
file(GLOB IR_FILES "${CMAKE_CURRENT_SOURCE_DIR}/IRs/*.wav")
juce_add_binary_data(MFAmpIRData SOURCES ${IR_FILES})

# Bundle the default neural amp model into the binary
juce_add_binary_data(MFAmpModelData SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Models/6505Plus_Red_DirectOut.json"
)

target_sources(MFAmp
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/NeuralAmpProcessor.cpp
        Source/EQProcessor.cpp
        Source/StageProcessor.cpp
        Source/Tuner.cpp
        Source/IRLoader.cpp
        Source/KnobRangesDialog.cpp
        Source/SectionColoursDialog.cpp
        Source/CloudSeed/Parameters.cpp
        Source/CloudSeed/DSP/Biquad.cpp
        Source/CloudSeed/DSP/RandomBuffer.cpp
)

if(MFAMP_ASIO)
    set(MFAMP_ASIO_FLAG 1)
else()
    set(MFAMP_ASIO_FLAG 0)
endif()

target_compile_definitions(MFAmp
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_MODAL_LOOPS_PERMITTED=1
        JUCE_ASIO=${MFAMP_ASIO_FLAG}
    PRIVATE
        BUFFER_SIZE=128
        MAX_STR_SIZE=64
)

set(MFAMP_INCLUDE_DIRS
    "${rtneural_SOURCE_DIR}/modules/json"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/CloudSeed"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/CloudSeed/DSP"
)
if(MFAMP_ASIO)
    list(APPEND MFAMP_INCLUDE_DIRS "${JUCE_ASIO_SUPPORT_PATH}/common")
endif()

target_include_directories(MFAmp
    PRIVATE
        ${MFAMP_INCLUDE_DIRS}
)

target_link_libraries(MFAmp
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        RTNeural
        MFAmpIRData
        MFAmpModelData
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if(MFAMP_INSTALL_LOCAL)
    # Also copy VST3 to Common Files after build
    add_custom_command(TARGET MFAmp_VST3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_BINARY_DIR}/MFAmp_artefacts/$<CONFIG>/VST3/MF AMP.vst3"
            "C:/Program Files/Common Files/VST3/MF AMP.vst3"
        COMMENT "Installing VST3 to Common Files/VST3..."
    )
endif()
