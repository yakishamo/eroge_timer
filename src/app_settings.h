#ifndef EROGE_TIMER_APP_SETTINGS_H
#define EROGE_TIMER_APP_SETTINGS_H

#include <windows.h>
#include <stddef.h>

typedef enum {
    CLOCK_SIZE_SMALL,
    CLOCK_SIZE_MEDIUM,
    CLOCK_SIZE_LARGE
} ClockSize;

typedef enum {
    CLOCK_POSITION_TOP_LEFT,
    CLOCK_POSITION_TOP_RIGHT,
    CLOCK_POSITION_BOTTOM_LEFT,
    CLOCK_POSITION_BOTTOM_RIGHT
} ClockPosition;

typedef enum {
    CLOCK_DISPLAY_TIME,
    CLOCK_DISPLAY_TIME_SECONDS,
    CLOCK_DISPLAY_DATE_TIME,
    CLOCK_DISPLAY_DATE_TIME_YEAR,
    CLOCK_DISPLAY_DATE_WEEKDAY_TIME,
    CLOCK_DISPLAY_COUNT
} ClockDisplay;

typedef enum {
    RENDER_RESOLUTION_NATIVE,
    RENDER_RESOLUTION_640_480,
    RENDER_RESOLUTION_800_600,
    RENDER_RESOLUTION_1024_768,
    RENDER_RESOLUTION_1280_720,
    RENDER_RESOLUTION_1280_800,
    RENDER_RESOLUTION_1366_768,
    RENDER_RESOLUTION_1600_900,
    RENDER_RESOLUTION_1920_1080,
    RENDER_RESOLUTION_COUNT
} RenderResolution;

typedef struct {
    ClockSize size;
    ClockPosition position;
    ClockDisplay display;
    BOOL outline;
    BOOL shadow;
    RenderResolution render_resolution;
    wchar_t font[LF_FACESIZE];
} AppSettings;

void app_settings_init(AppSettings *settings);
int app_settings_font_height(const AppSettings *settings);
void app_settings_format_clock(const AppSettings *settings,
                               const SYSTEMTIME *time,
                               wchar_t *buffer, size_t buffer_length);
BOOL app_settings_render_size(const AppSettings *settings,
                              int *width, int *height);

#endif
