cmake_minimum_required(VERSION 3.15)

project(WarriorUSBRecorder VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# For now, create a simple library without VST3 SDK dependencies
# This allows testing the core components

# Audio libraries (optional for basic testing)
pkg_check_modules(PORTAUDIO portaudio-2.0)
pkg_check_modules(LIBUSB libusb-1.0)

# Core component library (without VST3 dependencies)
add_library(WarriorCore STATIC
    src/USBAudioDetector_simple.cpp
    src/GenreEffectsEngine.cpp
    src/PresetManager.cpp
    src/LowLatencyProcessor.cpp
)

target_include_directories(WarriorCore PUBLIC
    include/
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(WarriorCore PUBLIC
    ${CMAKE_THREAD_LIBS_INIT}
)

# Link optional dependencies if available
if(PORTAUDIO_FOUND)
    target_include_directories(WarriorCore PUBLIC ${PORTAUDIO_INCLUDE_DIRS})
    target_link_libraries(WarriorCore PUBLIC ${PORTAUDIO_LIBRARIES})
    target_compile_definitions(WarriorCore PUBLIC HAVE_PORTAUDIO=1)
endif()

if(LIBUSB_FOUND)
    target_include_directories(WarriorCore PUBLIC ${LIBUSB_INCLUDE_DIRS})
    target_link_libraries(WarriorCore PUBLIC ${LIBUSB_LIBRARIES})
    target_compile_definitions(WarriorCore PUBLIC HAVE_LIBUSB=1)
endif()

# Test executable
add_executable(test_components
    tests/test_components.cpp
)

target_link_libraries(test_components PRIVATE
    WarriorCore
)

# Enable testing
enable_testing()
add_test(NAME ComponentTest COMMAND test_components)

# VST3 Plugin (only if VST3 SDK is available)
# VST3 SDK
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/vst3sdk")
    message(STATUS "VST3 SDK found, building plugin")
    add_subdirectory(third_party/vst3sdk EXCLUDE_FROM_ALL)

    # Main plugin library
    add_library(WarriorUSBRecorderPlugin MODULE
        src/WarriorUSBRecorderProcessor.cpp
        src/WarriorUSBRecorderController.cpp
        src/factory.cpp
    )

    target_include_directories(WarriorUSBRecorderPlugin PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/
    )

    target_link_libraries(WarriorUSBRecorderPlugin PRIVATE
        WarriorCore
        sdk
        vstgui_support
    )

    target_compile_definitions(WarriorUSBRecorderPlugin PRIVATE
        ${PORTAUDIO_CFLAGS_OTHER}
        ${LIBUSB_CFLAGS_OTHER}
    )

    # Set plugin properties
    set_target_properties(WarriorUSBRecorderPlugin PROPERTIES
        BUNDLE TRUE
        BUNDLE_EXTENSION "vst3"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.warrior.usbrecorder"
        MACOSX_BUNDLE_BUNDLE_NAME "Warrior USB Recorder"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
    )

    # Install
    install(TARGETS WarriorUSBRecorderPlugin
        BUNDLE DESTINATION .
        RUNTIME DESTINATION bin
    )
else()
    message(WARNING "VST3 SDK not found. Only building core components and tests.")
    message(STATUS "To build the full plugin, please run the build script which downloads the VST3 SDK.")
endif()