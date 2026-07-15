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
    DATE_FORMAT_NONE,
    DATE_FORMAT_MONTH_DAY,
    DATE_FORMAT_YEAR_MONTH_DAY,
    DATE_FORMAT_YEAR_MONTH_DAY_WEEKDAY,
    DATE_FORMAT_COUNT
} DateFormat;

typedef enum {
    TIME_FORMAT_HOUR_MINUTE,
    TIME_FORMAT_HOUR_MINUTE_SECOND,
    TIME_FORMAT_NONE,
    TIME_FORMAT_COUNT
} TimeFormat;

typedef enum {
    TEXT_ALIGNMENT_LEFT,
    TEXT_ALIGNMENT_CENTER,
    TEXT_ALIGNMENT_RIGHT,
    TEXT_ALIGNMENT_COUNT
} TextAlignment;

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
    DateFormat date_format;
    TimeFormat time_format;
    TextAlignment text_alignment;
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
