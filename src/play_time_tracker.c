#include "play_time_tracker.h"

#include <shlobj.h>
#include <stdlib.h>
#include <wchar.h>

#define DEFAULT_IDLE_TIMEOUT_SECONDS 60
#define SAVE_INTERVAL_MS 30000ULL
#define NO_ACTIVE_RECORD ((size_t)-1)

static BOOL get_settings_path(wchar_t *path, size_t path_length,
                              BOOL create_directory)
{
    wchar_t app_data[MAX_PATH];
    if (SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT,
                         app_data) != S_OK) {
        return FALSE;
    }
    wchar_t directory[MAX_PATH];
    int length = swprintf(directory, ARRAYSIZE(directory),
                          L"%ls\\ErogeTimer", app_data);
    if (length < 0 || (size_t)length >= ARRAYSIZE(directory)) return FALSE;
    if (create_directory && !CreateDirectoryW(directory, NULL) &&
        GetLastError() != ERROR_ALREADY_EXISTS) {
        return FALSE;
    }
    length = swprintf(path, path_length, L"%ls\\settings.ini", directory);
    return length >= 0 && (size_t)length < path_length;
}

static BOOL get_history_path(wchar_t *path, size_t path_length,
                             BOOL create_directory)
{
    if (!get_settings_path(path, path_length, create_directory)) return FALSE;
    wchar_t *file_name = wcsrchr(path, L'\\');
    if (file_name == NULL) return FALSE;
    const wchar_t history_name[] = L"\\play_time_history.ini";
    size_t prefix_length = (size_t)(file_name - path);
    if (prefix_length + ARRAYSIZE(history_name) > path_length) return FALSE;
    wcscpy(file_name, history_name);
    return TRUE;
}

static ULONGLONG read_total_milliseconds(const wchar_t *path)
{
    wchar_t text[32];
    GetPrivateProfileStringW(L"PlayTime", L"TotalMilliseconds", L"0",
                             text, ARRAYSIZE(text), path);
    wchar_t *end = NULL;
    unsigned long long value = wcstoull(text, &end, 10);
    return end != text ? (ULONGLONG)value : 0;
}

static ULONGLONG read_unsigned_value(const wchar_t *path,
                                    const wchar_t *section,
                                    const wchar_t *key)
{
    wchar_t text[32];
    GetPrivateProfileStringW(section, key, L"0",
                             text, ARRAYSIZE(text), path);
    wchar_t *end = NULL;
    unsigned long long value = wcstoull(text, &end, 10);
    return end != text ? (ULONGLONG)value : 0;
}

static BOOL ensure_record_capacity(PlayTimeTracker *tracker, size_t required)
{
    if (required <= tracker->record_capacity) return TRUE;
    size_t capacity = tracker->record_capacity == 0
        ? 8 : tracker->record_capacity * 2;
    while (capacity < required) capacity *= 2;
    PlayTimeRecord *records = realloc(
        tracker->records, capacity * sizeof(*records));
    if (records == NULL) return FALSE;
    tracker->records = records;
    tracker->record_capacity = capacity;
    return TRUE;
}

static BOOL append_record(PlayTimeTracker *tracker,
                          const wchar_t *executable_path,
                          ULONGLONG registered_file_time,
                          ULONGLONG total_milliseconds)
{
    if (!ensure_record_capacity(tracker, tracker->record_count + 1)) {
        return FALSE;
    }
    PlayTimeRecord *record = &tracker->records[tracker->record_count++];
    ZeroMemory(record, sizeof(*record));
    lstrcpynW(record->executable_path, executable_path,
              ARRAYSIZE(record->executable_path));
    record->registered_file_time = registered_file_time;
    record->total_milliseconds = total_milliseconds;
    return TRUE;
}

static void load_history(PlayTimeTracker *tracker)
{
    wchar_t path[MAX_PATH];
    if (!get_history_path(path, ARRAYSIZE(path), FALSE)) return;
    int count = GetPrivateProfileIntW(L"History", L"Count", 0, path);
    if (count < 0 || count > 10000) return;
    for (int i = 0; i < count; ++i) {
        wchar_t section[32];
        swprintf(section, ARRAYSIZE(section), L"Record%d", i);
        wchar_t executable[MAX_PATH];
        GetPrivateProfileStringW(section, L"Executable", L"",
                                 executable, ARRAYSIZE(executable), path);
        if (executable[0] == L'\0') continue;
        if (!append_record(
                tracker, executable,
                read_unsigned_value(path, section, L"RegisteredFileTime"),
                read_unsigned_value(path, section, L"TotalMilliseconds"))) {
            break;
        }
    }
}

