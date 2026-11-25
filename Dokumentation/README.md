# PrintFarm_Gruppe_03

Offline-Desktop-Anwendung (C++ / Qt Widgets) zum Monitoring einer 3D-Druckerfarm.
Umgesetzt nach der Aufgabenstellung „Programmentwurf C++ und Qt, Thema 1: PrintFarm".

## Anmeldung (Testzugänge)

Rollen: `bediener` / `technik` / `admin` mit Standardpasswörtern
`bediener123` / `technik123` / `admin123` (siehe `src/auth.cpp` und
`Dokumentation/Testzugänge.md`; überschreibbar per Umgebungsvariable
`PRINTFARM_PW_BEDIENER` / `PRINTFARM_PW_TECHNIK` / `PRINTFARM_PW_ADMIN`).
Der Benutzername ist frei wählbar. Nur `technik` und `admin` dürfen
Reglerparameter ändern, nur `admin` Stammdaten wie Status und Standort.

## Voraussetzungen / Entwicklungsumgebung

- Windows mit Qt 6.11.x inkl. **MinGW 64-bit-Kit** (empfohlen: Qt Creator 20.0.x)
- Qt-Tools: MinGW, CMake, Ninja (Standardauswahl im Qt-Online-Installer)
- CMake als Buildsystem (Ninja oder MinGW Makefiles)

## Build

**Variante 1 (empfohlen, läuft auf jedem Windows-PC):** Doppelklick auf `build.bat`.
Das Skript findet Qt, MinGW, CMake und Ninja automatisch (PATH, `C:\Qt`, `D:\Qt`,
`%SystemDrive%\Qt`, `%USERPROFILE%\Qt`; jeweils höchste Qt-6.x-Version mit
MinGW-Kit) und meldet verständlich, was fehlt. Optional übersteuern:
`QT_HOME` (Qt-Basisordner) und `MINGW_DIR` (MinGW-`bin`-Ordner).
Fällt Ninja aus, schaltet es automatisch auf MinGW-Makefiles um.

Nach jedem erfolgreichen Build:

- werden per `windeployqt` alle Qt-Laufzeitdateien neben die EXE kopiert
  (inkl. `libgcc_s_seh-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll`),
