cmake_minimum_required(VERSION 3.25)
project(Claymore VERSION 1.0.0)

# C++17 is required for this project
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch JUCE 8.0.12 via FetchContent
include(FetchContent)

FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.12
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

# Add the plugin target
juce_add_plugin(Claymore
    COMPANY_NAME                "Nathan McMillan"
    PLUGIN_MANUFACTURER_CODE    NMcM
    PLUGIN_CODE                 Clyr
    FORMATS                     VST3 AU
    PRODUCT_NAME                "Claymore"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     TRUE
    VST3_CATEGORIES             "Fx" "Distortion"
    AU_MAIN_TYPE                kAudioUnitType_Effect
    BUNDLE_ID                   com.nathanmcmillan.claymore
)

# Embedded assets — fonts and logos for GUI
juce_add_binary_data(ClaymoreAssets
    NAMESPACE ClaymoreAssets
    SOURCES
        Assets/Fonts/CormorantGaramond.ttf
        Assets/Fonts/DMSans-Regular.ttf
        Assets/Fonts/DMSans-Light.ttf
)

# Explicit source list — do NOT use GLOB_RECURSE
target_sources(Claymore PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/look/ClaymoreTheme.cpp
)

# Compile definitions
target_compile_definitions(Claymore
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

# Link libraries
target_link_libraries(Claymore
    PRIVATE
        ClaymoreAssets
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# C++17 standard (explicit per-target)
target_compile_features(Claymore PRIVATE cxx_std_17)