static BOOL save_history(const PlayTimeTracker *tracker)
{
    wchar_t path[MAX_PATH];
    if (!get_history_path(path, ARRAYSIZE(path), TRUE)) return FALSE;
    wchar_t number[32];
    swprintf(number, ARRAYSIZE(number), L"%llu",
             (unsigned long long)tracker->record_count);
    BOOL success = WritePrivateProfileStringW(
        L"History", L"Count", number, path);
    for (size_t i = 0; i < tracker->record_count; ++i) {
        wchar_t section[32];
        swprintf(section, ARRAYSIZE(section), L"Record%llu",
                 (unsigned long long)i);
        const PlayTimeRecord *record = &tracker->records[i];
        success = WritePrivateProfileStringW(
            section, L"Executable", record->executable_path, path) && success;
        swprintf(number, ARRAYSIZE(number), L"%llu",
                 (unsigned long long)record->registered_file_time);
        success = WritePrivateProfileStringW(
            section, L"RegisteredFileTime", number, path) && success;
        swprintf(number, ARRAYSIZE(number), L"%llu",
                 (unsigned long long)record->total_milliseconds);
        success = WritePrivateProfileStringW(
            section, L"TotalMilliseconds", number, path) && success;
    }
    return success;
}

static BOOL write_utf8(HANDLE file, const wchar_t *text)
{
    int byte_count = WideCharToMultiByte(
        CP_UTF8, 0, text, -1, NULL, 0, NULL, NULL);
    if (byte_count <= 1) return byte_count == 1;
    char *bytes = malloc((size_t)byte_count);
    if (bytes == NULL) return FALSE;
    WideCharToMultiByte(CP_UTF8, 0, text, -1,
                        bytes, byte_count, NULL, NULL);
    DWORD written = 0;
    BOOL success = WriteFile(file, bytes, (DWORD)(byte_count - 1),
                             &written, NULL) &&
                   written == (DWORD)(byte_count - 1);
    free(bytes);
    return success;
}

static wchar_t *quote_csv_field(const wchar_t *text)
{
    size_t length = wcslen(text);
    wchar_t *quoted = malloc((length * 2 + 3) * sizeof(*quoted));
    if (quoted == NULL) return NULL;
    wchar_t *output = quoted;
    *output++ = L'"';
    for (const wchar_t *input = text; *input != L'\0'; ++input) {
        if (*input == L'"') *output++ = L'"';
        *output++ = *input;
    }
    *output++ = L'"';
    *output = L'\0';
    return quoted;
}

BOOL play_time_tracker_export_csv(const PlayTimeTracker *tracker,
                                  const wchar_t *path)
{
    HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return FALSE;
    const BYTE bom[] = {0xef, 0xbb, 0xbf};
    DWORD written = 0;
    BOOL success = WriteFile(file, bom, sizeof(bom), &written, NULL) &&
                   written == sizeof(bom) &&
                   write_utf8(file,
                       L"登録日時,実行ファイル,実行ファイルのパス,"
                       L"プレイ時間（秒）,プレイ時間（時:分:秒）\r\n");
    for (size_t i = 0; success && i < tracker->record_count; ++i) {
        const PlayTimeRecord *record = &tracker->records[i];
        ULARGE_INTEGER timestamp;
        timestamp.QuadPart = record->registered_file_time;
        FILETIME utc = {timestamp.LowPart, timestamp.HighPart};
        FILETIME local;
        SYSTEMTIME registered = {0};
        FileTimeToLocalFileTime(&utc, &local);
        FileTimeToSystemTime(&local, &registered);
        const wchar_t *file_name = wcsrchr(record->executable_path, L'\\');
        file_name = file_name != NULL ? file_name + 1
                                      : record->executable_path;
        wchar_t *quoted_name = quote_csv_field(file_name);
        wchar_t *quoted_path = quote_csv_field(record->executable_path);
        if (quoted_name == NULL || quoted_path == NULL) {
            free(quoted_name);
            free(quoted_path);
            success = FALSE;
            break;
        }
        size_t row_length = wcslen(quoted_name) + wcslen(quoted_path) + 96;
        wchar_t *row = malloc(row_length * sizeof(*row));
        if (row == NULL) {
            free(quoted_name);
            free(quoted_path);
            success = FALSE;
            break;
        }
        ULONGLONG total_seconds = record->total_milliseconds / 1000;
        swprintf(row, row_length,
                 L"%04u/%02u/%02u %02u:%02u:%02u,%ls,%ls,%llu,"
                 L"%llu:%02llu:%02llu\r\n",
                 registered.wYear, registered.wMonth, registered.wDay,
                 registered.wHour, registered.wMinute, registered.wSecond,
                 quoted_name, quoted_path,
                 (unsigned long long)total_seconds,
                 (unsigned long long)(total_seconds / 3600),
                 (unsigned long long)((total_seconds / 60) % 60),
                 (unsigned long long)(total_seconds % 60));
        success = write_utf8(file, row);
        free(row);
        free(quoted_name);
        free(quoted_path);
    }
    if (!CloseHandle(file)) success = FALSE;
    if (!success) DeleteFileW(path);
    return success;
}

