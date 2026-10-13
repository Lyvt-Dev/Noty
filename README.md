# Noty 0.6

Eigenständiger nativer Notizeneditor: linke Notizen-Sidebar, SVG-Icon-Leiste, 8-px-Eckenradius, helle/dunkle Editorflächen. Keine oberen Dokument-Tabs. Vollbild mit F11, zurück mit Escape.

Updates werden standardmäßig beim Start und alle sechs Stunden automatisch von GitHub geladen und anhand SHA-256 geprüft. Sobald das Update bereit ist, erscheint der Neustart-Button. Der native Helfer wartet auf das Ende der App und aktualisiert sie; ein abgebrochenes Schließen wegen ungespeicherter Notizen verhindert die Installation. Bei normalem Beenden wird ein vorbereitetes Update installiert, ohne Neustart. Automatische Vorbereitung lässt sich im Einstellungsmenü ausschalten.

Windows verwendet ein stilles Setup im bestehenden Verzeichnis. macOS ersetzt ein beschreibbares Noty.app-Bundle. Linux benötigt eine DEB-Systeminstallation unter /usr/bin/Noty und gegebenenfalls die Systemfreigabe über pkexec. Portable ZIP/TGZ-Installationen sind für automatische Updates noch nicht unterstützt. Version 0.5 benötigt einmal das bisherige Setup; ab 0.6 gilt der neue Ablauf. macOS/Linux-Helfer werden gebaut, ihre reale Update-Installation ist noch nicht auf diesen Betriebssystemen nachgewiesen.

## Vorhandene Editor-Funktionen und Grenzen

### Noty

Native C++20 text and source editor for Windows, macOS and Linux. Version 0.6.0 is a preview, not a claim of complete Notepad++ parity.

## Oberfläche

Die Standardansicht verwendet eine Notizen-Sidebar und eine kompakte Icon-Werkzeugleiste. Karten und Fenster erhalten 8-px-Ecken. Die Fenstercontrols sind anklickbare native Controls; F11 schaltet Vollbild um. Editor-Befehle sind über das Befehlsmenü, Einstellungen und Tastenkürzel erreichbar. Die Windows-Titelfläche bleibt verschiebbar; Resize funktioniert am Fensterrand. Snap-Layouts per Hover über den eigenen Maximieren-Button sind noch nicht implementiert.

Alle deutschen Oberflächentexte verwenden ausdrückliche UTF-8-Konvertierung. Windows-ANSI-Codepages bestimmen ihre Darstellung nicht mehr. Die Regex-Suche arbeitet mit Unicode über wxRegEx/PCRE2; ein Punkt kann auch ein einzelnes mehrbyteiges Zeichen wie `ß` oder ein Emoji treffen.

## Funktionsumfang

