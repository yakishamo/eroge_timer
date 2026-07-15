#include "app_settings.h"

#include <wchar.h>

void app_settings_init(AppSettings *settings)
{
    settings->size = CLOCK_SIZE_MEDIUM;
    settings->position = CLOCK_POSITION_TOP_LEFT;
    settings->date_format = DATE_FORMAT_NONE;
    settings->time_format = TIME_FORMAT_HOUR_MINUTE_SECOND;
    settings->text_alignment = TEXT_ALIGNMENT_CENTER;
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

    wchar_t date[48] = L"";
    wchar_t clock[16] = L"";
    switch (settings->date_format) {
    case DATE_FORMAT_MONTH_DAY:
        swprintf(date, ARRAYSIZE(date), L"%02u/%02u",
                 time->wMonth, time->wDay);
        break;
    case DATE_FORMAT_YEAR_MONTH_DAY:
        swprintf(date, ARRAYSIZE(date), L"%04u/%02u/%02u",
                 time->wYear, time->wMonth, time->wDay);
        break;
    case DATE_FORMAT_YEAR_MONTH_DAY_WEEKDAY:
        swprintf(date, ARRAYSIZE(date), L"%04u/%02u/%02u (%ls)",
                 time->wYear, time->wMonth, time->wDay,
                 weekdays[time->wDayOfWeek]);
        break;
    case DATE_FORMAT_NONE:
    default:
        break;
    }

    if (settings->time_format == TIME_FORMAT_HOUR_MINUTE) {
        swprintf(clock, ARRAYSIZE(clock), L"%02u:%02u",
                 time->wHour, time->wMinute);
    } else if (settings->time_format == TIME_FORMAT_HOUR_MINUTE_SECOND) {
        swprintf(clock, ARRAYSIZE(clock), L"%02u:%02u:%02u",
                 time->wHour, time->wMinute, time->wSecond);
    }

    if (date[0] != L'\0' && clock[0] != L'\0') {
        swprintf(buffer, buffer_length, L"%ls\n%ls", date, clock);
    } else if (date[0] != L'\0') {
        wcsncpy(buffer, date, buffer_length);
        buffer[buffer_length - 1] = L'\0';
    } else {
        wcsncpy(buffer, clock, buffer_length);
        buffer[buffer_length - 1] = L'\0';
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
