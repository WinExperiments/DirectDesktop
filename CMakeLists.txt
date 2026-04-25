cmake_minimum_required(VERSION 3.16)
project(DirectDesktopSuite)

# 1. New Qt6 Engine (Optional)
find_package(Qt6 COMPONENTS Core Gui Quick Widgets QUIET)
if(NOT Qt6_FOUND)
    message(STATUS "Qt6 not found. DirectDesktop will build without Qt engine.")
endif()

# 2. Main DirectUI + Qt Application
add_subdirectory(DirectDesktop)

# 3. Consolidation (Install to 'bin' folder in build directory)
set(OUTPUT_DIR "bin")

install(TARGETS DirectDesktop RUNTIME DESTINATION "${OUTPUT_DIR}")
if(Qt6_FOUND)
    # Deploy Qt libraries
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
    
    if(WINDEPLOYQT_EXECUTABLE)
        install(CODE "
            execute_process(COMMAND \"${WINDEPLOYQT_EXECUTABLE}\" 
                            --release
                            --no-translations
                            --no-compiler-runtime
                            --no-opengl-sw
                            --no-system-d3d-compiler
                            \"\${CMAKE_INSTALL_PREFIX}/${OUTPUT_DIR}/DirectDesktop.exe\")
        ")
    endif()
endif()

# Copy UI files for DirectUI
install(DIRECTORY "${CMAKE_SOURCE_DIR}/DirectDesktop/ui" DESTINATION "${OUTPUT_DIR}/DirectDesktop")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/DirectDesktop/ImageResources" DESTINATION "${OUTPUT_DIR}/DirectDesktop")

# Copy C# binaries if they exist
find_program(DOTNET_EXE dotnet)
if(DOTNET_EXE)
    install(CODE "
        file(GLOB_RECURSE CS_BINS \"${CMAKE_SOURCE_DIR}/DirectDesktopSettings/bin/Release/net11.0-windows/*\")
        foreach(file \${CS_BINS})
            file(INSTALL DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${OUTPUT_DIR}\" TYPE FILE FILES \"\${file}\")
        endforeach()
    ")
endif()

# 4. CPack Installer Configuration
set(CPACK_PACKAGE_NAME "DirectDesktop")
set(CPACK_PACKAGE_VENDOR "Rectify11")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "DirectDesktop Custom Desktop Shell")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "DirectDesktop")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/.gitignore") # Placeholder if no LICENSE file

# NSIS Specifics
set(CPACK_GENERATOR "NSIS")
set(CPACK_NSIS_DISPLAY_NAME "DirectDesktop")
set(CPACK_NSIS_PACKAGE_NAME "DirectDesktop")
set(CPACK_NSIS_HELP_LINK "https://github.com/Rectify11/DirectDesktop")
set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/Rectify11/DirectDesktop")
set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/DirectDesktop/DirectDesktop.ico")
set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/DirectDesktop/DirectDesktop.ico")

# Shortcuts: Point to the Settings app so users know what it's doing
set(CPACK_NSIS_CREATE_ICONS_EXTRA "
  CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\DirectDesktop Control Panel.lnk' '$INSTDIR\\\\bin\\\\DirectDesktopSettings.exe'
  CreateShortCut '$DESKTOP\\\\DirectDesktop Control Panel.lnk' '$INSTDIR\\\\bin\\\\DirectDesktopSettings.exe'
")
set(CPACK_NSIS_DELETE_ICONS_EXTRA "
  Delete '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\DirectDesktop Control Panel.lnk'
  Delete '$DESKTOP\\\\DirectDesktop Control Panel.lnk'
")

# Registry entry for auto-start or identification
set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
  WriteRegStr HKLM 'Software\\\\DirectDesktop' 'InstallDir' '$INSTDIR'
")

include(CPack)
