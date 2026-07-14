#include "settings_dialog.h"

#include "overlay.h"
#include "settings_store.h"

#define CONTROL_SIZE_SMALL 2001
#define CONTROL_SIZE_MEDIUM 2002
#define CONTROL_SIZE_LARGE 2003
#define CONTROL_POSITION_TOP_LEFT 2011
#define CONTROL_POSITION_TOP_RIGHT 2012
#define CONTROL_POSITION_BOTTOM_LEFT 2013
#define CONTROL_POSITION_BOTTOM_RIGHT 2014
#define CONTROL_FONT 2021
#define CONTROL_DISPLAY 2022
#define CONTROL_OUTLINE 2023
#define CONTROL_SHADOW 2024
#define CONTROL_RENDER_RESOLUTION 2025

static const wchar_t SETTINGS_CLASS_NAME[] = L"ErogeTimerSettings";
static HWND settings_window;

static void set_control_font(HWND control)
{
    SendMessageW(control, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
}

static HWND create_control(HWND parent, const wchar_t *class_name, const wchar_t *text,
                           DWORD style, int x, int y, int width, int height, int id)
{
    HWND control = CreateWindowExW(
        0, class_name, text, WS_CHILD | WS_VISIBLE | style,
        x, y, width, height, parent, (HMENU)(INT_PTR)id,
        (HINSTANCE)GetWindowLongPtrW(parent, GWLP_HINSTANCE), NULL);
    if (control != NULL) {
        set_control_font(control);
    }
    return control;
}

static int CALLBACK enumerate_font(const LOGFONTW *log_font,
                                   const TEXTMETRICW *text_metric,
                                   DWORD font_type, LPARAM parameter)
{
    (void)text_metric;
    (void)font_type;

    HWND combo = (HWND)parameter;
    const wchar_t *face_name = log_font->lfFaceName;
    if (face_name[0] != L'@' &&
        SendMessageW(combo, CB_FINDSTRINGEXACT, (WPARAM)-1,
                     (LPARAM)face_name) == CB_ERR) {
        SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)face_name);
    }
    return 1;
}

static void fill_font_list(HWND combo, const AppSettings *settings)
{
    HDC dc = GetDC(NULL);
    if (dc != NULL) {
        LOGFONTW query = {0};
        query.lfCharSet = DEFAULT_CHARSET;
        EnumFontFamiliesExW(dc, &query, enumerate_font, (LPARAM)combo, 0);
        ReleaseDC(NULL, dc);
    }

    LRESULT selected = SendMessageW(combo, CB_FINDSTRINGEXACT, (WPARAM)-1,
                                    (LPARAM)settings->font);
    if (selected == CB_ERR) {
        selected = SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)settings->font);
    }
    SendMessageW(combo, CB_SETCURSEL, (WPARAM)selected, 0);
}

static void fill_display_list(HWND combo, const AppSettings *settings)
{
    static const wchar_t *items[] = {
        L"時:分（例 12:34）",
        L"時:分:秒（例 12:34:56）",
        L"月/日＋時刻（例 07/15 12:34）",
        L"年/月/日＋時刻（例 2026/07/15 12:34）",
        L"年/月/日＋曜日＋時刻（例 2026/07/15 (水) 12:34）"
    };
    for (int i = 0; i < CLOCK_DISPLAY_COUNT; ++i) {
        SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)items[i]);
    }
    SendMessageW(combo, CB_SETCURSEL, settings->display, 0);
}

static void fill_resolution_list(HWND combo, const AppSettings *settings)
{
    static const wchar_t *items[] = {
        L"ネイティブ（低解像度化なし）",
        L"640 x 480",
        L"800 x 600",
        L"1024 x 768",
        L"1280 x 720",
        L"1280 x 800",
        L"1366 x 768",
        L"1600 x 900",
        L"1920 x 1080"
    };
    for (int i = 0; i < RENDER_RESOLUTION_COUNT; ++i) {
        SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)items[i]);
    }
    SendMessageW(combo, CB_SETCURSEL, settings->render_resolution, 0);
}

