# Eroge Timer

ゲームの手前に表示する、C言語およびWin32 API製の時計オーバーレイです。

## 必要なもの

- CMake 3.16以上
- MinGW-w64 GCC（またはVisual StudioのMSVC）

## ビルド（MinGW-w64）

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

生成物は `build/eroge_timer.exe` です。

## 実行

```powershell
.\build\eroge_timer.exe
```

時計はクリック操作を背後のゲームへ透過します。通知領域のアイコンを右クリックすると、時計の表示切り替え、設定、アプリの終了ができます。設定画面では時計のサイズ（小・中・大）、表示位置（左上・右上・左下・右下）、Windowsにインストール済みのフォント、日付・曜日・秒を含む表示内容を変更できます。アイコンをダブルクリックしても表示を切り替えられます。

設定は `%APPDATA%\ErogeTimer\settings.ini` に保存され、次回起動時に復元されます。

ゲームは排他的フルスクリーンではなく、ボーダーレス全画面で実行してください。
