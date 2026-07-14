#include "settings_store.h"

#include <shlobj.h>
#include <wchar.h>

static BOOL get_settings_path(wchar_t *path, size_t path_length,
                              BOOL create_directory)
{
    wchar_t app_data[MAX_PATH];
    if (SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT,
                         app_data) != S_OK) {
        return FALSE;
    }

    wchar_t directory[MAX_PATH];
    int directory_length = swprintf(directory, ARRAYSIZE(directory),
                                    L"%ls\\ErogeTimer", app_data);
    if (directory_length < 0 || (size_t)directory_length >= ARRAYSIZE(directory)) {
        return FALSE;
    }

    if (create_directory &&
        !CreateDirectoryW(directory, NULL) &&
        GetLastError() != ERROR_ALREADY_EXISTS) {
        return FALSE;
    }

    int result = swprintf(path, path_length, L"%ls\\settings.ini", directory);
    return result >= 0 && (size_t)result < path_length;
}

static int read_bounded_int(const wchar_t *path, const wchar_t *key,
                            int fallback, int minimum, int maximum)
{
    int value = GetPrivateProfileIntW(L"Clock", key, fallback, path);
    return value >= minimum && value <= maximum ? value : fallback;
}

BOOL settings_store_load(AppSettings *settings)
{
    wchar_t path[MAX_PATH];
    if (!get_settings_path(path, ARRAYSIZE(path), FALSE)) {
        return FALSE;
    }

    settings->size = (ClockSize)read_bounded_int(
        path, L"Size", settings->size, CLOCK_SIZE_SMALL, CLOCK_SIZE_LARGE);
    settings->position = (ClockPosition)read_bounded_int(
        path, L"Position", settings->position,
        CLOCK_POSITION_TOP_LEFT, CLOCK_POSITION_BOTTOM_RIGHT);
    settings->display = (ClockDisplay)read_bounded_int(
        path, L"Display", settings->display,
        CLOCK_DISPLAY_TIME, CLOCK_DISPLAY_COUNT - 1);
    settings->outline = read_bounded_int(
        path, L"Outline", settings->outline, FALSE, TRUE);
    settings->shadow = read_bounded_int(
        path, L"Shadow", settings->shadow, FALSE, TRUE);
    wchar_t default_font[LF_FACESIZE];
    wcsncpy(default_font, settings->font, LF_FACESIZE);
    default_font[LF_FACESIZE - 1] = L'\0';
    GetPrivateProfileStringW(L"Clock", L"Font", default_font,
                             settings->font, LF_FACESIZE, path);
    settings->font[LF_FACESIZE - 1] = L'\0';
    return TRUE;
}

static BOOL write_int(const wchar_t *path, const wchar_t *key, int value)
{
    wchar_t text[16];
    swprintf(text, ARRAYSIZE(text), L"%d", value);
    return WritePrivateProfileStringW(L"Clock", key, text, path);
}

BOOL settings_store_save(const AppSettings *settings)
{
    wchar_t path[MAX_PATH];
    if (!get_settings_path(path, ARRAYSIZE(path), TRUE)) {
        return FALSE;
    }

    BOOL success = TRUE;
    success = write_int(path, L"Size", settings->size) && success;
    success = write_int(path, L"Position", settings->position) && success;
    success = write_int(path, L"Display", settings->display) && success;
    success = write_int(path, L"Outline", settings->outline) && success;
    success = write_int(path, L"Shadow", settings->shadow) && success;
    success = WritePrivateProfileStringW(
        L"Clock", L"Font", settings->font, path) && success;
    return success;
}
