cmake_minimum_required(VERSION 3.16)

project(eroge_timer VERSION 1.0.0 LANGUAGES C RC)

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

install(TARGETS eroge_timer RUNTIME DESTINATION .)
install(FILES
    CHANGELOG.md
    LICENSE
    README.md
    RELEASE_NOTES_v1.0.0.md
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
