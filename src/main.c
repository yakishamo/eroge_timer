#include <windows.h>

#include "app_settings.h"
#include "overlay.h"
#include "play_time_dialog.h"
#include "play_time_tracker.h"
#include "settings_dialog.h"
#include "settings_store.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous_instance,
                    PWSTR command_line, int show_command)
{
    (void)previous_instance;
    (void)command_line;
    (void)show_command;

    SetProcessDPIAware();

    if (!overlay_register_class(instance) ||
        !settings_dialog_register_class(instance) ||
        !play_time_dialog_register_class(instance)) {
        MessageBoxW(NULL, L"ウィンドウクラスを登録できませんでした。",
                    L"Eroge Timer", MB_ICONERROR);
        return 1;
    }

    AppSettings settings;
    app_settings_init(&settings);
    settings_store_load(&settings);
    PlayTimeTracker play_time_tracker;
    play_time_tracker_init(&play_time_tracker);
    if (overlay_create(instance, &settings, &play_time_tracker) == NULL) {
        MessageBoxW(NULL, L"時計ウィンドウを作成できませんでした。",
                    L"Eroge Timer", MB_ICONERROR);
        return 1;
    }

    MSG message;
    while (GetMessageW(&message, NULL, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    play_time_tracker_shutdown(&play_time_tracker);
    return (int)message.wParam;
}
