#include "tray.h"

#include <shellapi.h>

#define TRAY_ICON_ID 1

static NOTIFYICONDATAW tray_icon;

BOOL tray_add(HWND owner)
{
    ZeroMemory(&tray_icon, sizeof(tray_icon));
    tray_icon.cbSize = sizeof(tray_icon);
    tray_icon.hWnd = owner;
    tray_icon.uID = TRAY_ICON_ID;
    tray_icon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    tray_icon.uCallbackMessage = TRAY_CALLBACK_MESSAGE;
    tray_icon.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    lstrcpynW(tray_icon.szTip, L"Eroge Timer", ARRAYSIZE(tray_icon.szTip));
    return Shell_NotifyIconW(NIM_ADD, &tray_icon);
}

void tray_remove(void)
{
    Shell_NotifyIconW(NIM_DELETE, &tray_icon);
}

void tray_show_menu(HWND owner, BOOL clock_visible)
{
    HMENU menu = CreatePopupMenu();
    if (menu == NULL) {
        return;
    }

    AppendMenuW(menu, MF_STRING | (clock_visible ? MF_CHECKED : MF_UNCHECKED),
                TRAY_COMMAND_TOGGLE_VISIBILITY, L"時計を表示");
    AppendMenuW(menu, MF_STRING, TRAY_COMMAND_SETTINGS, L"設定...");
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu, MF_STRING, TRAY_COMMAND_EXIT, L"終了");

    POINT cursor;
    GetCursorPos(&cursor);
    SetForegroundWindow(owner);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, cursor.x, cursor.y, 0, owner, NULL);
    PostMessageW(owner, WM_NULL, 0, 0);
    DestroyMenu(menu);
}
