# Comparing Static and Dynamic Human-Robot Task Sharing - Experiment Software

## Getting started

### Windows
#### Disk space and path length
vcpkg compiles all dependencies from source. **Reserve at least 70 GB** of free disk space on the drive that will host the repository. Use a **short repository path** (e.g. `C:\hrc`) — MSVC imposes a maximum path length and deeply nested vcpkg build trees will exceed it on longer paths.

#### VCPKG
Checkout this project and submodules, then install vcpkg dependencies using: 
 ```bash
  cd .\tools\vcpkg
  .\bootstrap-vcpkg.bat
  .\vcpkg.exe install boost-serialization catch2 poco[net] eigen3 nlohmann-json opencv realsense2 pcl[vtk] spdlog --triplet x64-windows
```
Note: right now, poco is only necessary to build libfranka.
#### Qt
As of now, we don't recommend building Qt with vcpkg. Instead, download their prebuild binaries and add the location of your Qt installation to the "CMAKE_PREFIX_PATH" in the file CMakeUserPresets.json.

The following Qt components must be installed (select them in the Qt Online Installer):
- Qt Core
- Qt GUI
- Qt Quick / Qt Quick Controls 2
- Qt Quick 3D
- Qt QML
- Qt Widgets

### Linux 
Our vcpkg dependencies have themselves dependencies on the distribution's package manager:
```bash
  sudo apt-get install build-essential tar curl zip unzip  -y && 
  sudo apt-get install bison -y && 
  sudo apt-get install python3-setuptools -y && 
  sudo apt-get install libx11-dev libxft-dev libxext-dev -y && 
  sudo apt-get install libxmu-dev libxi-dev libgl-dev -y && 
  sudo apt-get install autoconf -y && 
  sudo apt-get install libtool -y && 
  sudo apt-get install xorg-dev -y && 
  sudo apt-get install autoconf-archive -y && 
  sudo apt-get install libdbus-1-dev libxi-dev libxtst-dev  -y && 
  sudo apt-get install libudev-dev -y && 
  sudo apt-get install libxt-dev -y && 
  sudo apt-get install libfmt-dev -y && 
  sudo apt-get install libglvnd-dev -y && 
  sudo apt-get install libx11-dev libgles2-mesa-dev -y &&
  sudo apt-get install libfontconfig1-dev   libfreetype6-dev   libx11-dev    libx11-xcb-dev libxext-dev   libxfixes-dev    libxi-dev    libxrender-dev    libxcb1-dev    libxcb-glx0-dev    libxcb-keysyms1-dev    libxcb-image0-dev    libxcb-shm0-dev    libxcb-icccm4-dev    libxcb-sync-dev    libxcb-xfixes0-dev    libxcb-shape0-dev    libxcb-randr0-dev    libxcb-render-util0-dev    libxcb-util-dev    libxcb-xinerama0-dev    libxcb-xkb-dev    libxkbcommon-dev    libxkbcommon-x11-dev -y &&
  sudo apt-get install python3-dev 
  sudo apt-get install flex
    sudo apt-get install '^libxcb.*-dev' libx11-xcb-dev libglu1-mesa-dev libxrender-dev libxi-dev libxkbcommon-dev libxkbcommon-x11-dev libegl1-mesa-dev
```
Aferwards, you can install the dependencies as usual:
```shell
  cd ./tools/vcpkg
  sudo ./bootstrap-vcpkg.sh
  ./vcpkg install boost-serialization catch2 poco eigen3 qtbase qtquick3d nlohmann-json opencv  realsense2 pcl[vtk] spdlog
```

### Python integration
1. Install Python 3.11+ and ensure it is on your system `PATH`. Required packages: `networkx`, `matplotlib`. Make sure you downloaded the debug binaries during install.
2. CMake's `FindPython` will locate Python automatically via `PATH` and the Windows registry. If you need to pin a specific installation, set `Python_ROOT_DIR` as a system environment variable pointing to that directory.

Hint: If you encounter issues, check the CMake output to see which Python version is found and used.

### Building

#### Option A — Command line

**Windows** (run inside a *Visual Studio 2022 Developer Command Prompt* so `cl.exe` is on `PATH`):

```cmd
cmake --preset x64-debug
cmake --build out/build/x64-debug
```

Use `x64-release` for a `RelWithDebInfo` build. Run the result:

```cmd
out\build\x64-debug\src\App\application.exe
```

**Linux** — first add a Linux preset to `CMakeUserPresets.json` (the file is not committed; create it if it does not exist):

```json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "linux-debug",
      "displayName": "Linux Debug",
      "inherits": "linux-base",
      "environment": {
        "CMAKE_PREFIX_PATH": "/home/<you>/Qt/6.8.2/gcc_64",
        "Python_ROOT_DIR": "/usr",
        "PYTHONHOME": "/usr"
      }
    }
  ]
}
```

Then configure, build, and run:

```bash
cmake --preset linux-debug
cmake --build out/build/linux-debug
out/build/linux-debug/src/App/application
```

**Tests (both platforms):**

```bash
cd out/build/<preset> && ctest
```

#### Option B — IDE (GUI)

**Windows — Visual Studio 2022**

