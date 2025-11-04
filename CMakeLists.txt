cmake_minimum_required(VERSION 3.10)
project(candela)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-specific settings
if(WIN32)
    add_compile_definitions(UNICODE _UNICODE WIN32_LEAN_AND_MEAN)
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/tray.cpp
    src/gui.cpp
    src/brightness.cpp
    src/settings.cpp
)

# Resource file for Windows
set(RESOURCES
    candela.rc
)

# Create executable
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${RESOURCES})

# Include the directory containing resource.h
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Link Windows libraries
target_link_libraries(${PROJECT_NAME} 
    user32
    gdi32
    shell32
    comctl32
    ole32
    oleaut32
    dxva2  # For monitor configuration APIs
)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Windows subsystem
set_target_properties(${PROJECT_NAME} PROPERTIES
    LINK_FLAGS "-mwindows"
)