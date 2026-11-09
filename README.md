# Gruppe_JFH_Printfarm

Offline-Desktop-Anwendung (C++ / Qt Widgets) zum Monitoring einer 3D-Druckerfarm.
Umgesetzt nach der Aufgabenstellung „Programmentwurf C++ und Qt — Thema 1: PrintFarm".

## Voraussetzungen / Entwicklungsumgebung

- Qt Creator 20.0.x
- Kit: Desktop Qt 6.11.x MinGW 64-bit (z. B. `C:\Qt\6.11.1\mingw_64`)
- CMake als Buildsystem (Ninja oder MinGW Makefiles)

## Build

In Qt Creator das Projekt `CMakeLists.txt` öffnen und mit dem Kit
„Desktop Qt 6.11.x MinGW 64-bit" bauen. Alternativ auf der Kommandozeile:

```bat
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=C:\Qt\6.11.1\mingw_64
cmake --build build
```

## Start

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
- CSV-Import mit Plausibilitätsprüfungen; robustes Verhalten bei defekten Dateien
- Druckerliste mit Suche/Filter (ID, Modell, Standort, Status, Kinematik), Sortierung
- Detaildialog je Drucker (Stammdaten + Aufträge + Temperaturverlauf + Regelgüte)
- Stammdaten bearbeiten (rollenabhängig), Export als `drucker_out.csv` / `auftraege_out.csv`
- Lazy Loading der JSON-Druckprotokolle über den Master-Index, LRU-Cache (20),
  Laden im Hintergrund (QtConcurrent)
- Regelgüte: Anstiegszeit, Überschwingen (K/%), Ausregelzeit (±2 K),
  mittlere/maximale Regelabweichung der Druckphase (Pausen & Wiederaufheizen ausgeschlossen)
- Fehlererkennung in Protokollen (schlechte Reglereinstellung, Düsenverstopfung,
  Heizungsausfall, verrauschter Fühler, unpassende Betttemperatur)
- Auftragsplanung (Greedy) für geplante Aufträge inkl. Gantt-Liste
- Rollenkonzept: bediener / technik / admin (Login), zentrale Rechteprüfungen
- Light-/Darkmode zur Laufzeit umschaltbar, Deutsch/Englisch umschaltbar
- SQLite-Persistenz (Qt SQL) neben CSV-Import/-Export
- Eigene Erweiterungen: PID-Simulation, Kostenrechnung je Auftrag, Berichtsexport (CSV)

## Tests

Automatisierte Tests mit Qt Test (Import/Export, Validierung, Regelgüte inkl.
Pausenbehandlung, Rechte, Planung, Kosten, PID-Simulation, SQLite, Lazy Loading):

```bat
cmake --build build --target tst_printfarm
build\tst_printfarm.exe        # bzw. ctest im build-Verzeichnis
```

## Verzeichnisstruktur

```
├── CMakeLists.txt
├── src/            Quellen (model/io/logic/gui getrennt)
├── tests/          Qt-Test
├── translations/   Deutsch/Englisch (.ts)
└── QT-Projekt_info/  Aufgabenstellung, Layouts, Prompt, PrintFarm_Daten
```
