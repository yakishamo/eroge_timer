cmake_minimum_required(VERSION 3.16)

project(eroge_timer VERSION 0.1.0 LANGUAGES C)

add_executable(eroge_timer WIN32
    src/app_settings.c
    src/main.c
    src/overlay.c
    src/settings_dialog.c
    src/settings_store.c
    src/tray.c
)

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

target_link_libraries(eroge_timer PRIVATE user32 gdi32 shell32)
