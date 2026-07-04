# Open Observer

Open Observer is an open-source OpenCPN plugin for structured marine observations.

It helps mariners, expedition teams, citizen-science projects and researchers record observations at sea with consistent manual inputs, vessel data, positions, timestamps and configurable project protocols.

## Status

**Open Observer 0.2.1** is an early public release.

The current release has been validated in OpenCPN on:

- macOS Apple Silicon
- Windows 32-bit

It is intended for real use, while broader field feedback, additional platform testing and documentation continue to strengthen the project.

## What Open Observer does

Open Observer supports:

- Configurable observation projects and field protocols
- Manual observation data combined with automatically captured vessel and NMEA data
- Project-specific listings, controlled values, icons and colours
- OpenCPN observation marks
- Observation exports in CSV, XML and GeoJSON
- Raw OpenCPN GPX track exports
- Data Packages for structured project sharing and archival
- Optional NMEA stream recording during observations
- Links between observations and their associated NMEA recordings
- Access to recorded NMEA files from the observations table

Typical use cases include:

- Marine mammal observations and identification
- Seabird surveys
- Pollution and marine debris records
- Environmental sampling
- Citizen-science observation campaigns
- Expedition and voyage field notes

## Installation

Release packages are distributed as OpenCPN import archives.

1. Download the package matching your platform from the relevant Open Observer release.
2. Open OpenCPN.
3. Go to **Options → Plugins**.
4. Select **Import plugin...**
5. Select the downloaded `-IMPORT.tar.gz` archive.
6. Enable Open Observer in the Plugins list.
7. Use the Open Observer toolbar icon to open the plugin.

Do not extract the import archive manually before installation.

## NMEA recording

Open Observer can optionally record the incoming NMEA stream while an observation is active.

To link a recording automatically to an observation, the project must contain a field whose **Field Type** is `NMEA Recording`.

When this field is present, Open Observer can store the recording filename in the observation and later provide direct access to the linked file.

## Data Packages

Data Packages provide a structured folder for sharing, archiving and further processing observation projects.

They can include:

- Project XML
- Observation exports
- NMEA recordings when available
- Raw OpenCPN GPX track exports
- Daily working folders for media and related field material
- A working area for maps, reports, notes and analysis files

Generated data is kept separate from user working material so that future package updates do not overwrite project notes or media.

## Development

Open Observer is developed as an OpenCPN plugin using CMake, wxWidgets and the OpenCPN plugin API.

Clone the repository with its submodules:

    git clone --recurse-submodules https://github.com/atlasexpeditions/openobserver_pi.git
    cd openobserver_pi

A standard local build can then be configured with CMake:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build -j 2

Platform-specific dependencies follow the OpenCPN plugin development environment.

Importable OpenCPN release archives are produced by the GitHub Actions workflows.

## Contributing

Bug reports, reproducible test results, protocol ideas, documentation improvements and field feedback are welcome.

Before proposing a change, please describe:

- OpenCPN version
- Operating system and architecture
- Plugin version
- Steps needed to reproduce the issue
- Expected and observed behaviour

## License

Open Observer is distributed under the terms of the GNU General Public License, version 3 or later.

See [LICENSE](LICENSE).

## Project identity and attribution

Open Observer is developed by Atlas Expeditions and contributors.

Forks and modified versions are welcome under the GPL. Please do not present a modified version as an official Open Observer or Atlas Expeditions release, and do not imply endorsement by Atlas Expeditions where none exists.

## Origins

Open Observer grew from expedition and citizen-science work carried out by Atlas Expeditions, including marine observation programmes developed during voyages to Greenland, across the Atlantic and in other field environments.

The project aims to make structured, shareable and scientifically useful observation workflows more accessible to people already navigating at sea.
