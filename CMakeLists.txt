cmake_minimum_required(VERSION 3.15)

project(KosmoVibe VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch JUCE via FetchContent
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        master # Use master to get macOS 15 Sequoia compilation fixes
)
FetchContent_MakeAvailable(JUCE)

# Create the Audio Plugin target
juce_add_plugin(KosmoVibe
    PRODUCT_NAME "KosmoVibe"
    VERSION "1.0.0"
    COMPANY_NAME "KosmoAudio"
    BUNDLE_ID "com.kosmoaudio.kosmovibe"
    FORMATS AU VST3 Standalone
    PLUGIN_MANUFACTURER_CODE "KsmV"
    PLUGIN_CODE "KsVb"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
)

# Generate JuceHeader.h for backwards compatibility
juce_generate_juce_header(KosmoVibe)

# Source Files
target_sources(KosmoVibe PRIVATE
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp
    Source/RibbonComponent.h
    Source/DSP/KosmoVCO.h
    Source/DSP/KosmoVCO.cpp
    Source/DSP/KosmoLFO.h
    Source/DSP/KosmoLFO.cpp
    Source/DSP/KosmoVCF.h
    Source/DSP/KosmoVCF.cpp
    Source/DSP/KosmoDelay.h
    Source/DSP/KosmoDelay.cpp
)

# JUCE Module Linking
target_link_libraries(KosmoVibe PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# Fix VST3 compilation error regarding VST2 replacement
target_compile_definitions(KosmoVibe PRIVATE JUCE_VST3_CAN_REPLACE_VST2=0)
