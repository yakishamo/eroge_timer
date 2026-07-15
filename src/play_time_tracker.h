#ifndef EROGE_TIMER_PLAY_TIME_TRACKER_H
#define EROGE_TIMER_PLAY_TIME_TRACKER_H

#include <windows.h>

typedef enum {
    PLAY_TIME_DISABLED,
    PLAY_TIME_WAITING,
    PLAY_TIME_TRACKING,
    PLAY_TIME_IDLE
} PlayTimeState;

typedef struct {
    BOOL enabled;
    wchar_t executable_path[MAX_PATH];
    DWORD idle_timeout_seconds;
} PlayTimeConfig;

typedef struct {
    wchar_t executable_path[MAX_PATH];
    ULONGLONG registered_file_time;
    ULONGLONG total_milliseconds;
} PlayTimeRecord;

typedef struct {
    PlayTimeConfig config;
    ULONGLONG total_milliseconds;
    PlayTimeRecord *records;
    size_t record_count;
    size_t record_capacity;
    size_t active_record_index;
    ULONGLONG last_update_tick;
    ULONGLONG last_save_tick;
    PlayTimeState state;
    BOOL dirty;
} PlayTimeTracker;

void play_time_tracker_init(PlayTimeTracker *tracker);
void play_time_tracker_update(PlayTimeTracker *tracker);
void play_time_tracker_shutdown(PlayTimeTracker *tracker);
void play_time_tracker_set_config(PlayTimeTracker *tracker,
                                  const PlayTimeConfig *config);
BOOL play_time_tracker_register_target(PlayTimeTracker *tracker,
                                       const wchar_t *executable_path);
void play_time_tracker_reset(PlayTimeTracker *tracker);
BOOL play_time_tracker_save(PlayTimeTracker *tracker);
BOOL play_time_tracker_export_csv(const PlayTimeTracker *tracker,
                                  const wchar_t *path);
BOOL play_time_get_foreground_executable(wchar_t *path, DWORD path_length);
const wchar_t *play_time_state_text(PlayTimeState state);
const PlayTimeRecord *play_time_tracker_records(
    const PlayTimeTracker *tracker, size_t *count);

#endif