| Bereich | Vorhanden | Grenze |
|---|---|---|
| Dokumente | Mehrere Notizen in der Sidebar, Schließen, andere/alle schließen, geschlossenen Tab wieder öffnen | Keine abgetrennten Fenster/Tab-Pinning |
| Dateien | Hintergrund-Laden, Speichern, Speichern unter, alle speichern, Fehlerschutz | Maximal 128 MiB; Speichern läuft noch im UI-Thread |
| Encodings | UTF-8, UTF-8 BOM, UTF-16 LE/BE mit BOM, Windows-1252, ISO-8859-1 | Kein vollständiger Codepage-Katalog; nicht darstellbare Zeichen lösen einen Fehler aus |
| Zeilenenden | LF, CRLF und CR erkennen/konvertieren | Reopen-/Session-EOL-Einstellungen noch nicht vollständig unabhängig konfigurierbar |
| Bearbeiten | Undo/Redo, Zwischenablage, Zeile duplizieren/löschen, Groß-/Kleinschreibung, Sortieren, Trim, Tabs/Spaces | Kein vollständiger Satz aller Notepad++-Zeilenoperationen |
| Suchen | Literal/Regex, Case, Whole Word, vorwärts/rückwärts, alle Notizen, Ersetzen, alle ersetzen, Trefferliste | Keine identische Notepad++-Suchdialog-/Regex-Ersetzungssemantik für jede Spezialsyntax |
| Ordnersuche | Hintergrund-Suche mit Trefferliste und Dateisprung | Zeilenbasiert, 16 MiB je Datei, höchstens 10.000 Treffer; `.git`, `build`, `node_modules` ausgeschlossen |
| Syntax | Automatische Scintilla-Lexer für C/C++, Python, JSON, HTML/XML/PHP, CSS, JS/TS, Markdown, Batch, Rust, SQL, YAML, Shell, Lua, PowerShell und weitere; manuelle Sprachwahl | Nicht alle Sprachstile/Keywords gleich detailliert; keine UDL-Import-Kompatibilität |
| Navigation | Gehe zu Zeile, Lesezeichen, Klammermarkierung, Folding, Einrückung | Keine Function List oder Document Map |
| Ansichten | Optionale Zeilennummern, Whitespace, Guides, Wrap, Zoom, Schriftwahl, Light/Dark, Projektordner, zwei Ansichten eines gemeinsamen Puffers | Einige Menüaktionen beziehen sich auf die Hauptansicht |
| Mehrfachauswahl | Scintilla Multiple Selection, zusätzliche Eingabe, Rechteckauswahl | Kein Notepad++ Column Editor mit Nummernreihen |
| Makros | Text-Insert/Delete aufnehmen, gruppiert wiedergeben, JSON laden/speichern | Keine vollständige Aufzeichnung von Navigation, Menü- und Suchaktionen |
| Sitzung | Offene Notizen, Cursor, View-Einstellungen; regelmäßige JSON-Backups geänderter Puffer | Backup nur bis 8 MiB je Puffer; keine vollständige Disaster-Recovery-Garantie |
| Plugins | Explizites Laden nativer Noty-ABI-v1-Plugins; Beispiel-Plugin | Notepad++-DLLs verwenden eine andere ABI und sind nicht kompatibel |
| Updates | Automatische Release-Prüfung und Downloads, HTTPS, SHA-256, nativer Installations-/Neustart-Helfer | Plattformrechte können nötig sein; Windows-Upgrades im CI geprüft, reale macOS/Linux-Upgrades stehen aus |
| Accessibility/DPI | Native Editor-Eingabe, Tastenkürzel, PerMonitorV2-Manifest | Eigene Sidebar/Icon-Controls haben noch keine vollständige UIA-Struktur; breite Screenreader-/Monitorprüfungen stehen aus |

