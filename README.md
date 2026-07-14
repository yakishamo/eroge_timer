# Eroge Timer

![Eroge Timer icon](assets/app-icon.png)

ゲームの手前に表示する、C言語およびWin32 API製の時計オーバーレイです。

## ダウンロード

GitHub Releasesから最新の`eroge_timer-v1.0.0-windows-x64.zip`をダウンロードし、任意のフォルダーへ展開してください。インストールは不要です。

## 対応環境

- Windows 10/11（64bit）
- ボーダーレス全画面またはウィンドウ表示のゲーム

排他的フルスクリーンでは、時計がゲームより前面に表示されない場合があります。

## 主な機能

- クリックをゲームへ透過する常時最前面の時計
- サイズ、位置、フォント、表示形式の変更
- 日付、曜日、秒の表示
- 輪郭線と影
- 代表的なゲーム解像度に合わせた描画
- 設定の自動保存
- 時計または通知領域の右クリックメニュー

## 使い方

1. `eroge_timer.exe`を起動します。
2. 時計または通知領域のアイコンを右クリックして設定します。
3. 終了する場合は、右クリックメニューの「終了」を選択します。

設定は `%APPDATA%\ErogeTimer\settings.ini` に保存され、次回起動時に復元されます。

## 制限事項

- 排他的フルスクリーンでの表示は保証されません。
- アンチチートを使用するゲームでの動作は保証されません。
- コード署名を行っていないため、Windowsから警告が表示される場合があります。

## 開発環境

- CMake 3.16以上
- MinGW-w64 GCC（またはVisual StudioのMSVC）

## ビルド（MinGW-w64）

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

生成物は `build/eroge_timer.exe` です。

## 開発版の実行

```powershell
.\build\eroge_timer.exe
```

## リリースパッケージの作成

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build --clean-first
Set-Location build
cpack -G ZIP
```

`build/eroge_timer-v1.0.0-windows-x64.zip`が生成されます。

## ライセンス

このソフトウェアは[MIT License](LICENSE)で公開されています。

Copyright (c) 2026 yakishamo
