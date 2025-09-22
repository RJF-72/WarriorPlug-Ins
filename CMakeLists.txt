cmake_minimum_required(VERSION 3.15)
project(WarriorPlugIns VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.12
)
FetchContent_MakeAvailable(JUCE)

# Common settings for all plugins
set(PLUGIN_FORMATS VST3 AU Standalone)
set(PLUGIN_COMPANY_NAME "Warrior Audio")
set(PLUGIN_COMPANY_WEBSITE "https://github.com/RJF-72/WarriorPlug-Ins")
set(PLUGIN_COMPANY_EMAIL "warrior@audio.com")

# Function to create a plugin
function(create_warrior_plugin PLUGIN_NAME PLUGIN_DESCRIPTION)
    juce_add_plugin(${PLUGIN_NAME}
        VERSION 1.0.0
        COMPANY_NAME ${PLUGIN_COMPANY_NAME}
        COMPANY_WEBSITE ${PLUGIN_COMPANY_WEBSITE}
        COMPANY_EMAIL ${PLUGIN_COMPANY_EMAIL}
        PLUGIN_MANUFACTURER_CODE Warr
        PLUGIN_CODE ${ARGN}
        FORMATS ${PLUGIN_FORMATS}
        PRODUCT_NAME ${PLUGIN_NAME}
        DESCRIPTION ${PLUGIN_DESCRIPTION}
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD TRUE
        PLUGIN_MANUFACTURER "Warrior Audio"
        VST3_CATEGORIES "Fx"
        AU_MAIN_TYPE "kAudioUnitType_Effect"
    )

    # Add source files
    target_sources(${PLUGIN_NAME} PRIVATE
        src/${PLUGIN_NAME}/PluginProcessor.cpp
        src/${PLUGIN_NAME}/PluginEditor.cpp
        src/${PLUGIN_NAME}/PluginProcessor.h
        src/${PLUGIN_NAME}/PluginEditor.h
    )

    # Link libraries
    target_link_libraries(${PLUGIN_NAME} PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
        WarriorCommon
    )

    # Compile definitions
    target_compile_definitions(${PLUGIN_NAME} PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
    )
endfunction()

# Common library for shared code
juce_add_module(WarriorCommon)

target_sources(WarriorCommon PRIVATE
    src/common/DSPUtils.cpp
    src/common/DSPUtils.h
    src/common/WarriorLookAndFeel.cpp
    src/common/WarriorLookAndFeel.h
)

target_link_libraries(WarriorCommon PUBLIC
    juce::juce_audio_basics
    juce::juce_core
    juce::juce_graphics
    juce::juce_gui_basics
)

# Create plugins
create_warrior_plugin(WarriorReverb "Advanced reverb processor with multiple algorithms" WrRv)
create_warrior_plugin(WarriorDelay "Multi-tap delay with modulation and filtering" WrDl)
create_warrior_plugin(WarriorDistortion "Multi-stage distortion with tube modeling" WrDs)
create_warrior_plugin(WarriorCompressor "Transparent compressor with side-chain support" WrCm)