1. Open Visual Studio 2022 and choose **File → Open → CMake...**, then select `CMakeLists.txt` at the repository root. Visual Studio has built-in CMake support and reads `CMakePresets.json` automatically.
2. Set machine-specific paths in `CMakeUserPresets.json` as described in [Option A](#option-a--command-line) above. Visual Studio picks up the file on the next re-configure.
3. Select the desired preset (`x64-debug` or `x64-release`) from the configuration drop-down in the toolbar.
4. Build with **Build → Build All** or **Ctrl+Shift+B**.
5. To run, set the startup item to `application.exe` via the run drop-down and press **F5** (debug) or **Ctrl+F5** (without debugger).

**Linux — Qt Creator**

1. Open Qt Creator and choose **File → Open File or Project**, then select `CMakeLists.txt`.
2. Qt Creator detects the presets from `CMakePresets.json`. Select `linux-debug` in the **Configure Project** dialog.
3. If machine-specific paths are not yet set, create or edit `CMakeUserPresets.json` as described in [Option A](#option-a--command-line) above. Qt Creator picks up the file automatically on the next re-configure.
4. Click **Build** (hammer icon) or press **Ctrl+B**.
5. To run, select the `application` target in the kit selector and press **Ctrl+R**.

### Other
For development you can switch to a simulated robot by changing the connect_to_hardware property of the task in App/Experiment.qml

## Hardware configuration

### TCP offset
The TCP offset in `src/franka_robot.cpp` (`flange_T_tcp`) is set to 0.124 m along the flange Z-axis — the measured distance from the Franka flange face to the tip of the Franka Hand fingers. If your gripper or finger geometry differs, update this value.

### Force-torque sensor adapter
During the study a force-torque sensor was mounted between the robot flange and the gripper as part of a parallel experiment (adding 0.065 m to the TCP offset and 0.372 kg to the tool mass). These contributions are **commented out** in the release version so the software works with the bare Franka Hand out of the box. If you mount a sensor or other adapter between the flange and gripper, restore and adjust the following:

- `src/franka_robot.cpp` — `flange_T_tcp`: uncomment `+ 0.065` and set the adapter height.
- `src/franka_util.cpp` — `tool_mass()`, `tool_center_of_mass()`, `tool_inertia()`: set `m_fts` to the sensor mass and verify the center-of-mass position `c_fts`.

### Camera poses

Two sets of poses in `src/franka_robot.cpp` are workspace-specific and must be verified when the robot or workspace layout changes:

- **Exploration poses** (lines 48–60, `franka_robot` constructor): five overhead TCP poses (x, y, z in metres relative to the robot base) through which the robot sweeps during the exploration sequence to locate all parts with the RealSense camera. Adjust these if the workspace footprint or robot mounting position changes.
- **Camera-to-flange transform** (`world_T_camera`, lines 183–195): a hardcoded 4×4 affine matrix (`flange_T_camera`) describing the pose of the RealSense D435i relative to the Franka flange. Re-measure and update this matrix if the camera mount is repositioned or replaced.

## Running a study session

### Loading a task
There are two ways to point the application at a task:

**Option A — configuration file (recommended for study runs).**
Use *File → Open* and select a `configuration.json` file (e.g. `studies/JHu_MA/TaskA/configuration_dynamic.json`). The file specifies the task description path, which actors are enabled (human / robot), which operation indices trigger an interruption, and whether the scheduler re-plans after an interruption. All settings are applied at once.

**Option B — manual settings.**
Use *Tools → Options* to set the task description path and override individual scheduling parameters. This is useful for quick tests without a full configuration file.

> **Important:** always load a task before pressing *Start*. Starting without a loaded task description causes the application to crash.

### Running the experiment
1. Start the application from the build output directory (`out/build/<preset>/`).
2. Load a configuration file via *File → Open*.
3. Press **Start**. The robot begins its exploration sequence to locate parts.
4. Once the task is running, the button label changes to **Assembly step completed**. The participant presses this after each completed human assembly step.
5. An interruption dialog appears automatically at the configured operation indices. After the secondary task, the participant presses **I am back** to resume.
6. Questionnaires (NASA-TLX, SUS, Fluency, Demographics, Experience) are accessible under the *Tools* menu and can be filled in between runs.
7. Study data is logged automatically as JSONL in the working directory.

### Simulation mode
Set `connect_to_hardware: false` on the `Task` element in `src/App/Experiment.qml` to run without physical hardware. The software will simulate robot actions and camera observations, which is useful for testing scheduling logic.

## Questionnaire strings

There is no central translation file — questionnaire item text is hardcoded inline as `ListModel`/`ListElement` entries directly in the QML files under `src/App/`. Sites that want to translate the questionnaires (or use a different validated instrument version) should edit the `label`/`description` strings directly in:

| File | Questionnaire |
|---|---|
| `src/App/NASA_TLX.qml` | NASA-TLX dimension labels/descriptions and pairwise comparison labels |
| `src/App/SUS.qml` | System Usability Scale (SUS) items |
| `src/App/ATI.qml` | Affinity for Technology Interaction (ATI) items |
| `src/App/PerformanceInteraction.qml` | Human-robot fluency/teamwork items |
| `src/App/Experience.qml` | Prior-experience questions |
| `src/App/Demographics.qml` | Demographics form (labels, gender/answer options) |

`TLXRatings.qml` and `TLXWeights.qml` are sub-views loaded by `NASA_TLX.qml`; they render the `questionaire`/`comparisions` models rather than defining their own text.

## Operation timing for scheduling

The scheduler assigns every operation the same flat duration in seconds for a given agent. These are hardcoded constants at the top of `core::schedule::schedule()` in `src/core/hrc_schedule.cpp`:

```cpp
const int robot_speed = 10;
const int human_speed = 5;
const int interrupt_duration = 30;
```

- `robot_speed` / `human_speed` — duration (in scheduler time steps) assigned to every operation for the robot / human agent respectively, passed to the Python `list_scheduler` as each agent's per-operation duration.
- `interrupt_duration` — number of time steps the human is marked unavailable when scheduling around an interruption.

To change how long human or robot steps are assumed to take (and thus how the scheduler interleaves operations), edit these three constants directly.