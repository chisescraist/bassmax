cmake_minimum_required(VERSION 3.22)
project(ChisescraistVOX VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
elseif (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    message(FATAL_ERROR "JUCE not found. Clone JUCE into ./JUCE or configure with -DJUCE_DIR=C:/path/to/JUCE")
endif()

juce_add_plugin(ChisescraistVOX
    COMPANY_NAME "Chisescraist Audio"
    BUNDLE_ID "audio.chisescraist.vox"
    PLUGIN_MANUFACTURER_CODE Chis
    PLUGIN_CODE Cvox
    FORMATS VST3 Standalone
    PRODUCT_NAME "CHISESCRAIST VOX"
    DESCRIPTION "Real-time vocal performance FX for Tech House"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(ChisescraistVOX PRIVATE
    Source/PluginProcessor.cpp Source/PluginProcessor.h
    Source/PluginEditor.cpp Source/PluginEditor.h
    Source/DSP/VoxEngine.cpp Source/DSP/VoxEngine.h
    Source/DSP/DarkProcessor.h Source/DSP/RadioProcessor.h
    Source/DSP/StutterProcessor.h Source/DSP/DelayProcessor.h
    Source/DSP/ReverbProcessor.h Source/DSP/FilterProcessor.h
    Source/UI/LookAndFeel.cpp Source/UI/LookAndFeel.h
)

target_compile_definitions(ChisescraistVOX PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(ChisescraistVOX PRIVATE
    juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags)
