#ifndef EROGE_TIMER_TRAY_H
#define EROGE_TIMER_TRAY_H

#include <windows.h>

#define TRAY_CALLBACK_MESSAGE (WM_APP + 1)

enum TrayCommand {
    TRAY_COMMAND_TOGGLE_VISIBILITY = 1001,
    TRAY_COMMAND_EXIT,
    TRAY_COMMAND_SETTINGS,
    TRAY_COMMAND_PLAY_TIME
};

BOOL tray_add(HWND owner);
void tray_remove(void);
void tray_show_menu(HWND owner, BOOL clock_visible);
BOOL tray_is_menu_open(void);
void tray_cancel_menu(void);

#endif
