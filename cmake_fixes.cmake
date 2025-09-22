# Fix for missing threading include
add_compile_definitions(_GNU_SOURCE)

# Additional include for missing headers
include_directories(/usr/include)

# Fix CMakeLists.txt for proper compilation
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# Additional system libraries
if(UNIX AND NOT APPLE)
    set(SYSTEM_LIBS ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} rt)
elseif(APPLE)
    set(SYSTEM_LIBS ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})
elseif(WIN32)
    set(SYSTEM_LIBS ${CMAKE_THREAD_LIBS_INIT})
endif()

# Update the main target with system libraries
target_link_libraries(WarriorUSBRecorder PRIVATE
    sdk
    vstgui_support
    ${PORTAUDIO_LIBRARIES}
    ${LIBUSB_LIBRARIES}
    ${SYSTEM_LIBS}
)