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

typedef struct {
    ClockSize size;
    ClockPosition position;
    ClockDisplay display;
    wchar_t font[LF_FACESIZE];
} AppSettings;

void app_settings_init(AppSettings *settings);
int app_settings_clock_width(const AppSettings *settings);
int app_settings_clock_height(const AppSettings *settings);
int app_settings_font_height(const AppSettings *settings);
void app_settings_format_clock(const AppSettings *settings,
                               const SYSTEMTIME *time,
                               wchar_t *buffer, size_t buffer_length);

#endif
