# Ignition

A racing car game project built with the Ignis Game Engine, demonstrating the engine's capabilities including 3D rendering, physics, audio, and scripting.

- **Car Control:** WASD to move, R to reset car's position, ESC to pause menu

Please check out the main Ignis engine repository for more details: https://github.com/JamesLiu12/Ignis

Note: we have removed some model files in the assets folder as they are copyright protected. You may find them in release, but they are password protected for internal use.

## 🚀 Getting Started

### Prerequisites

- **CMake** 3.20 or higher
- **C++20** compatible compiler
  - Windows: Visual Studio 2022 or later (with C++ desktop development workload)
  - macOS: Xcode Command Line Tools
- **Ninja** build system
  - Windows: Visual Studio 2022 has Ninja by default, or install via Visual Studio Installer
  - macOS: `brew install ninja`
- **Git** for version control
- **Homebrew** for macOS only

### Building the Editor

#### Windows

```bash
# Clone the repository with submodules
git clone --recursive https://github.com/your-username/Ignition.git
cd Ignition

# Configure with preset
cmake --preset x64-debug
# or for Release build
cmake --preset x64-release

# Build (builds Editor, Runtime, and script module)
cmake --build --preset x64-debug
# or for Release build
cmake --build --preset x64-release
```

#### macOS

```bash
# Install dependencies
brew install cmake ninja

# Clone the repository with submodules
git clone --recursive https://github.com/your-username/Ignition.git
cd Ignition

# Configure with preset
cmake --preset arm64-debug
# or for Release build
cmake --preset arm64-release

# Build (builds Editor, Runtime, and script module)
cmake --build --preset arm64-debug
# or for Release build
cmake --build --preset arm64-release
```

### Running the Editor

After building, run the editor executable:

**Windows:**

```bash
# Debug build
out\build\x64-debug\Ignis\bin\Editor.exe

# Release build
out\build\x64-release\Ignis\bin\Editor.exe
```

**macOS:**

```bash
# Debug build
out/build/arm64-debug/Ignis/bin/Editor

# Release build
out/build/arm64-release/Ignis/bin/Editor
```

The editor will open and load the example project file `Ignition/Ignition.igproj` using the project menu tab → load project

## 🎮 Exporting Ignition Game

### Using the Editor

1. **Open the project** in the Editor
2. If you modify scripts, **Click "Project" → "Build Scripts"** in the menu bar
3. **Click "Project" → "Export Game"** in the menu bar
4. The export process will:
   - Validate that Runtime and script module are built
   - Package all necessary files (executable, DLLs, assets, scripts)
   - Create a distribution folder

### Export Location

The exported game will be located at:

```
Ignition/
└── Exports/
    └── Ignition_Distribution/
        ├── Ignition.exe (Windows) or Ignition (macOS)
        ├── Ignis.dll (Windows) or libIgnis.dylib (macOS)
        ├── Other dependency DLLs/dylibs
        ├── scripts/
        │   └── Ignition.dll (Windows) or libIgnition.dylib (macOS)
        ├── assets/
        │   ├── models/
        │   ├── images/
        │   ├── audio/
        │   └── scenes/
        └── Ignition.igproj
```

### Running the Exported Game

Navigate to the export folder and run the executable:

**Windows:**

```bash
cd Exports\Ignition_Distribution
Ignition.exe
```

**macOS:**

```bash
cd Exports/Ignition_Distribution
./Ignition
```

The game will run as a standalone application without requiring the editor.

## 📁 Project Structure

```
Ignition/
├── Ignis/                    # Ignis engine (git submodule)
│   ├── Ignis/                # Core engine library
│   ├── Editor/               # Editor application
│   └── Runtime/              # Standalone runtime
├── assets/                   # Project assets
│   ├── models/               # 3D models (FBX, OBJ)
│   ├── images/               # Textures and HDR environments
│   ├── audio/                # Sound effects and music
│   ├── fonts/                # Font files
│   ├── scenes/               # Scene files (.igscene)
│   ├── scripts/              # C++ script source files
│   └── TestAR.igar           # Asset registry
├── bin/                      # Script module output (DLL/dylib)
│   └── {Platform}/{Config}/
├── out/                      # Build output directory
│   └── build/{preset}/
├── Exports/                  # Exported game distributions
├── Ignition.igproj           # Project configuration file
└── CMakeLists.txt            # Build configuration
```

## 🛠️ Game Development Workflow For Ignis Editor

1. **Edit Scripts and Create Game Contents** - Modify C++ scripts in `assets/scripts/` and ceate game content
2. **Rebuild** - Click Project → Build Scripts in menu tab if you create or modify scripts
3. **Test in Editor** - Use plat mode in editor to test changes
4. **Export** - Create standalone build for distribution

## 📄 License

This example project is provided as-is for demonstration purposes. See the main Ignis engine repository for license information.

## 🤝 Contributing

This is an example project for the Ignis Game Engine. For engine contributions, please see the main Ignis repository.
