#include "play_time_dialog.h"

#include "overlay.h"

#include <commdlg.h>
#include <stdlib.h>
#include <wchar.h>

#define CONTROL_ENABLED 3001
#define CONTROL_EXECUTABLE 3002
#define CONTROL_CAPTURE 3003
#define CONTROL_IDLE_TIMEOUT 3004
#define CONTROL_TOTAL 3005
#define CONTROL_STATE 3006
#define CONTROL_RESET 3007
#define CONTROL_HISTORY 3008
#define CONTROL_EXPORT 3009
#define CAPTURE_TIMER_ID 1
#define REFRESH_TIMER_ID 2
#define EXPORT_COMPLETE_MESSAGE (WM_APP + 20)

typedef struct {
    HWND owner;
    wchar_t path[MAX_PATH];
    PlayTimeTracker snapshot;
} ExportTask;

typedef struct {
    PlayTimeTracker *tracker;
    PlayTimeConfig draft;
    BOOL target_registered;
    BOOL exporting;
} PlayTimeDialogContext;

static const wchar_t CLASS_NAME[] = L"ErogeTimerPlayTime";
static HWND dialog_window;

static DWORD WINAPI export_thread_proc(void *parameter)
{
    ExportTask *task = parameter;
    BOOL success = play_time_tracker_export_csv(&task->snapshot, task->path);
    if (!PostMessageW(task->owner, EXPORT_COMPLETE_MESSAGE,
                      (WPARAM)success, (LPARAM)task)) {
        free(task->snapshot.records);
        free(task);
    }
    return 0;
}

static BOOL start_export(HWND hwnd, PlayTimeDialogContext *context,
                         const wchar_t *path)
{
    ExportTask *task = calloc(1, sizeof(*task));
    if (task == NULL) return FALSE;
    task->owner = hwnd;
    lstrcpynW(task->path, path, ARRAYSIZE(task->path));
    task->snapshot.record_count = context->tracker->record_count;
    if (task->snapshot.record_count > 0) {
        size_t size = task->snapshot.record_count * sizeof(PlayTimeRecord);
        task->snapshot.records = malloc(size);
        if (task->snapshot.records == NULL) {
            free(task);
            return FALSE;
        }
        CopyMemory(task->snapshot.records, context->tracker->records, size);
    }
    HANDLE thread = CreateThread(NULL, 0, export_thread_proc, task, 0, NULL);
    if (thread == NULL) {
        free(task->snapshot.records);
        free(task);
        return FALSE;
    }
    CloseHandle(thread);
    context->exporting = TRUE;
    HWND button = GetDlgItem(hwnd, CONTROL_EXPORT);
    EnableWindow(button, FALSE);
    SetWindowTextW(button, L"書き出し中...");
    return TRUE;
}