BOOL play_time_get_foreground_executable(wchar_t *path, DWORD path_length)
{
    HWND foreground = GetForegroundWindow();
    if (foreground == NULL) return FALSE;
    DWORD process_id = 0;
    GetWindowThreadProcessId(foreground, &process_id);
    if (process_id == 0) return FALSE;
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,
                                 FALSE, process_id);
    if (process == NULL) return FALSE;
    DWORD length = path_length;
    BOOL result = QueryFullProcessImageNameW(process, 0, path, &length);
    CloseHandle(process);
    return result;
}

void play_time_tracker_init(PlayTimeTracker *tracker)
{
    ZeroMemory(tracker, sizeof(*tracker));
    tracker->active_record_index = NO_ACTIVE_RECORD;
    tracker->config.idle_timeout_seconds = DEFAULT_IDLE_TIMEOUT_SECONDS;
    tracker->state = PLAY_TIME_DISABLED;
    wchar_t path[MAX_PATH];
    if (get_settings_path(path, ARRAYSIZE(path), FALSE)) {
        tracker->config.enabled = GetPrivateProfileIntW(
            L"PlayTime", L"Enabled", FALSE, path) != 0;
        tracker->config.idle_timeout_seconds = GetPrivateProfileIntW(
            L"PlayTime", L"IdleTimeoutSeconds",
            DEFAULT_IDLE_TIMEOUT_SECONDS, path);
        if (tracker->config.idle_timeout_seconds < 5 ||
            tracker->config.idle_timeout_seconds > 3600) {
            tracker->config.idle_timeout_seconds = DEFAULT_IDLE_TIMEOUT_SECONDS;
        }
        GetPrivateProfileStringW(
            L"PlayTime", L"Executable", L"",
            tracker->config.executable_path,
            ARRAYSIZE(tracker->config.executable_path), path);
        int active_index = GetPrivateProfileIntW(
            L"PlayTime", L"ActiveRecord", -1, path);
        load_history(tracker);
        if (active_index >= 0 && (size_t)active_index < tracker->record_count) {
            tracker->active_record_index = (size_t)active_index;
        } else if (tracker->record_count > 0) {
            tracker->active_record_index = tracker->record_count - 1;
        }
        if (tracker->record_count == 0 &&
            tracker->config.executable_path[0] != L'\0') {
            FILETIME registered;
            GetSystemTimeAsFileTime(&registered);
            ULARGE_INTEGER timestamp;
            timestamp.LowPart = registered.dwLowDateTime;
            timestamp.HighPart = registered.dwHighDateTime;
            if (append_record(tracker, tracker->config.executable_path,
                              timestamp.QuadPart,
                              read_total_milliseconds(path))) {
                tracker->active_record_index = 0;
                tracker->dirty = TRUE;
            }
        }
    }
    if (tracker->active_record_index != NO_ACTIVE_RECORD) {
        PlayTimeRecord *active =
            &tracker->records[tracker->active_record_index];
        lstrcpynW(tracker->config.executable_path,
                  active->executable_path,
                  ARRAYSIZE(tracker->config.executable_path));
        tracker->total_milliseconds = active->total_milliseconds;
    }
    if (tracker->config.executable_path[0] == L'\0') {
        tracker->config.enabled = FALSE;
    }
    tracker->last_update_tick = GetTickCount64();
    tracker->last_save_tick = tracker->last_update_tick;
}

BOOL play_time_tracker_save(PlayTimeTracker *tracker)
{
    wchar_t path[MAX_PATH];
    if (!get_settings_path(path, ARRAYSIZE(path), TRUE)) return FALSE;
    wchar_t number[32];
    BOOL success = TRUE;
    if (tracker->active_record_index != NO_ACTIVE_RECORD &&
        tracker->active_record_index < tracker->record_count) {
        tracker->records[tracker->active_record_index].total_milliseconds =
            tracker->total_milliseconds;
    }
    swprintf(number, ARRAYSIZE(number), L"%d", tracker->config.enabled);
    success = WritePrivateProfileStringW(
        L"PlayTime", L"Enabled", number, path) && success;
    swprintf(number, ARRAYSIZE(number), L"%lu",
             (unsigned long)tracker->config.idle_timeout_seconds);
    success = WritePrivateProfileStringW(
        L"PlayTime", L"IdleTimeoutSeconds", number, path) && success;
    success = WritePrivateProfileStringW(
        L"PlayTime", L"Executable", tracker->config.executable_path,
        path) && success;
    swprintf(number, ARRAYSIZE(number), L"%lld",
             tracker->active_record_index == NO_ACTIVE_RECORD
                 ? -1LL : (long long)tracker->active_record_index);
    success = WritePrivateProfileStringW(
        L"PlayTime", L"ActiveRecord", number, path) && success;
    swprintf(number, ARRAYSIZE(number), L"%llu",
             (unsigned long long)tracker->total_milliseconds);
    success = WritePrivateProfileStringW(
        L"PlayTime", L"TotalMilliseconds", number, path) && success;
    success = save_history(tracker) && success;
    if (success) {
        tracker->dirty = FALSE;
        tracker->last_save_tick = GetTickCount64();
    }
    return success;
}

