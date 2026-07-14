#ifndef EROGE_TIMER_SETTINGS_STORE_H
#define EROGE_TIMER_SETTINGS_STORE_H

#include <windows.h>

#include "app_settings.h"

BOOL settings_store_load(AppSettings *settings);
BOOL settings_store_save(const AppSettings *settings);

#endif