static void set_control_font(HWND control)
{
    SendMessageW(control, WM_SETFONT,
                 (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
}

static HWND create_control(HWND parent, const wchar_t *class_name,
                           const wchar_t *text, DWORD style,
                           int x, int y, int width, int height, int id)
{
    HWND control = CreateWindowExW(
        0, class_name, text, WS_CHILD | WS_VISIBLE | style,
        x, y, width, height, parent, (HMENU)(INT_PTR)id,
        (HINSTANCE)GetWindowLongPtrW(parent, GWLP_HINSTANCE), NULL);
    if (control != NULL) set_control_font(control);
    return control;
}

static const wchar_t *file_name_from_path(const wchar_t *path)
{
    const wchar_t *slash = wcsrchr(path, L'\\');
    return slash != NULL ? slash + 1 : path;
}

static void format_duration(ULONGLONG milliseconds,
                            wchar_t *buffer, size_t buffer_length)
{
    ULONGLONG seconds = milliseconds / 1000;
    swprintf(buffer, buffer_length, L"%llu:%02llu:%02llu",
             (unsigned long long)(seconds / 3600),
             (unsigned long long)((seconds / 60) % 60),
             (unsigned long long)(seconds % 60));
}

static void update_history(HWND hwnd, const PlayTimeDialogContext *context)
{
    HWND list = GetDlgItem(hwnd, CONTROL_HISTORY);
    SendMessageW(list, LB_RESETCONTENT, 0, 0);
    size_t count = 0;
    const PlayTimeRecord *records = play_time_tracker_records(
        context->tracker, &count);
    for (size_t offset = 0; offset < count; ++offset) {
        size_t index = count - offset - 1;
        const PlayTimeRecord *record = &records[index];
        ULARGE_INTEGER timestamp;
        timestamp.QuadPart = record->registered_file_time;
        FILETIME utc = {timestamp.LowPart, timestamp.HighPart};
        FILETIME local;
        SYSTEMTIME registered = {0};
        FileTimeToLocalFileTime(&utc, &local);
        FileTimeToSystemTime(&local, &registered);
        wchar_t duration[32];
        format_duration(record->total_milliseconds,
                        duration, ARRAYSIZE(duration));
        wchar_t row[MAX_PATH + 80];
        swprintf(row, ARRAYSIZE(row),
                 L"%04u/%02u/%02u %02u:%02u  |  %-24ls  |  %ls",
                 registered.wYear, registered.wMonth, registered.wDay,
                 registered.wHour, registered.wMinute,
                 file_name_from_path(record->executable_path), duration);
        SendMessageW(list, LB_ADDSTRING, 0, (LPARAM)row);
    }
}

static void update_display(HWND hwnd, const PlayTimeDialogContext *context)
{
    wchar_t total[64];
    format_duration(context->tracker->total_milliseconds,
                    total, ARRAYSIZE(total));
    SetDlgItemTextW(hwnd, CONTROL_TOTAL, total);
    SetDlgItemTextW(hwnd, CONTROL_STATE,
                    play_time_state_text(context->tracker->state));
}

static void create_controls(HWND hwnd, PlayTimeDialogContext *context)
{
    create_control(hwnd, L"BUTTON", L"プレイ時間を自動計測する",
                   BS_AUTOCHECKBOX | WS_TABSTOP,
                   20, 18, 250, 24, CONTROL_ENABLED);
    CheckDlgButton(hwnd, CONTROL_ENABLED,
                   context->draft.enabled ? BST_CHECKED : BST_UNCHECKED);

    create_control(hwnd, L"BUTTON", L"対象ゲーム", BS_GROUPBOX,
                   16, 52, 570, 112, 0);
    create_control(hwnd, L"EDIT", context->draft.executable_path,
                   ES_AUTOHSCROLL | ES_READONLY,
                   30, 78, 542, 24, CONTROL_EXECUTABLE);
    create_control(hwnd, L"BUTTON", L"3秒後のアクティブウィンドウを登録",
                   BS_PUSHBUTTON | WS_TABSTOP,
                   181, 116, 240, 30, CONTROL_CAPTURE);

    create_control(hwnd, L"STATIC", L"無操作で停止するまで:", SS_LEFT,
                   22, 180, 150, 22, 0);
    wchar_t timeout[16];
    swprintf(timeout, ARRAYSIZE(timeout), L"%lu",
             (unsigned long)context->draft.idle_timeout_seconds);
    create_control(hwnd, L"EDIT", timeout,
                   ES_NUMBER | ES_RIGHT | WS_BORDER | WS_TABSTOP,
                   176, 176, 60, 24, CONTROL_IDLE_TIMEOUT);
    create_control(hwnd, L"STATIC", L"秒（5～3600）", SS_LEFT,
                   244, 180, 110, 22, 0);

    create_control(hwnd, L"BUTTON", L"計測状況", BS_GROUPBOX,
                   16, 216, 570, 112, 0);
    create_control(hwnd, L"STATIC", L"状態:", SS_LEFT,
                   30, 244, 54, 22, 0);
    create_control(hwnd, L"STATIC", L"", SS_LEFT,
                   88, 244, 420, 22, CONTROL_STATE);
    create_control(hwnd, L"STATIC", L"累計:", SS_LEFT,
                   30, 276, 54, 22, 0);
    create_control(hwnd, L"STATIC", L"", SS_LEFT,
                   88, 276, 190, 22, CONTROL_TOTAL);
    create_control(hwnd, L"BUTTON", L"リセット", BS_PUSHBUTTON | WS_TABSTOP,
                   484, 272, 88, 28, CONTROL_RESET);

    create_control(hwnd, L"BUTTON", L"過去の測定結果", BS_GROUPBOX,
                   16, 340, 570, 174, 0);
    create_control(hwnd, L"LISTBOX", L"",
                   LBS_NOINTEGRALHEIGHT | WS_BORDER | WS_VSCROLL |
                       WS_HSCROLL | WS_TABSTOP,
                   30, 366, 542, 132, CONTROL_HISTORY);

    create_control(hwnd, L"BUTTON", L"OK", BS_DEFPUSHBUTTON,
                   418, 528, 78, 28, IDOK);
    create_control(hwnd, L"BUTTON", L"キャンセル", BS_PUSHBUTTON,
                   506, 528, 78, 28, IDCANCEL);
    create_control(hwnd, L"BUTTON", L"CSVに書き出す...",
                   BS_PUSHBUTTON | WS_TABSTOP,
                   30, 528, 140, 28, CONTROL_EXPORT);
    update_display(hwnd, context);
    update_history(hwnd, context);
    SetTimer(hwnd, REFRESH_TIMER_ID, 1000, NULL);
}

static BOOL read_config(HWND hwnd, PlayTimeConfig *config)
{
    config->enabled = IsDlgButtonChecked(
        hwnd, CONTROL_ENABLED) == BST_CHECKED;
    wchar_t timeout[16];
    GetDlgItemTextW(hwnd, CONTROL_IDLE_TIMEOUT,
                    timeout, ARRAYSIZE(timeout));
    wchar_t *end = NULL;
    unsigned long value = wcstoul(timeout, &end, 10);
    if (end == timeout || *end != L'\0' || value < 5 || value > 3600) {
        MessageBoxW(hwnd, L"無操作時間は5～3600秒で入力してください。",
                    L"Eroge Timer", MB_OK | MB_ICONWARNING);
        return FALSE;
    }
    config->idle_timeout_seconds = (DWORD)value;
    if (config->enabled && config->executable_path[0] == L'\0') {
        MessageBoxW(hwnd, L"計測するゲームを登録してください。",
                    L"Eroge Timer", MB_OK | MB_ICONWARNING);
        return FALSE;
    }
    return TRUE;
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT message,
                                    WPARAM w_param, LPARAM l_param)
{
    if (message == WM_NCCREATE) {
        CREATESTRUCTW *create = (CREATESTRUCTW *)l_param;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                          (LONG_PTR)create->lpCreateParams);
    }
    PlayTimeDialogContext *context = (PlayTimeDialogContext *)
        GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    switch (message) {
    case WM_CREATE:
        create_controls(hwnd, context);
        return 0;
    case WM_COMMAND:
        switch (LOWORD(w_param)) {
        case CONTROL_CAPTURE:
            EnableWindow(GetDlgItem(hwnd, CONTROL_CAPTURE), FALSE);
            SetDlgItemTextW(hwnd, CONTROL_STATE,
                            L"3秒以内にゲームへ切り替えてください...");
            SetTimer(hwnd, CAPTURE_TIMER_ID, 3000, NULL);
            KillTimer(hwnd, REFRESH_TIMER_ID);
            ShowWindow(hwnd, SW_MINIMIZE);
            return 0;
        case CONTROL_RESET:
            if (MessageBoxW(hwnd, L"累計プレイ時間をリセットしますか？",
                            L"Eroge Timer",
                            MB_YESNO | MB_ICONWARNING) == IDYES) {
                play_time_tracker_reset(context->tracker);
                update_display(hwnd, context);
                update_history(hwnd, context);
            }
            return 0;
        case CONTROL_EXPORT: {
            wchar_t path[MAX_PATH] = L"play_time_history.csv";
            OPENFILENAMEW dialog = {0};
            dialog.lStructSize = sizeof(dialog);
            dialog.hwndOwner = hwnd;
            dialog.lpstrFilter = L"CSVファイル (*.csv)\0*.csv\0"
                                 L"すべてのファイル (*.*)\0*.*\0\0";
            dialog.lpstrFile = path;
            dialog.nMaxFile = ARRAYSIZE(path);
            dialog.lpstrDefExt = L"csv";
            dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST |
                           OFN_NOCHANGEDIR;
            overlay_suspend_mouse_hook();
            BOOL selected = GetSaveFileNameW(&dialog);
            overlay_resume_mouse_hook();
            if (selected) {
                if (!start_export(hwnd, context, path)) {
                    MessageBoxW(hwnd, L"測定履歴を書き出せませんでした。",
                                L"Eroge Timer", MB_OK | MB_ICONERROR);
                }
            }
            return 0;
        }
        case IDOK:
            if (context->exporting) return 0;
            if (read_config(hwnd, &context->draft)) {
                if (context->target_registered &&
                    !play_time_tracker_register_target(
                        context->tracker,
                        context->draft.executable_path)) {
                    MessageBoxW(hwnd, L"測定履歴を保存できませんでした。",
                                L"Eroge Timer", MB_OK | MB_ICONWARNING);
                    return 0;
                }
                play_time_tracker_set_config(context->tracker,
                                             &context->draft);
                DestroyWindow(hwnd);
            }
            return 0;
        case IDCANCEL:
            if (context->exporting) return 0;
            DestroyWindow(hwnd);
            return 0;
        default:
            break;
        }
        break;
    case EXPORT_COMPLETE_MESSAGE: {
        ExportTask *task = (ExportTask *)l_param;
        context->exporting = FALSE;
        HWND button = GetDlgItem(hwnd, CONTROL_EXPORT);
        EnableWindow(button, TRUE);
        SetWindowTextW(button, L"CSVに書き出す...");
        MessageBoxW(hwnd,
                    w_param ? L"測定履歴を書き出しました。"
                            : L"測定履歴を書き出せませんでした。",
                    L"Eroge Timer",
                    MB_OK | (w_param ? MB_ICONINFORMATION : MB_ICONERROR));
        free(task->snapshot.records);
        free(task);
        return 0;
    }
    case WM_TIMER:
        if (w_param == CAPTURE_TIMER_ID) {
            KillTimer(hwnd, CAPTURE_TIMER_ID);
            wchar_t path[MAX_PATH];
            wchar_t application_path[MAX_PATH];
            GetModuleFileNameW(NULL, application_path,
                               ARRAYSIZE(application_path));
            if (play_time_get_foreground_executable(path, ARRAYSIZE(path)) &&
                _wcsicmp(path, application_path) != 0) {
                lstrcpynW(context->draft.executable_path, path,
                          ARRAYSIZE(context->draft.executable_path));
                context->target_registered = TRUE;
                SetDlgItemTextW(hwnd, CONTROL_EXECUTABLE, path);
                wchar_t message_text[MAX_PATH + 32];
                swprintf(message_text, ARRAYSIZE(message_text),
                         L"登録: %ls", file_name_from_path(path));
                SetDlgItemTextW(hwnd, CONTROL_STATE, message_text);
            } else {
                SetDlgItemTextW(hwnd, CONTROL_STATE,
                                L"ゲームのウィンドウを取得できませんでした");
            }
            ShowWindow(hwnd, SW_RESTORE);
            SetForegroundWindow(hwnd);
            EnableWindow(GetDlgItem(hwnd, CONTROL_CAPTURE), TRUE);
            SetTimer(hwnd, REFRESH_TIMER_ID, 1000, NULL);
            return 0;
        }
        if (w_param == REFRESH_TIMER_ID) {
            update_display(hwnd, context);
            update_history(hwnd, context);
            return 0;
        }
        break;
    case WM_CLOSE:
        if (context->exporting) {
            MessageBoxW(hwnd, L"CSVの書き出しが完了するまでお待ちください。",
                        L"Eroge Timer", MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd, CAPTURE_TIMER_ID);
        KillTimer(hwnd, REFRESH_TIMER_ID);
        free(context);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        dialog_window = NULL;
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, message, w_param, l_param);
}

BOOL play_time_dialog_register_class(HINSTANCE instance)
{
    WNDCLASSEXW window_class = {0};
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = window_proc;
    window_class.hInstance = instance;
    window_class.hCursor = LoadCursorW(NULL, IDC_ARROW);
    window_class.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    window_class.lpszClassName = CLASS_NAME;
    return RegisterClassExW(&window_class) != 0;
}

void play_time_dialog_show(HWND owner, PlayTimeTracker *tracker)
{
    if (dialog_window != NULL) {
        ShowWindow(dialog_window, SW_RESTORE);
        SetForegroundWindow(dialog_window);
        return;
    }
    PlayTimeDialogContext *context = calloc(1, sizeof(*context));
    if (context == NULL) return;
    context->tracker = tracker;
    context->draft = tracker->config;
    HINSTANCE instance = (HINSTANCE)GetWindowLongPtrW(owner, GWLP_HINSTANCE);
    dialog_window = CreateWindowExW(
        WS_EX_DLGMODALFRAME, CLASS_NAME, L"Eroge Timer - プレイ時間計測",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 618, 610,
        owner, NULL, instance, context);
    if (dialog_window == NULL) {
        free(context);
        return;
    }
    ShowWindow(dialog_window, SW_SHOW);
    SetForegroundWindow(dialog_window);
}