- wird die Verknüpfung **`PrintFarm_Gruppe_03.lnk`** im Hauptordner angelegt/erneuert
  („Ausführen in" = Projektordner, damit der Datenordner automatisch gefunden wird).

**Variante 2:** In Qt Creator das Projekt `CMakeLists.txt` öffnen und mit dem Kit
„Desktop Qt 6.11.x MinGW 64-bit" bauen.

**Variante 3 (manuell, eigene Pfade):**

```bat
cmake -B build -G Ninja -DCMAKE_PREFIX_PATH=<Qt-Pfad> -DCMAKE_CXX_COMPILER=<g++.exe>
cmake --build build
<Qt-Pfad>\bin\windeployqt.exe --release build\PrintFarm_Gruppe_03.exe
```

## Start

- Doppelklick auf **`PrintFarm_Gruppe_03.lnk`** im Hauptordner (empfohlen), oder
- `build\PrintFarm_Gruppe_03.exe` direkt (Arbeitsverzeichnis = Projektordner).

Die Anwendung ist offline-fähig und verwendet relative Pfade. Beim Start wird
automatisch nach einem Datenordner gesucht (`data/` bzw.
`QT-Projekt_info/PrintFarm_Daten/PrintFarm_Daten` relativ zum Arbeitsverzeichnis).
Über **Datei → Datenordner wählen...** kann ein beliebiger Datenordner geladen werden.

Datenstruktur (im Datenordner):

```
├── drucker.csv, modelle.csv, materialien.csv, auftraege.csv, wartungen.csv
├── master_protokoll_index.csv
└── druckprotokolle_batch_XX/   (JXXXXX.json + protokoll_index.csv)
```

## Funktionen

- Vier Haupttabs: **Druckerliste**, **Gedruckte Teile**, **Material**, **Fehlerlog**
- CSV-Import mit Plausibilitätsprüfungen (Duplikat-IDs, Wertebereiche, Referenzen,
  ungültige Zahlen/Daten werden geloggt); robustes Verhalten bei defekten Dateien
- Druckerliste mit Suche/Filter (ID, Modell, Standort, Status, Kinematik), Sortierung
- Detaildialog je Drucker (Stammdaten + Aufträge + Wartungen + Temperaturverlauf + Regelgüte)
- Stammdaten bearbeiten (rollenabhängig: Stammdaten nur admin, PID nur technik/admin),
  Export als `drucker_out.csv` / `auftraege_out.csv`
- Lazy Loading der JSON-Druckprotokolle über den Master-Index, LRU-Cache (20),
  nicht-blockierendes Laden im Hintergrund (QtConcurrent)
- Regelgüte: Anstiegszeit, Überschwingen (K/%), Ausregelzeit (±2 K),
  mittlere/maximale Regelabweichung der Druckphase (Pausen & Wiederaufheizen ausgeschlossen)
- Fehlererkennung in Protokollen (schlechte Reglereinstellung, Düsenverstopfung,
  Heizungsausfall, verrauschter Fühler, unpassende Betttemperatur) mit dokumentierten Schwellen
- Auftragsplanung (Greedy/SJF nach Soll-Dauer, freie Zeit ab letzter Belegung/Wartung)
  für ausstehende Aufträge inkl. Gantt-Darstellung (nur admin)
- Light-/Darkmode zur Laufzeit umschaltbar, Deutsch/Englisch umschaltbar (vollständig übersetzt)
- SQLite-Import **und** -Export (Qt SQL, Menü Datei) neben CSV-Import/-Export; Duplikate
  werden geloggt (letzter Eintrag gewinnt, konsistent zur Map)
- Eigene Erweiterungen:
  - **PID-Simulation** (`src/pidsim.*`): physikalische Strecke (PT2 + Totzeit + Stellglied,
    Verluste linear + Strahlung ~T⁴, auf reale Arbeitspunkte kalibriert), Regler mit
    Anti-Windup und D auf Messwert, Störgrößen (Extrusion, Lüfter, Fühlerrauschen),
    Rangliste mehrerer Parameter-Kandidaten, optionaler Fit der Strecke aus dem neuesten
    echten Aufheizprotokoll des Druckers
  - **Kostenrechnung** je Auftrag (Material + Maschinenstunden, Annahme 12 €/h dokumentiert)
  - **Berichtsexport** (Wochenbericht, Kostenübersicht, Wartungsplan als CSV)

## Code-Konventionen

- Alle Bezeichner (Klassen, Funktionen, Variablen) sind **Englisch**; **Kommentare sind Deutsch**
- Trennung in `src/`: Datenhaltung (`datastore`, `datatypes`), Datei-I/O (`csvio`, `protocol`),
  Datenbank (SQLite in `datastore`), fachliche Logik (`analyse`, `regelguete`, `pidsim`),
  GUI (`mainwindow`, `*widget`, `dialogs`, `plotwidget`, `ganttwidget`, `tablemodels`), Theme-Daten (`theme.h`)
- UI-Texte bleiben Deutsch (Quellsprache) und werden über `translations/*.ts` ins Englische übersetzt

## Hinweis zur Doku-PDF (Abweichungen)

Die `Dokumentation_PrintFarm_Gruppe_03.pdf` nennt an zwei Stellen noch einen
älteren Stand und weicht damit vom Code ab (eine Neuerzeugung der PDF war nicht
praktikabel; diese README und die Anforderungsmatrix dokumentieren hier den
tatsächlichen Stand):

- **Reglersimulation:** Die PDF behauptet in „Bekannte Einschränkungen“ ein
  „Streckenmodell erster Ordnung“. Der Code (`src/pidsim.h` / `src/pidsim.cpp`)
  simuliert eine **PT2-Strecke + Totzeit** (Block- und Fühlerkapazität,
  SSR-Ansprechverzögerung, Totzeit-Ringpuffer) mit temperaturabhängigen
  Verlusten (linear + Strahlung ~T^4).
- **Auftragsplanung:** Die PDF beschreibt „frühester Fertigstellungszeitpunkt“.
  Der Code (`Planung::greedy` in `src/analyse.cpp`) sortiert die ausstehenden
  Aufträge als **SJF nach `dauerSollMin`** (kürzester Job zuerst) und wählt je
  Auftrag den bereitstehenden, material-geeigneten Drucker mit dem frühesten
  Fertigstellungszeitpunkt.

## Tests

Alle Tests liegen im dedizierten Ordner **`tests/`** (Qt Test). Neue Tests bitte
immer dort ablegen (z. B. `tests/tst_<thema>.cpp`) und in `tests/CMakeLists.txt`
als eigenes Test-Target registrieren.

Bestehende Test-Targets:

- `tst_printfarm`: fachliche Logik ohne GUI (53 Tests): CSV-Import/Export, Validierung,
  Regelgüte inkl. Pausenbehandlung, Rechte (Passwort-Login + Logout), Planung, Kosten,
  PID-Simulation (inkl. Realismus gegen echte Daten, Rangliste, Fit, Windup, T_max),
  SQLite-Persistenz (inkl. Duplikate, fehlende Tabellen), Lazy Loading sowie Security-Tests
  (Path Traversal, CSV-Formelinjection)
- `tst_gui`: GUI-QA headless (offscreen) mit den echten Projektdaten (35 Tests):
  Tabs, Filter/Suche, Sortierung, Dialoge, Theme/Sprache (EN ohne deutsche Fallbacks),
  Plot, Rangliste/Fit im PID-Dialog, Rechte-Trennung, Wartungs-Tabelle

Ausführen (nach dem Build):

```bat
cd build
ctest --output-on-failure
```

## Verzeichnisstruktur

```
├── CMakeLists.txt
├── build.bat                    portables Build-Skript (Baut + Deployment + Verknuepfung)
├── src/                         Quellen (Datenhaltung, Logik, GUI getrennt)
├── tests/                       alle Qt-Tests
├── translations/                Deutsch/Englisch (.ts/.qm)
├── Dokumentation/               Dokumentation (PDF), Anforderungsmatrix, Testzugänge, diese README
└── QT-Projekt_info/             Aufgabenstellung, Layoutentwuerfe, Prompt, PrintFarm_Daten
```

## Abgabe

Zip-Datei mit vollständigem, lauffähigem Projekt (ohne `build/`), der repräsentativen
Datenbasis (alle globalen CSV-Dateien, mindestens ein Batch-Ordner mit Index) sowie der
Dokumentation als PDF (`Dokumentation/Dokumentation_PrintFarm_Gruppe_03.pdf`).
Ordnername: **PrintFarm_Gruppe_03**.
