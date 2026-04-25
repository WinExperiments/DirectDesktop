cmake_minimum_required(VERSION 3.16)
project(DirectDesktopWin32 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# MSVC specific flags to match .vcxproj
if(MSVC)
    add_definitions(-DUNICODE -D_UNICODE -DDIRECTUI_STATIC -DDUSER_STATIC)
endif()

# Collect source files
file(GLOB SRC_FILES_ROOT "*.cpp")
file(GLOB SRC_FILES_BACKEND "backend/*.cpp")
file(GLOB SRC_FILES_COREUI "coreui/*.cpp")
file(GLOB SRC_FILES_UI "ui/*.cpp")

# Exclude QtEngine.cpp from the main list as it is built separately
list(REMOVE_ITEM SRC_FILES_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/QtEngine.cpp")

set(SRC_FILES 
    ${SRC_FILES_ROOT} 
    ${SRC_FILES_BACKEND} 
    ${SRC_FILES_COREUI} 
    ${SRC_FILES_UI}
    "Include/dui70/DirectUI/DirectUI.cpp"
)

# Main executable target
add_executable(DirectDesktop WIN32 
    ${SRC_FILES} 
    DirectDesktop.rc 
    DirectDesktop.manifest
)

# Apply compiler options to the main target
if(MSVC)
    target_compile_options(DirectDesktop PRIVATE /Zc:wchar_t-)
    target_compile_definitions(DirectDesktop PRIVATE _HAS_STD_BYTE=0)
endif()

# Include directories
target_include_directories(DirectDesktop PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/backend
    ${CMAKE_CURRENT_SOURCE_DIR}/coreui
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
)

# Link the provided and system libraries
target_link_libraries(DirectDesktop PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/dui70.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/DUser.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/Everything64.lib
    user32
    shell32
    gdi32
    comctl32
    ole32
    shlwapi
    dwmapi
    uxtheme
    windowscodecs
    gdiplus
    advapi32
    shcore
    wtsapi32
    powrprof
    oleaut32
)

# Embed Qt Engine if available
if(Qt6_FOUND)
    add_library(DirectDesktopQtEngine STATIC QtEngine.cpp ui/qml.qrc)
    
    # Qt requires _HAS_STD_BYTE=1 and doesn't use the /Zc:wchar_t- flag
    target_compile_definitions(DirectDesktopQtEngine PRIVATE _HAS_STD_BYTE=1 ENABLE_QT DIRECTUI_STATIC DUSER_STATIC)
    set_target_properties(DirectDesktopQtEngine PROPERTIES AUTOMOC ON AUTORCC ON AUTOUIC ON)
    
    target_link_libraries(DirectDesktopQtEngine PRIVATE 
        Qt6::Core 
        Qt6::Gui 
        Qt6::Quick 
        Qt6::Widgets
    )
    
    target_compile_definitions(DirectDesktop PRIVATE ENABLE_QT)
    target_link_libraries(DirectDesktop PRIVATE DirectDesktopQtEngine)
endif()

# Build the C# Control Panel as a post-step
find_program(DOTNET_EXE dotnet)
if(DOTNET_EXE)
    add_custom_command(TARGET DirectDesktop POST_BUILD
        COMMAND ${DOTNET_EXE} build ${CMAKE_SOURCE_DIR}/DirectDesktopSettings/DirectDesktopSettings.csproj -c Release
        COMMENT "Building C# Control Panel..."
    )
endif()