static void create_settings_controls(HWND hwnd, const AppSettings *settings)
{
    create_control(hwnd, L"BUTTON", L"サイズ", BS_GROUPBOX, 16, 12, 282, 82, 0);
    create_control(hwnd, L"BUTTON", L"小", BS_AUTORADIOBUTTON | WS_GROUP,
                   32, 42, 60, 24, CONTROL_SIZE_SMALL);
    create_control(hwnd, L"BUTTON", L"中", BS_AUTORADIOBUTTON,
                   118, 42, 60, 24, CONTROL_SIZE_MEDIUM);
    create_control(hwnd, L"BUTTON", L"大", BS_AUTORADIOBUTTON,
                   204, 42, 60, 24, CONTROL_SIZE_LARGE);

    create_control(hwnd, L"BUTTON", L"表示位置", BS_GROUPBOX, 16, 106, 282, 112, 0);
    create_control(hwnd, L"BUTTON", L"左上", BS_AUTORADIOBUTTON | WS_GROUP,
                   32, 134, 100, 24, CONTROL_POSITION_TOP_LEFT);
    create_control(hwnd, L"BUTTON", L"右上", BS_AUTORADIOBUTTON,
                   166, 134, 100, 24, CONTROL_POSITION_TOP_RIGHT);
    create_control(hwnd, L"BUTTON", L"左下", BS_AUTORADIOBUTTON,
                   32, 174, 100, 24, CONTROL_POSITION_BOTTOM_LEFT);
    create_control(hwnd, L"BUTTON", L"右下", BS_AUTORADIOBUTTON,
                   166, 174, 100, 24, CONTROL_POSITION_BOTTOM_RIGHT);

    create_control(hwnd, L"BUTTON", L"フォント", BS_GROUPBOX, 16, 230, 282, 70, 0);
    HWND font_combo = create_control(
        hwnd, L"COMBOBOX", L"",
        CBS_DROPDOWNLIST | CBS_SORT | WS_VSCROLL | WS_TABSTOP,
        32, 254, 250, 200, CONTROL_FONT);
    if (font_combo != NULL) {
        fill_font_list(font_combo, settings);
    }

    create_control(hwnd, L"BUTTON", L"表示内容", BS_GROUPBOX, 16, 312, 282, 70, 0);
    HWND display_combo = create_control(
        hwnd, L"COMBOBOX", L"",
        CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
        32, 336, 250, 180, CONTROL_DISPLAY);
    if (display_combo != NULL) {
        fill_display_list(display_combo, settings);
    }

    create_control(hwnd, L"BUTTON", L"文字効果", BS_GROUPBOX,
                   16, 394, 282, 62, 0);
    create_control(hwnd, L"BUTTON", L"輪郭線", BS_AUTOCHECKBOX | WS_TABSTOP,
                   32, 416, 100, 24, CONTROL_OUTLINE);
    create_control(hwnd, L"BUTTON", L"影", BS_AUTOCHECKBOX | WS_TABSTOP,
                   166, 416, 100, 24, CONTROL_SHADOW);
    CheckDlgButton(hwnd, CONTROL_OUTLINE,
                   settings->outline ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, CONTROL_SHADOW,
                   settings->shadow ? BST_CHECKED : BST_UNCHECKED);

    create_control(hwnd, L"BUTTON", L"ゲーム解像度", BS_GROUPBOX,
                   16, 468, 282, 70, 0);
    HWND resolution_combo = create_control(
        hwnd, L"COMBOBOX", L"",
        CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
        32, 492, 250, 200, CONTROL_RENDER_RESOLUTION);
    if (resolution_combo != NULL) {
        fill_resolution_list(resolution_combo, settings);
    }

    create_control(hwnd, L"BUTTON", L"OK", BS_DEFPUSHBUTTON,
                   132, 554, 78, 28, IDOK);
    create_control(hwnd, L"BUTTON", L"キャンセル", BS_PUSHBUTTON,
                   220, 554, 78, 28, IDCANCEL);

    CheckRadioButton(hwnd, CONTROL_SIZE_SMALL, CONTROL_SIZE_LARGE,
                     CONTROL_SIZE_SMALL + settings->size);
    CheckRadioButton(hwnd, CONTROL_POSITION_TOP_LEFT, CONTROL_POSITION_BOTTOM_RIGHT,
                     CONTROL_POSITION_TOP_LEFT + settings->position);
}

