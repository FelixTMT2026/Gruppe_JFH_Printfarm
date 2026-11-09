# Anforderungsmatrix — Gruppe_JFH_Printfarm

Umsetzungsstand gegen Aufgabenstellung (PDF) und Prompt.txt.

## Mindestanforderungen (50 Punkte)

| Nr. | Anforderung | Umsetzung | Status |
|-----|-------------|-----------|--------|
| 1 | Einlesen der globalen CSVs beim Start, Klassen/Datenstrukturen, Plausibilitätsprüfungen | `DataStore::loadFromFolder`, `plausibilitaet()` (Modell-/Material-/Temperatur-Prüfungen), Log | implementiert, getestet |
| 2 | Listen-/Detailansichten (Druckerliste, Detaildialog mit Stammdaten + Aufträgen) | Tab „Druckerliste" (QTableView + DruckerModel), `DruckerDetailDialog` | implementiert |
| 3 | Suche/Filter in Druckerliste | Textsuche (ID/Modell/Standort), Status-, Kinematik-Filter | implementiert |
| 4 | Bearbeitung von Stammdaten, Speicherung + CSV-Export (`*_out.csv`) | `DruckerEditDialog`, rollenabhängig, `DataStore::exportCsv` | implementiert, getestet |
| 5 | Verlaufsanzeige pro Auftrag (JSON, Lazy Loading, Master-Index, ≥2 Verläufe) | `ProtocolLoader` (lazy, LRU, async), PlotWidget (Düse Soll/Ist, Bett) | implementiert, getestet |
| 6 | Regelgüte (Anstiegszeit, Überschwingen K/%, Ausregelzeit, Ø/max Abweichung Druck) | `RegelgueteRechner` (Pausen & Wiederaufheizen ausgeschlossen) | implementiert, getestet |
| 7 | Fehlerbehandlung (fehlende/defekte Dateien, Degradation) | Fehlerlog-Tab mit Kategorien, Hinweise bei fehlenden Protokollen | implementiert |

## Erweiterungen (max. 18 Punkte werden angerechnet)

| Nr. | Erweiterung | Umsetzung | Status |
|-----|-------------|-----------|--------|
| 1 | Erweiterte Listen/Statistiken (Wartungen, Auftragsliste, Erfolgsquote, Fehlercodes, Verbrauch, Kosten) | Tab „Material" (Statistik), AuftragModel | implementiert |
| 2 | Kombinierte Filter und Sortierungen | mehrstufige Filter + sortierbare Tabellen | implementiert |
| 3 | Rollen-/Rechtekonzept (bediener/technik/admin) | `Auth`, LoginDialog, zentrale Rechteprüfungen | implementiert, getestet |
| 4 | Benutzerfreundliche GUI (Tooltips, Kontextmenüs) | Kontextmenü Druckerliste, Tooltips | implementiert |
| 5 | Caching & Performance (LRU 10–20, Hintergrundladen) | LRU-Cache (20) + QtConcurrent in `ProtocolLoader` | implementiert |
| 6 | Fehlererkennung in Protokollen | `Fehlererkennung` (Regler, Düse, Heizung, Fühler, Betttemp.) | implementiert |
| 7 | Auftragsplanung (Greedy) + Gantt | `Planung::greedy`, Gantt-Liste in Tab 2 | implementiert, getestet |
| 8 | Regleranalyse (Vergleich mehrerer Sprungantworten, Rangliste) | PlotWidget::zeigeVergleich, PID-Simulation | implementiert |
| 9 | i18n (DE/EN, Laufzeit-Umschaltung) | QTranslator + app_de/app_en.ts | implementiert |
| 10 | Persistenz-Upgrade (SQLite, Qt SQL) | `DataStore::exportSqlite/importSqlite` | implementiert, getestet |

## Eigene Erweiterungen (max. 6 Punkte)

| Erweiterung | Umsetzung | Status |
|-------------|-----------|--------|
| PID-Simulation mit einstellbaren Parametern | `PidSim`, `PidSimDialog` | implementiert, getestet |
| Kostenrechnung je Auftrag (Material + Maschinenstunden) | `Kostenrechnung`, Anzeige in Tab 2 | implementiert, getestet |
| Berichtsexport (Wochenbericht, Kostenübersicht, Wartungsplan als CSV) | `BerichtDialog` | implementiert |

## GUI-Vorgaben

- Genau vier Haupttabs in Reihenfolge: Druckerliste, Gedruckte Teile, Material, Fehlerlog
- Light-/Darkmode mit Laufzeit-Umschaltung (Menü Ansicht)
- Geplante/abgebrochene/fehlgeschlagene Aufträge über Ergebnis-Filter in Tab 2 erreichbar
- Fehlerlog unterscheidet Import-, Validierungs-, Datei- und Protokollfehler
