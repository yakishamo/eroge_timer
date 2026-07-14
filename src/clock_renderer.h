#ifndef EROGE_TIMER_CLOCK_RENDERER_H
#define EROGE_TIMER_CLOCK_RENDERER_H

#include <windows.h>

#include "app_settings.h"

/* Backend-neutral rendering interface. */
typedef struct ClockRenderer ClockRenderer;

ClockRenderer *clock_renderer_create(void);
void clock_renderer_destroy(ClockRenderer *renderer);
BOOL clock_renderer_measure(ClockRenderer *renderer,
                            const AppSettings *settings,
                            SIZE *size);
BOOL clock_renderer_render(ClockRenderer *renderer, HWND hwnd,
                           const AppSettings *settings);

#endif
