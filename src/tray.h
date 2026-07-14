#ifndef EROGE_TIMER_TRAY_H
#define EROGE_TIMER_TRAY_H

#include <windows.h>

#define TRAY_CALLBACK_MESSAGE (WM_APP + 1)

enum TrayCommand {
    TRAY_COMMAND_TOGGLE_VISIBILITY = 1001,
    TRAY_COMMAND_EXIT,
    TRAY_COMMAND_SETTINGS
};

BOOL tray_add(HWND owner);
void tray_remove(void);
void tray_show_menu(HWND owner, BOOL clock_visible);

#endif
