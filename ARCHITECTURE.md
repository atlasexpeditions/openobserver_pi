# Open Observer Architecture

## Purpose

Open Observer is an OpenCPN plugin for creating structured marine observations from configurable project protocols, vessel position data, NMEA sentences and manual field input.

The plugin is written in C++ with wxWidgets and the OpenCPN plugin API.

## Main plugin entry point

### `src/openobserver_pi.*`

`openobserver_pi` is the OpenCPN-facing plugin class.

It manages:

- plugin initialization and shutdown;
- toolbar integration;
- OpenCPN callbacks for position, viewport and NMEA data;
- plugin preferences;
- creation and lifetime of the main control window and mini interfaces;
- observation highlighting and navigation on the chart;
- integration between observations, NMEA recording and OpenCPN marks.

Changes here can affect plugin lifecycle and platform stability. Test startup, shutdown and repeated window opening carefully.

## User interfaces

### Standard control window

- `src/ooControlDialogDef.*`
- `src/ooControlDialogImpl.*`

`ooControlDialogDef` defines the base dialog and widgets.

`ooControlDialogImpl` contains the operational behaviour: projects, observations, imports, exports, Data Packages, NMEA controls, project editing, filtering, backups and grid behaviour.

The standard control window is the stable primary interface.

### Mini interface

- `src/ooMiniDialogDef.*`
- `src/ooMiniDialogImpl.*`
- `src/ooMiniPanel.*`

These files provide the compact observation interface and its display behaviour.

### Experimental AUI panel

- `src/ooAuiPanel.*`

The AUI mini panel is optional and experimental. It is not part of the stable cross-platform release path because Windows crashes were observed during its destruction.

## Observation and project model

### `src/ooObservations.*`

This is the core data layer.

It defines:

- `ooProject`, including project name, description, field labels, field types, listings, colour and mark icon;
- `ooObservations`, the grid-backed observation table;
- project and observation XML persistence;
- CSV import and export;
- GeoJSON export;
- project listings and configured NMEA fields;
- observation lifecycle, timestamps, NMEA-derived values and OpenCPN mark GUIDs.

Changes to this layer may affect existing projects and exported data. Preserve backward compatibility unless a migration is explicitly designed and reviewed.

### `src/ooProjectFieldMapper.*`

Maps imported or external field names to Open Observer project field types and helps keep imports aligned with the configured project schema.

### `src/ooTableImport.*`

Handles table and CSV import workflows.

## NMEA and automated logging

### `src/ooNmeaRecorder.*`

Records incoming NMEA sentences while an observation is active.

It manages temporary and final recording files and provides the final recording path linked to an observation when the project contains an `NMEA Recording` field.

### `src/ooDataLogger.*`

Manages scheduled or interval-based observation logging for a selected project.

### NMEA flow

```text
OpenCPN NMEA callback
        ↓
openobserver_pi
        ↓
ooControlDialogImpl / ooObservations
        ↓
optional ooNmeaRecorder and ooDataLogger
```

NMEA changes require tests with real or representative sentences and must be checked for clean stop/restart behaviour.

## Exports, packages and chart interaction

### `src/ooScientificPackage.*`

Creates and updates structured Data Packages for sharing, archival and downstream work.

### `src/ooGpxTrackExport.*`

Exports raw OpenCPN GPX tracks for selected observation dates or compiled package output.

Raw track structure is intentionally preserved. Do not silently join independent tracks or alter track geometry.

### `src/ooObservationHighlight.*`

Controls observation highlighting and navigation on the OpenCPN chart.

## Resources

### `data/`

Packaged user-facing resources include:

- default project templates;
- Data Package templates;
- controlled-value listings;
- user icons;
- plugin icons and logos.

The build copies these resources into the installed plugin package. Resource changes should be tested in an imported package, not only in a local source tree.

## Build and packaging

### `CMakeLists.txt`

Defines Open Observer version metadata, compilation sources and plugin build configuration.

### `cmake/`

Contains inherited OpenCPN plugin build, installation, packaging and localization support.

### GitHub Actions

- `.github/workflows/build-macos.yml`
- `.github/workflows/build-windows-win32.yml`

Both workflows:

1. configure and build a release package;
2. create an OpenCPN `-IMPORT.tar.gz` archive;
3. verify package resources;
4. upload the import archive and generated XML metadata.

The generated import archive must contain `metadata.xml` at its root.

## Important constraints

- Do not commit files from `build/`.
- Do not casually regenerate `ooControlDialogDef.*` from wxFormBuilder.
- Do not change the AUI mini panel lifecycle without dedicated Windows validation.
- Do not alter project or observation XML formats without considering existing user data.
- Do not change package layout without importing the resulting archive in OpenCPN.
- Keep raw GPX export faithful to OpenCPN source tracks.

## Typical change map

| Change needed | First files to inspect |
|---|---|
| Plugin lifecycle, Preferences, OpenCPN callbacks | `openobserver_pi.*` |
| Main window controls and behaviour | `ooControlDialogDef.*`, `ooControlDialogImpl.*` |
| Projects, observations, XML, CSV, GeoJSON | `ooObservations.*` |
| Project field mapping or CSV import | `ooProjectFieldMapper.*`, `ooTableImport.*` |
| NMEA recording | `ooNmeaRecorder.*`, `openobserver_pi.*`, `ooObservations.*` |
| Scheduled data logging | `ooDataLogger.*`, `ooControlDialogImpl.*` |
| Data Packages | `ooScientificPackage.*` |
| GPX exports | `ooGpxTrackExport.*` |
| Packaged icons, listings and templates | `data/`, `cmake/PluginInstall.cmake` |
| CI packages | `.github/workflows/` |
