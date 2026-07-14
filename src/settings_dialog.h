#ifndef EROGE_TIMER_SETTINGS_DIALOG_H
#define EROGE_TIMER_SETTINGS_DIALOG_H

#include <windows.h>

#include "app_settings.h"

BOOL settings_dialog_register_class(HINSTANCE instance);
void settings_dialog_show(HWND owner, AppSettings *settings);

#endif