Weitere offene Notepad++-Bereiche: Shortcut Mapper, vollständiger Style Configurator, User Defined Languages, Plugin Admin/Katalog, Run-Kommandos, Drucken/Print Preview, File Monitoring/tail, vollständiger Spalteneditor, Function List, Document Map, vollständige Sprach-/Codepage-Abdeckung, Sucheinstellungen und alle seltenen Menü-/Tastenkombinationen. Referenz für die weitere Paritätsarbeit: [offizielles Notepad++-Handbuch](https://npp-user-manual.org/docs/user-interface/).

## Updates von Version 0.4

Hilfe bzw. Einstellungsmenü → Nach Updates suchen. Der bisherige Updater konvertierte JSON-Daten über die lokale ANSI-Codepage. Die Release-Pipeline hält die sichtbaren Release-Texte für diesen alten Client ASCII-kompatibel und korrigiert die Metadaten von v0.4.0. Installer, Tags und deren Prüfsummen werden dabei nicht verändert.

Ein Installer-Update aktualisiert die vorhandene Anwendung; eine vorherige Deinstallation ist nicht nötig. Version 0.6 installiert vorbereitete Updates nach dem Schließen und prüft dabei ungespeicherte Dokumente. Der alte v0.4-Client muss wie in seinem Dialog beschrieben manuell geschlossen werden. Portable ZIP-Versionen haben noch keinen verifizierten Austausch der laufenden ZIP-Installation.

Dateien werden nur aus `Lyvt-Dev/Noty` geladen; ein exakter Assetname, eine feste GitHub-URL und ein SHA-256-Digest werden geprüft. SHA-256 ersetzt keine Codesignatur. Windows-Installer sind noch unsigniert, macOS-Pakete nicht notarisiert. Apple-Zugangsdaten und Signierzertifikate sind nicht enthalten.

## Architektur und Build

wxWidgets: Win32/Cocoa/GTK. Scintilla: Puffer, Eingabe, Undo/Redo, Lexer und Rendering. Eigene Module: Chrome, Dokumente, Codec, Unicode-Suche, Sitzungen, Plugin-ABI und Updater. Keine Electron-, Qt- oder .NET-Oberfläche.

Voraussetzungen: CMake >=3.24, Git und C++20-Compiler. Windows: Visual Studio C++ + Windows SDK, NSIS für Setup-Pakete. macOS: Xcode Command Line Tools. Ubuntu 24.04: `libgtk-3-dev libcurl4-openssl-dev libsecret-1-dev`.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target Noty update_test codec_test ui_text_test search_test noty_example_plugin --parallel 3
ctest --test-dir build -C Release --output-on-failure
cd build
cpack -C Release
```

GitHub Actions baut Windows x64, macOS Apple Silicon und Ubuntu 24.04 x64. Nur ein erfolgreicher Lauf aller Plattformen veröffentlicht die neue Version. CMake-Projektversion erhöhen, um einen neuen Release anzulegen. Bereits veröffentlichte Versionen werden nicht überschrieben.

```sh
Noty --portable
Noty --no-session file.txt
Noty --smoke-test
```

`--portable` speichert die Sitzung beim Programm, sonst im Benutzerprofil. Der Testmodus verwendet ein separates Profil neben dem Testprogramm und überschreibt die normale Sitzung nicht.

## Tests

Automatisch: SemVer/SHA-256-Grundprüfung, UTF-8/BOM/UTF-16/1252-Roundtrips einschließlich Umlauten und Emojis, fehlerhafte Encodings, Unicode-Menütexte und Regex-Dot/Lookbehind/Rückverweise/Null-Längen-Treffer. Der Anwendungstest prüft echte Editor-Operationen, Undo, Lesezeichen, gemeinsamen Split-Puffer, Makro-Aufnahme/-Wiedergabe und UTF-16-Speichern mit deutschem Dateinamen. Windows und Linux führen ihn in CI aus.

Zusätzlich manuell prüfen: Snap Layouts auf Windows 11, mehrere DPI-Monitore, Tab-Dragging mit vielen Dokumenten, Screenreader, Installer-Upgrade von v0.4, Schrift-/Theme-Wechsel und Plattformdialoge. Ein erfolgreicher Build ist kein Nachweis für fehlerfreie Bedienung auf jedem Betriebssystem.

## Logo und Plugin-ABI

`assets/noty-logo.png` ist das neue, mit Imagegen erzeugte Original. `noty.ico`, `noty.icns` und der eingebettete 64-px-PNG sind technische App-Icon-Konvertierungen. Das Logo ist ein eigenes Noty-Zeichen; keine Microsoft- oder Notepad++-Marke.

`src/plugin_api.h` beschreibt eine C-ABI v1. Ein Plugin exportiert `noty_plugin_v1`, liefert ABI-Version, UTF-8-Namen und eine synchrone Execute-Funktion. Host-Pointer bleiben nur während dieses Aufrufs gültig. Plugins arbeiten auf dem UI-Thread und dürfen keine Host-Pointer für Hintergrundzugriffe behalten. Nur vertrauenswürdige Plugins laden: nativer Code ist nicht sandboxed.
