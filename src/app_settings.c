#include "app_settings.h"

#include <wchar.h>

void app_settings_init(AppSettings *settings)
{
    settings->size = CLOCK_SIZE_MEDIUM;
    settings->position = CLOCK_POSITION_TOP_LEFT;
    settings->display = CLOCK_DISPLAY_TIME_SECONDS;
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