void play_time_tracker_update(PlayTimeTracker *tracker)
{
    ULONGLONG now = GetTickCount64();
    ULONGLONG elapsed = now - tracker->last_update_tick;
    tracker->last_update_tick = now;
    if (elapsed > 2000) elapsed = 2000;

    if (!tracker->config.enabled ||
        tracker->config.executable_path[0] == L'\0') {
        tracker->state = PLAY_TIME_DISABLED;
    } else {
        wchar_t foreground_path[MAX_PATH];
        if (!play_time_get_foreground_executable(
                foreground_path, ARRAYSIZE(foreground_path)) ||
            _wcsicmp(foreground_path,
                     tracker->config.executable_path) != 0) {
            tracker->state = PLAY_TIME_WAITING;
        } else {
            LASTINPUTINFO input = {0};
            input.cbSize = sizeof(input);
            DWORD idle_ms = 0;
            if (GetLastInputInfo(&input)) {
                idle_ms = (DWORD)GetTickCount64() - input.dwTime;
            }
            if (idle_ms >= tracker->config.idle_timeout_seconds * 1000UL) {
                tracker->state = PLAY_TIME_IDLE;
            } else {
                tracker->state = PLAY_TIME_TRACKING;
                tracker->total_milliseconds += elapsed;
                if (tracker->active_record_index != NO_ACTIVE_RECORD &&
                    tracker->active_record_index < tracker->record_count) {
                    tracker->records[tracker->active_record_index]
                        .total_milliseconds = tracker->total_milliseconds;
                }
                tracker->dirty = TRUE;
            }
        }
    }
    if (tracker->dirty && now - tracker->last_save_tick >= SAVE_INTERVAL_MS) {
        play_time_tracker_save(tracker);
    }
}

void play_time_tracker_set_config(PlayTimeTracker *tracker,
                                  const PlayTimeConfig *config)
{
    tracker->config = *config;
    tracker->last_update_tick = GetTickCount64();
    tracker->dirty = TRUE;
    play_time_tracker_save(tracker);
}

BOOL play_time_tracker_register_target(PlayTimeTracker *tracker,
                                       const wchar_t *executable_path)
{
    FILETIME registered;
    GetSystemTimeAsFileTime(&registered);
    ULARGE_INTEGER timestamp;
    timestamp.LowPart = registered.dwLowDateTime;
    timestamp.HighPart = registered.dwHighDateTime;
    if (!append_record(tracker, executable_path, timestamp.QuadPart, 0)) {
        return FALSE;
    }
    tracker->active_record_index = tracker->record_count - 1;
    tracker->total_milliseconds = 0;
    lstrcpynW(tracker->config.executable_path, executable_path,
              ARRAYSIZE(tracker->config.executable_path));
    tracker->last_update_tick = GetTickCount64();
    tracker->dirty = TRUE;
    return play_time_tracker_save(tracker);
}

void play_time_tracker_reset(PlayTimeTracker *tracker)
{
    tracker->total_milliseconds = 0;
    if (tracker->active_record_index != NO_ACTIVE_RECORD &&
        tracker->active_record_index < tracker->record_count) {
        tracker->records[tracker->active_record_index].total_milliseconds = 0;
    }
    tracker->dirty = TRUE;
    play_time_tracker_save(tracker);
}

void play_time_tracker_shutdown(PlayTimeTracker *tracker)
{
    play_time_tracker_update(tracker);
    if (tracker->dirty) play_time_tracker_save(tracker);
    free(tracker->records);
    tracker->records = NULL;
    tracker->record_count = 0;
    tracker->record_capacity = 0;
}

const PlayTimeRecord *play_time_tracker_records(
    const PlayTimeTracker *tracker, size_t *count)
{
    if (count != NULL) *count = tracker->record_count;
    return tracker->records;
}

const wchar_t *play_time_state_text(PlayTimeState state)
{
    switch (state) {
    case PLAY_TIME_WAITING: return L"ゲームの起動・操作待ち";
    case PLAY_TIME_TRACKING: return L"計測中";
    case PLAY_TIME_IDLE: return L"無操作のため一時停止中";
    case PLAY_TIME_DISABLED:
    default: return L"無効";
    }
}
