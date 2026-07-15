#ifndef EROGE_TIMER_OVERLAY_H
#define EROGE_TIMER_OVERLAY_H

#include <windows.h>

#include "app_settings.h"
#include "play_time_tracker.h"

BOOL overlay_register_class(HINSTANCE instance);
HWND overlay_create(HINSTANCE instance, AppSettings *settings,
                    PlayTimeTracker *play_time_tracker);
void overlay_apply_settings(HWND hwnd, const AppSettings *settings);

#endif
