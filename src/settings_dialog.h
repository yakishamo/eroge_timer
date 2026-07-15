#ifndef EROGE_TIMER_SETTINGS_DIALOG_H
#define EROGE_TIMER_SETTINGS_DIALOG_H

#include <windows.h>

#include "app_settings.h"
#include "play_time_tracker.h"

BOOL settings_dialog_register_class(HINSTANCE instance);
void settings_dialog_show(HWND owner, AppSettings *settings,
                          PlayTimeTracker *play_time_tracker);

#endif
