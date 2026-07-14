#ifndef EROGE_TIMER_OVERLAY_H
#define EROGE_TIMER_OVERLAY_H

#include <windows.h>

#include "app_settings.h"

BOOL overlay_register_class(HINSTANCE instance);
HWND overlay_create(HINSTANCE instance, AppSettings *settings);
void overlay_apply_settings(HWND hwnd, const AppSettings *settings);

#endif
