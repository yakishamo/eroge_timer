#include "app_settings.h"

#include <wchar.h>

void app_settings_init(AppSettings *settings)
{
    settings->size = CLOCK_SIZE_MEDIUM;
    settings->position = CLOCK_POSITION_TOP_LEFT;
    settings->display = CLOCK_DISPLAY_TIME_SECONDS;
    settings->outline = TRUE;
    settings->shadow = TRUE;
    settings->render_resolution = RENDER_RESOLUTION_NATIVE;
    wcsncpy(settings->font, L"Segoe UI", LF_FACESIZE);
    settings->font[LF_FACESIZE - 1] = L'\0';
}

int app_settings_font_height(const AppSettings *settings)
{
    static const int heights[] = {-30, -42, -56};
    return heights[settings->size];
}

void app_settings_format_clock(const AppSettings *settings,
                               const SYSTEMTIME *time,
                               wchar_t *buffer, size_t buffer_length)
{
    static const wchar_t *weekdays[] = {
        L"日", L"月", L"火", L"水", L"木", L"金", L"土"
    };

    switch (settings->display) {
    case CLOCK_DISPLAY_TIME:
        swprintf(buffer, buffer_length, L"%02u:%02u",
                 time->wHour, time->wMinute);
        break;
    case CLOCK_DISPLAY_DATE_TIME:
        swprintf(buffer, buffer_length, L"%02u/%02u %02u:%02u",
                 time->wMonth, time->wDay, time->wHour, time->wMinute);
        break;
    case CLOCK_DISPLAY_DATE_TIME_YEAR:
        swprintf(buffer, buffer_length, L"%04u/%02u/%02u %02u:%02u",
                 time->wYear, time->wMonth, time->wDay,
                 time->wHour, time->wMinute);
        break;
    case CLOCK_DISPLAY_DATE_WEEKDAY_TIME:
        swprintf(buffer, buffer_length, L"%04u/%02u/%02u (%ls) %02u:%02u",
                 time->wYear, time->wMonth, time->wDay,
                 weekdays[time->wDayOfWeek], time->wHour, time->wMinute);
        break;
    case CLOCK_DISPLAY_TIME_SECONDS:
    default:
        swprintf(buffer, buffer_length, L"%02u:%02u:%02u",
                 time->wHour, time->wMinute, time->wSecond);
        break;
    }
}

BOOL app_settings_render_size(const AppSettings *settings,
                              int *width, int *height)
{
    static const int widths[] = {
        0, 640, 800, 1024, 1280, 1280, 1366, 1600, 1920
    };
    static const int heights[] = {
        0, 480, 600, 768, 720, 800, 768, 900, 1080
    };
    if (settings->render_resolution <= RENDER_RESOLUTION_NATIVE ||
        settings->render_resolution >= RENDER_RESOLUTION_COUNT) {
        return FALSE;
    }
    *width = widths[settings->render_resolution];
    *height = heights[settings->render_resolution];
    return TRUE;
}
