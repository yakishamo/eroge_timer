#ifndef EROGE_TIMER_PLAY_TIME_DIALOG_H
#define EROGE_TIMER_PLAY_TIME_DIALOG_H

#include <windows.h>

#include "play_time_tracker.h"

BOOL play_time_dialog_register_class(HINSTANCE instance);
void play_time_dialog_show(HWND owner, PlayTimeTracker *tracker);

#endif
