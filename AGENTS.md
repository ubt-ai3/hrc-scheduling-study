# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

A C++17/Qt6 application for human-robot collaboration (HRC) experiments with a Franka Panda robot. The app coordinates assembly tasks between a human operator and the robot, captures questionnaire data (NASA-TLX, SUS, ATI, Demographics), and logs study data as JSONL.

## Build System

Dependencies are managed via **vcpkg** (in `tools/vcpkg/`). Qt is installed separately (not via vcpkg on Windows). Build presets are defined in `CMakePresets.json`; machine-specific paths (Qt location, Python) go in `CMakeUserPresets.json`.

**Configure and build (Windows):**
```
cmake --preset x64-debug
cmake --build out/build/x64-debug
```
or use the `x64-release` preset for `RelWithDebInfo`.

**Run the main application:**
```
out/build/x64-debug/src/App/application.exe
```

**Run tests (Catch2):**
```
cd out/build/x64-debug && ctest
```
or run the test binary directly: `out/build/x64-debug/tests.exe`

**CMake targets:**
- `application` — main Qt Quick 3D GUI
- `tests` — Catch2 unit tests (`src/tests/main.cpp`)
- `hardware_test` — standalone Franka hardware sanity check (`src/test_franka.cpp`)
- `ColorCalibration` — standalone tool for calibrating HSV color thresholds

## Architecture

### Layer overview

```
src/App/          Qt Quick 3D GUI (QML + C++ entry point)
src/core/         Business logic exposed to QML (task, schedule, operation, settings, robot_handler)
src/              Hardware abstractions (franka_robot, cameras, motion generators, world model)
src/vision/       Computer vision (pose estimation, entity detection via OpenCV/PCL)
src/core/scheduler_py/  Python scheduling algorithm, called at runtime via pybind11
```

### Key classes

| Class / File | Role |
|---|---|
| `core::task` (QML: `Task`) | Central QML element; owns schedule, robot handler, logger; drives the experiment state machine |
| `core::schedule` | Ordered lists of human/robot `Operation*`; advances as ops complete |
| `core::robot_handler` | Thread-safe queue that serializes robot commands; subclasses: `hardware_robot_handler` (real Franka) and `simulated_robot_handler` |
| `benchmark::abstract_robot` | Pure-virtual robot API (`pick`, `place`, `explore`, `move_to`); implemented by `franka_robot` |
| `benchmark::abstract_camera` / `Camera` | Camera abstraction; `realsense_camera` wraps librealsense2 |
| `vision::pose_estimation` | Detects LEGO-style assembly parts via HSV color thresholds + PCL point cloud; returns `Entity` positions |
| `core::json_logger` | Writes timestamped JSONL log files to the working directory during a study run |
| `core::settings` (QML: `Settings`) | Runtime parameters: task description path, scheduling flags, interrupt indices |

### Python scheduler

`src/core/scheduler_py/` contains the HRC scheduling algorithm (networkx-based). It is embedded at runtime via **pybind11** and called from `core::task::reschedule()`. Python 3.11+ with `networkx` and `matplotlib` is required. The scheduler files are copied to the build output directory post-build.

### QML UI structure

`src/App/main.qml` is the root. `Experiment.qml` hosts the `Task {}` element and drives the main experiment flow. Scene rendering uses Qt Quick 3D with 3D models from `src/App/HrcModels/` and `src/App/HrcModels/meshes/`. `src/App/Mock/ConsoleLogger.qml` is a no-op logger used in the Qt Designer preview.

### Study data

`studies/` contains per-study subdirectories. Each task variant has a `benchmark_task/task_description.json` and a `configuration_dynamic.json` / `configuration_static.json`. The configuration file is loaded at runtime via *File → Open* and points the application at the correct task description.

## Configuration files

`CMakeUserPresets.json` must be updated for each machine:
- `CMAKE_PREFIX_PATH` — path to the Qt 6 MSVC installation (e.g. `C:/Qt/6.8.2/msvc2022_64`)
- `Python_ROOT_DIR` / `PYTHONHOME` — path to Python 3.11+ installation

## Simulation mode

Set `connect_to_hardware: false` on the `Task` element in `src/App/Experiment.qml` to run without physical hardware. The `simulated_robot_handler` substitutes for real robot actions.

## TCP offset and tool mass

`flange_T_tcp` in `src/franka_robot.cpp` is the distance from the Franka flange face to the tool tip (default: 0.124 m for the bare Franka Hand). If a force-torque sensor or adapter is added, also update `tool_mass()`, `tool_center_of_mass()`, and `tool_inertia()` in `src/franka_util.cpp` (see the commented-out `m_fts` values).
