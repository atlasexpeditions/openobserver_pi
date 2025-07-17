# openobserver_pi

The Open Observer plugin for OpenCPN is a tool to record marine observations.

This plugin is based on [testplugin_pi](https://github.com/jongough/testplugin_pi) and uses the FrontEnd 2 (FE2) templated build process.

## What is Open Observer?

Every voyage has the potential to lead to an observation of scientific interest, and every boat has the potential to be a sensor providing accurate and consistent data.

Open Observer is a tool to make use of this potential in order to record marine observations.

The Open Observer project has the following goals:

1. Aid the recording of accurate, complete, consistent and structured observations
   - Observation protocols can be easily created, customised and shared.
   - Valid values and formatting can be specified per field.
   - Fields can be automatically populated from boat sensor data where available.
   - Observations can be easily exported and shared.
2. Easy to install, setup and use on any platform
   - Available through the OpenCPN plugin manager
   - Ready and available to use at all times, without obstructing navigation
3. Promote citizen science projects and encourage participation within the sailing, boating and fishing communities

## Who is Open Observer for?

- Mariners who want to contribute to citizen science projects
- Scientists who want to run a citizen science project
- Mariners who want to record other observations during their voyages, separately to their log book

Example projects which Open Observer could be used for include:

- Whale observations and identification
- Sea bird observation and counting
- Plastic waste observation and counting
- Microplastics trawling
- ...

## Why is Open Observer an OpenCPN plugin?

OpenCPN...

- is already widely used and has proven reliability and versatility
- is available on a wide range of platforms
- is open source and freely available
- is supported by an active community of developers
- has an built-in plugin manager, making it easy to distribute, install and update this tool
- can be configured as a hub for boat sensor data, which Open Observer can make use of
- provides a chart-based interface, upon which observations can be overlayed

## Why are we developing Open Observer?

The [Glacialis expedition](https://atlasexpeditions.org/glacialis/) in 2021 was an independent expedition supported by [Atlas Expeditions](https://atlasexpeditions.org) with the aim to collect marine wildlife data, with a focus on large whales, over a 10'000 miles voyage from the Azores archipelago to Disko Bay on the west coast of Greenland.

This experimental initiative proved that a well-prepared small craft can contribute to scientific discovery. In particular, we made unprecedented observations of humpback whales which raised new questions about their migration patterns.

These results were due to the joint effort of a team of trained citizen scientists ([Glacialis](https://atlasexpeditions.org/glacialis/)), supported by a non-profit organisation ([Atlas Expeditions](https://atlasexpeditions.org)) working in direct collaboration with academic and institutional researchers (NAHWC).

At [Atlas Expeditions](https://atlasexpeditions.org), this expedition and the following SILA expedition to southwest Greenland in 2023, taught us that any voyage at sea has the potential to provide a relevant scientific observation, and it is therefore extremely valuable for sailors to be encouraged to go to sea with an observation attitude, the right training and the right tools to record observations in a simple but structured manner. We had the idea that integrating these tools in chart plotters would be very powerful and thus the idea for Open Observer was born.

## Installing Open Observer

Open Observer is still in development and is therefore not yet available directly from the OpenCPN plugin manager.

If you would like to test the latest beta version, follow the steps below.

1. Open https://cloudsmith.io/~aracais/repos/openobserver-beta/ and download the version of the plugin for your OS
    - For MacOS, download the .gz file containing “darwin” in the name e.g. https://dl.cloudsmith.io/public/aracais/openobserver-beta/raw/names/openobserver_pi-0.0.1.0-darwin-wx32-arm64-x86_64-14.1-macos-tarball/versions/0.0.1.0+241.0369d1c/openobserver_pi-0.0.1.0-darwin-wx32-arm64-x86_64-14.1-macos.tar.gz
    - For Windows, download the .gz file containing “msvc” in the name e.g. https://dl.cloudsmith.io/public/aracais/openobserver-beta/raw/names/openobserver_pi-0.0.1.0-msvc-x86-wx32-10.0.20348-MSVC-tarball/versions/0.0.1.0+236.0369d1c/openobserver_pi-0.0.1.0-msvc-x86-wx32-10.0.20348-MSVC.tar.gz
2. Open OpenCPN, open the Preferences panel, go to the Plugins tab and click the button “Import plugin…”. Select the downloaded package and click “Open” to install it.
3. Ensure that the tickbox “Enabled” is ticked.
4. Click on the Open Observer button in the toolbar to open the plugin windows.

# Development

## Install dependencies

Follow the instructions for OpenCPN development to install dependencies for your platform:
https://opencpn-manuals.github.io/main/opencpn-dev/index.html


## Initial repository setup

```
git submodule init
git submodule update
```

## Local build

The simplest way to produce a package that can be imported with the "Import plugin..." button in the OpenCPN plugin manager is to run the following:

```
rm build -rf; mkdir build; cd build; bash ../build-local-package.sh
```
