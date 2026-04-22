# f3dviewer

A lightweight 3D file preview plugin for [Seer](https://1218.io), built on [F3D SDK](https://github.com/f3d-app/f3d).

## Features

- Supports most 3D file formats (OBJ, STL, GLTF, FBX, USD, and more)
- Interactive 3D viewing with camera controls
- Animation playback support
- Configurable rendering options (grid, edges, point sprites)
- Real-time FPS and metadata display
- Built as a native DLL plugin for Seer 4.0.0+

## Building and Running

Requirements: Qt 6.8+, CMake 3.16+, F3D SDK 3.0+.

1. **Download F3D SDK**

   Download the latest F3D SDK (version 3.0 or later) from:  
   👉 [https://github.com/f3d-app/f3d/releases](https://github.com/f3d-app/f3d/releases)

   Extract it to a location like `../F3D/` relative to this project.

2. **Build**

   ```bash
   cd viewer
   cmake -B build
   cmake --build build
   ```

   This produces two outputs:
   - `f3dviewer.dll` — the Seer plugin
   - `test_f3dviewer.exe` — standalone viewer for testing

3. **Install the plugin**

   Copy `f3dviewer.dll` to your Seer plugins directory.

## Seer Plugin

f3dviewer is a file preview plugin for [Seer](https://1218.io) — a quick-look tool for Windows.

Visit [1218.io](https://1218.io) to download Seer and learn more about the plugin ecosystem.
