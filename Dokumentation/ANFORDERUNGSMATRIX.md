# Anforderungsmatrix zu PrintFarm_Gruppe_03

Umsetzungsstand gegen Aufgabenstellung (PDF) und Prompt.txt.

Code-Konvention: Alle Bezeichner (Klassen, Funktionen, Variablen) sind Englisch,
alle Kommentare Deutsch (siehe README, Abschnitt „Code-Konventionen").

## Mindestanforderungen (50 Punkte)

| Nr. | Anforderung | Umsetzung | Status |
|-----|-------------|-----------|--------|
| 1 | Einlesen der globalen CSVs beim Start, Klassen/Datenstrukturen, Plausibilitätsprüfungen | `DataStore::loadFromFolder`, `validateConsistency()` (Duplikat-IDs, Wertebereiche, Referenzen, ungültige Zahlen/Daten geloggt), Fehlerlog | implementiert, getestet |
| 2 | Listen-/Detailansichten (Druckerliste, Detaildialog mit Stammdaten + Aufträgen + Wartungen) | Tab „Druckerliste" (QTableView + PrinterModel), `PrinterDetailDialog` inkl. WartungModel | implementiert, getestet |
| 3 | Suche/Filter in Druckerliste | Textsuche (ID/Modell/Standort), Status-, Kinematik-Filter | implementiert, getestet |
| 4 | Bearbeitung von Stammdaten, Speicherung + CSV-Export (`*_out.csv`) | `PrinterEditDialog`, rollenabhängig (Stammdaten nur admin, PID nur technik/admin), `DataStore::exportCsv` | implementiert, getestet |
| 5 | Verlaufsanzeige pro Auftrag (JSON, Lazy Loading, Master-Index, ≥2 Verläufe) | `ProtocolLoader` (lazy, LRU-Cache 20, nicht-blockierend via QtConcurrent), PlotWidget (Düse + Bett) | implementiert, getestet |
| 6 | Regelgüte (Anstiegszeit, Überschwingen K/%, Ausregelzeit, Ø/max Abweichung Druck) | `ControlQualityCalculator` (Pausen & Wiederaufheizen ausgeschlossen, echte Zeitstempel) | implementiert, getestet |
| 7 | Fehlerbehandlung (fehlende/defekte Dateien, Degradation) | Fehlerlog-Tab mit Kategorien (Import/Validierung/Datei/Protokoll), Hinweise bei fehlenden Protokollen | implementiert, getestet |

## Erweiterungen (max. 18 Punkte werden angerechnet)

| Nr. | Erweiterung | Umsetzung | Status |
|-----|-------------|-----------|--------|
| 1 | Erweiterte Listen/Statistiken (Wartungen, Auftragsliste, Erfolgsquote, Fehlercodes, Verbrauch, Kosten) | Tab „Material" (Statistik + 6h-Plot), JobModel, Wartungs-Tabelle im Detaildialog | implementiert, getestet |
| 2 | Kombinierte Filter und Sortierungen | mehrstufige Filter + sortierbare Tabellen (alle Modelle) | implementiert, getestet |
| 3 | Rollen-/Rechtekonzept (bediener/technik/admin) | `Auth` (Session, SHA-256 + Salt, Env-Override), LoginDialog, zentrale Rechteprüfungen auch außerhalb der GUI, Export nur angemeldet | implementiert, getestet |
| 4 | Benutzerfreundliche GUI (Tooltips, Kontextmenüs) | Kontextmenü Druckerliste, Tooltips, Vollbild (F) | implementiert, getestet |
| 5 | Caching & Performance (LRU 10–20, Hintergrundladen) | LRU-Cache (20) + QtConcurrent in `ProtocolLoader`, Anzeige erst beim fertig-Signal; LogModel mit gecachter Index-Liste, Log gedeckelt (5000) | implementiert, getestet |
| 6 | Fehlererkennung in Protokollen | `ErrorDetection::detectFaults` (Regler, Düse, Heizung, Fühler, Betttemp.) mit dokumentierten Schwellen-Konstanten | implementiert, getestet |
| 7 | Auftragsplanung (Greedy) + Gantt | `Planning::assignJobsGreedy` (**SJF nach `plannedMinutes`**, freie Zeit ab letzter Belegung/Wartung), Gantt-Dialog (nur admin) | implementiert, getestet |
| 8 | Regleranalyse (Vergleich mehrerer Sprungantworten, Rangliste) | `PlotWidget::showComparison`, `PidSim` mit `rankParameterCandidates` + Rangliste im `PidSimDialog` | implementiert, getestet |
| 9 | i18n (DE/EN, Laufzeit-Umschaltung) | QTranslator + app_de/app_en.ts (vollständig übersetzt, EN ohne Fallbacks getestet) | implementiert, getestet |
| 10 | Persistenz-Upgrade (SQLite, Qt SQL) | `DataStore::exportSqlite/importSqlite` (alle 5 Tabellen, Transaktion, INSERT OR REPLACE mit Duplikat-Log), Import auch im Menü Datei | implementiert, getestet |

## Eigene Erweiterungen (max. 6 Punkte)

| Erweiterung | Umsetzung | Status |
|-------------|-----------|--------|
| PID-Simulation (PT2 + Totzeit + Stellglied; Verluste linear + Strahlung ~T⁴; Regler mit Anti-Windup, D auf Messwert; Störgrößen; Presets + Fit aus echten Aufheizdaten; Rangliste) | `src/pidsim.h/.cpp`, `PidSimDialog` | implementiert, getestet (inkl. Realismus gegen echte Protokolle) |
| Kostenrechnung je Auftrag (Material + Maschinenstunden, 12 €/h Annahme dokumentiert) | `CostCalculator::computeJobCost`, Anzeige in Tab 2 | implementiert, getestet |
| Berichtsexport (Wochenbericht, Kostenübersicht, Wartungsplan als CSV) | `ReportDialog` | implementiert, getestet |

## GUI-Vorgaben

- Genau vier Haupttabs in Reihenfolge: Druckerliste, Gedruckte Teile, Material, Fehlerlog
- Light-/Darkmode mit Laufzeit-Umschaltung (Menü Ansicht, Theme-Daten in `src/theme.h`)
- Geplante/abgebrochene/fehlgeschlagene Aufträge über Ergebnis-Filter in Tab 2 erreichbar
- Fehlerlog unterscheidet Import-, Validierungs-, Datei- und Protokollfehler

## Hinweis zur Doku-PDF (Abweichungen)

Die `Dokumentation_PrintFarm_Gruppe_03.pdf` weicht an zwei Stellen vom
tatsächlichen Code ab (PDF-Neuerzeugung war nicht praktikabel; diese Matrix
und die README dokumentieren hier den maßgeblichen Stand):

- Die PDF nennt in „Bekannte Einschränkungen" ein „Streckenmodell erster
  Ordnung" – umgesetzt ist **PT2 + Totzeit** (`src/pidsim.h` / `src/pidsim.cpp`):
  zwei Kapazitäten (Block, Fühler), SSR-Ansprechverzögerung und Totzeit-Ringpuffer.
- Die PDF beschreibt die Planung als „frühester Fertigstellungszeitpunkt" –
  umgesetzt ist **SJF nach `plannedMinutes`** (`Planning::assignJobsGreedy`,
  `src/analyse.cpp`): kürzeste Aufträge zuerst, je Auftrag der Drucker mit
  dem frühesten Ende.