static void read_settings(HWND hwnd, AppSettings *settings)
{
    if (IsDlgButtonChecked(hwnd, CONTROL_SIZE_SMALL) == BST_CHECKED) {
        settings->size = CLOCK_SIZE_SMALL;
    } else if (IsDlgButtonChecked(hwnd, CONTROL_SIZE_LARGE) == BST_CHECKED) {
        settings->size = CLOCK_SIZE_LARGE;
    } else {
        settings->size = CLOCK_SIZE_MEDIUM;
    }

    if (IsDlgButtonChecked(hwnd, CONTROL_POSITION_TOP_RIGHT) == BST_CHECKED) {
        settings->position = CLOCK_POSITION_TOP_RIGHT;
    } else if (IsDlgButtonChecked(hwnd, CONTROL_POSITION_BOTTOM_LEFT) == BST_CHECKED) {
        settings->position = CLOCK_POSITION_BOTTOM_LEFT;
    } else if (IsDlgButtonChecked(hwnd, CONTROL_POSITION_BOTTOM_RIGHT) == BST_CHECKED) {
        settings->position = CLOCK_POSITION_BOTTOM_RIGHT;
    } else {
        settings->position = CLOCK_POSITION_TOP_LEFT;
    }

    HWND font_combo = GetDlgItem(hwnd, CONTROL_FONT);
    LRESULT selected_font = SendMessageW(font_combo, CB_GETCURSEL, 0, 0);
    if (selected_font != CB_ERR) {
        SendMessageW(font_combo, CB_GETLBTEXT, (WPARAM)selected_font,
                     (LPARAM)settings->font);
        settings->font[LF_FACESIZE - 1] = L'\0';
    }

    HWND display_combo = GetDlgItem(hwnd, CONTROL_DISPLAY);
    LRESULT selected_display = SendMessageW(display_combo, CB_GETCURSEL, 0, 0);
    if (selected_display >= 0 && selected_display < CLOCK_DISPLAY_COUNT) {
        settings->display = (ClockDisplay)selected_display;
    }
    settings->outline = IsDlgButtonChecked(hwnd, CONTROL_OUTLINE) == BST_CHECKED;
    settings->shadow = IsDlgButtonChecked(hwnd, CONTROL_SHADOW) == BST_CHECKED;
    HWND resolution_combo = GetDlgItem(hwnd, CONTROL_RENDER_RESOLUTION);
    LRESULT selected_resolution = SendMessageW(
        resolution_combo, CB_GETCURSEL, 0, 0);
    if (selected_resolution >= 0 &&
        selected_resolution < RENDER_RESOLUTION_COUNT) {
        settings->render_resolution = (RenderResolution)selected_resolution;
    }
}

static LRESULT CALLBACK settings_window_proc(HWND hwnd, UINT message,
                                             WPARAM w_param, LPARAM l_param)
{
    if (message == WM_NCCREATE) {
        CREATESTRUCTW *create = (CREATESTRUCTW *)l_param;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)create->lpCreateParams);
    }

    AppSettings *settings = (AppSettings *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    switch (message) {
    case WM_CREATE:
        create_settings_controls(hwnd, settings);
        return 0;
    case WM_COMMAND:
        if (LOWORD(w_param) == IDOK) {
            HWND owner = GetWindow(hwnd, GW_OWNER);
            read_settings(hwnd, settings);
            overlay_apply_settings(owner, settings);
            if (!settings_store_save(settings)) {
                MessageBoxW(hwnd, L"設定を保存できませんでした。",
                            L"Eroge Timer", MB_OK | MB_ICONWARNING);
            }
            DestroyWindow(hwnd);
            return 0;
        }
        if (LOWORD(w_param) == IDCANCEL) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        settings_window = NULL;
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, message, w_param, l_param);
}

BOOL settings_dialog_register_class(HINSTANCE instance)
{
    WNDCLASSEXW window_class = {0};
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = settings_window_proc;
    window_class.hInstance = instance;
    window_class.hCursor = LoadCursorW(NULL, IDC_ARROW);
    window_class.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    window_class.lpszClassName = SETTINGS_CLASS_NAME;
    return RegisterClassExW(&window_class) != 0;
}

void settings_dialog_show(HWND owner, AppSettings *settings)
{
    if (settings_window != NULL) {
        ShowWindow(settings_window, SW_RESTORE);
        SetForegroundWindow(settings_window);
        return;
    }

    HINSTANCE instance = (HINSTANCE)GetWindowLongPtrW(owner, GWLP_HINSTANCE);
    settings_window = CreateWindowExW(
        WS_EX_DLGMODALFRAME, SETTINGS_CLASS_NAME, L"Eroge Timer 設定",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 330, 640,
        owner, NULL, instance, settings);

    if (settings_window != NULL) {
        ShowWindow(settings_window, SW_SHOW);
        SetForegroundWindow(settings_window);
    }
}
