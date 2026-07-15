cmake_minimum_required(VERSION 3.16)

project(eroge_timer VERSION 1.1.0 LANGUAGES C RC)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(eroge_timer WIN32
    resources/app.rc
    src/app_settings.c
    src/clock_renderer_gdi.c
    src/main.c
    src/overlay.c
    src/play_time_dialog.c
    src/play_time_tracker.c
    src/settings_dialog.c
    src/settings_store.c
    src/tray.c
)

target_include_directories(eroge_timer PRIVATE resources)

target_compile_features(eroge_timer PRIVATE c_std_11)
target_compile_definitions(eroge_timer PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    _WIN32_WINNT=0x0601
)

if(MSVC)
    target_compile_options(eroge_timer PRIVATE /W4 /utf-8)
else()
    target_compile_options(eroge_timer PRIVATE -Wall -Wextra -Wpedantic)
    target_link_options(eroge_timer PRIVATE -municode)
endif()

target_link_libraries(eroge_timer PRIVATE user32 gdi32 shell32 comdlg32)

find_program(INNO_SETUP_COMPILER
    NAMES ISCC.exe ISCC
    PATHS
        "$ENV{LOCALAPPDATA}/Programs/Inno Setup 6"
        "C:/Program Files (x86)/Inno Setup 6"
        "C:/Program Files/Inno Setup 6"
)

if(INNO_SETUP_COMPILER)
    add_custom_target(installer ALL
        COMMAND "${INNO_SETUP_COMPILER}"
            "/DAppVersion=${PROJECT_VERSION}"
            "/DBuildDir=${CMAKE_BINARY_DIR}"
            "${CMAKE_SOURCE_DIR}/installer/eroge_timer.iss"
        DEPENDS eroge_timer
        COMMENT "Building Eroge Timer installer for this build"
        VERBATIM
    )
else()
    message(STATUS
        "Inno Setup 6 was not found; install it to enable the installer target")
endif()

install(TARGETS eroge_timer RUNTIME DESTINATION .)
install(FILES
    CHANGELOG.md
    LICENSE
    README.md
    RELEASE_NOTES_v1.1.0.md
    DESTINATION .
)
install(FILES assets/app-icon.png DESTINATION assets)

set(CPACK_GENERATOR ZIP)
set(CPACK_PACKAGE_NAME "eroge_timer")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "eroge_timer-v${PROJECT_VERSION}-windows-x64")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
include(CPack)
