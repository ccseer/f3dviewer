# f3dviewer

A lightweight 3D viewer and Seer plugin based on [F3D SDK](https://github.com/f3d-app/f3d).

## Features

- Compatible with most 3D file formats supported by F3D

## Build

### Requirements

- Qt 6.x (Widgets + OpenGL)
- F3D SDK (version 3.0.0 or later)
- CMake 3.16 or newer

### Setup

1. **Download F3D**

   Download the latest release of F3D SDK from:  
   👉 [https://github.com/f3d-app/f3d/releases](https://github.com/f3d-app/f3d/releases)

2. **Modify `CMakeLists.txt`**

   Edit the `CMakeLists.txt` file and set the correct path to your local F3D installation

3. **Build the project**

   Open the project in Qt Creator or Visual Studio, then simply build it.

   Make sure you select the correct Qt kit (Desktop Qt 6.x) if prompted.

## TODO:

- scale font for fps/metadata
- animation control
- use args as options from plugin.json 
