# Noty

Nativer Quellcode-Editor in C++20 für Windows, macOS und Linux. Erste Vorschau, Version 0.2.0.

## Architektur und Oberfläche

wxWidgets verwendet Win32, Cocoa bzw. GTK. Scintilla (wxStyledTextCtrl) liefert Textpuffer, Undo/Redo, Rendering, Zeilennummern und Lexer. So bleibt der Editor ohne Qt, Electron, .NET oder Weboberfläche plattformübergreifend. Eine eigene Textengine wäre deutlich aufwendiger und würde zunächst weniger ausgereifte Eingabe- und Accessibility-Unterstützung bieten.

Windows erhält DWM Dark Titlebar, Rounded Corners und einen Mica-Backdrop, soweit das Betriebssystem sie unterstützt. Die Standard-Titelleiste bewahrt Snap Layouts und native Fensterbedienung. Die Vorschau hat noch keine maßgeschneiderte Fluent Command Bar oder animierten Flyouts. Ein Mica-Backdrop kann hinter deckenden nativen Controls nicht sichtbar sein.

## Dateien

- CMakeLists.txt: Abhängigkeiten, C++20, Plattform-Linking und CPack
- src/main.cpp: native Oberfläche, Dokumente, Scintilla, Suche und Themes
- src/updater.h / src/updater.cpp: asynchrone Release-Abfrage, SHA-256-Prüfung und Installer-Download
- packaging/noty.desktop: Linux Desktop-Eintrag
- .github/workflows/build.yml: drei native Builds, Installer und Preview-Releases
- RELEASE_NOTES.md: Lieferumfang und Einschränkungen

## Build

Voraussetzungen: CMake >= 3.24, C++20-Compiler, Git und Internet für die erstmalige Abhängigkeitsauflösung.
Windows: Visual Studio 2022 mit Desktop C++, Windows SDK, NSIS.
macOS: Xcode Command Line Tools.
Ubuntu 24.04: build-essential cmake git libgtk-3-dev libcurl4-openssl-dev libsecret-1-dev.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 3
cd build
cpack -C Release
```

GitHub Actions baut Windows x64, macOS ARM64 und Ubuntu Linux x64. Nur wenn alle Builds erfolgreich sind, veröffentlicht ein main-Push eine neue Preview-Version. Bereits veröffentlichte Versionen werden nicht überschrieben. Für die nächste Version PROJECT_VERSION in CMakeLists.txt erhöhen.

Windows erhält eine Setup.exe und eine ZIP, macOS eine DMG und Linux eine DEB plus TGZ. Die Linux-Pakete sind auf Ubuntu 24.04 ausgerichtet; keine Zusage für jede Distribution. macOS Intel ist noch nicht in der Build-Matrix.

## Updates

Hilfe → Nach Updates suchen fragt GitHub asynchron über HTTPS ab und berücksichtigt Vorschau-Releases. Höhere Versionen können nach Zustimmung direkt heruntergeladen werden. Der Updater wählt exakt das zur Plattform passende Installer-Asset aus diesem Repository und prüft dessen SHA-256-Digest gegen GitHubs HTTPS-Metadaten. Erst danach wird nach erneuter Zustimmung der Installer bzw. das Systemprogramm für DMG/DEB geöffnet. Keine stille Installation. Das laufende Noty muss vor dem Ersetzen geschlossen werden. SHA-256 ersetzt keine Codesignatur; Installer bleiben vorerst unsigniert.

## Manuelle Prüfung

1. Zwei UTF-8-Dateien öffnen, ändern, speichern, schließen und erneut öffnen.
2. Schließen mit Speichern / Verwerfen / Abbrechen prüfen; bei nicht beschreibbarem Ziel muss das Dokument geöffnet bleiben.
3. Undo/Redo und Ersetzen mit mehrsprachigem Text prüfen.
4. Syntax und Folding in C++, Python, JSON, HTML, CSS, Markdown und Batch prüfen.
5. Light/Dark, Schriftwahl, LF/CRLF und 100–200 % DPI prüfen.
6. Ohne Netzwerk Update-Prüfung ausführen: Oberfläche bleibt bedienbar, Fehlermeldung erscheint.
7. Installer auf frischem Rechner installieren und deinstallieren; macOS-DMG und Linux-DEB prüfen.

Die CI bestätigt Kompilierung und Paketbau, ersetzt aber keine visuellen oder interaktiven Plattformtests.

## Grenzen und Fahrplan

Die umfangreiche Ausgangsspezifikation ist noch nicht vollständig implementiert. Offen sind UTF-16/ANSI und BOM-Erhaltung, asynchrones Datei-I/O, große Dateien über 64 MiB, Regex/Whole Word/Suche über alle Tabs, Explorer, Session/Crash-Recovery, Makros, Plugin-ABI, eigene Fluent-Titelleiste, benutzerdefinierte Themes, explizite UI-Automation-Prüfung, Split View und Multi-Cursor.

V0.2: Encoding-Layer, Hintergrund-I/O, Sessions und Regex-Suche.
V0.3: Explorer, Makros, Plugin-ABI und Fluent-Oberfläche.
V1.0: signierte Windows-Installer, macOS-Notarisierung, automatischer Neustart nach Update, Intel-Mac und weitere Linux-Paketformate. Signierzertifikate und Apple-Zugangsdaten müssen dafür als GitHub Secrets bereitgestellt werden.
