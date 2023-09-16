# Application architecture
This application is a [Godot](https://godotengine.org/) project that hooks into custom C++ code through the [GDExtension](https://docs.godotengine.org/en/stable/tutorials/scripting/gdextension/what_is_gdextension.html) API.

The [OpenVDB](https://www.openvdb.org/) C++ library is used for rasterizing the internal SDF representation of the sculpt to a grid, and converting it to a polygonal mesh that Godot can display. It is also used for converting meshes to SDF

C++ code is located inside `/cpp/src/`, with the Godot project sitting at the root of the repository.

In general, C++ is only used for:
- Handling the internal SDF representation of the sculpt
- Rasterizing that representation to a grid and mesh through OpenVDB

Godot and GDScript (its bespoke scripting language) is used for everything else, including:
- Rendering the rasterized mesh
- UI
- Handling input
- VR support



# Build steps

The following steps assume you're on Windows (x86_64). This project has not been tested on other platforms, 
but should in theory still compile with some tweaking. 

## Initial build steps
1. Install scons with `pip install scons` - this requires [python3](https://www.python.org/downloads/)
2. Ensure [VS build tools](https://aka.ms/vs/17/release/vs_BuildTools.exe) are installed, with 
"Desktop development with C++" selected during installation
   - If not on Windows, just ensure you have g++ or clang installed
3. Install OpenVDB and its dependencies, preferably through [vcpkg](https://github.com/microsoft/vcpkg) like so:
   1. Clone https://github.com/microsoft/vcpkg to `./_ignore/`
   2. Run the included bootstrap script (`./_ignore/vcpkg/bootstrap-vcpkg.bat` on Windows)
   3. Install openvdb through vcpkg: 
       ```shell
       ./_ignore/vcpkg/vcpkg install openvdb:x64-windows # Use the relevant version for your OS 
       ```
      If you're not on Windows, not using vcpkg to install openvdb, or not installing it to `/_ignore/`, you may also need to modify the paths defined in `cpp/SConstruct` to reflect its actual install location.
   4. Copy the contents of `./_ignore/vcpkg/installed/x64-windows/bin` to `./bin/`.
4. From the root of the repository, run:
   ```shell
   cd ./cpp/godot-cpp
   scons
   ```
   This will generate C++ bindings for Godot.
5. Run:
   ```shell
   cd ../ # Navigates to cpp/
   scons
   ```
   This will build the necessary C++ code for the application.
6. Download [Godot 4.1](https://godotengine.org/download). It might be easiest to move the executable to `./_ignore/`, 
and rename it to something like `godot.exe`.
7. From the root of the repository, run the Godot editor:
   ```shell
   ./_ignore/godot.exe --editor
   ```
   It will then take some time to import assets.
8. Once the editor has finished importing, click the play button in the top right to confirm that the application works.

Once these steps are complete, you can run the project without the editor with `./_ignore/godot.exe`.

## Re-building during development
- When any files inside `cpp/src/` are modified, you will need to run `scons` in that directory to re-build the C++ code.
The Godot editor should probably be closed whilst this is happening.
- If for some reason files inside `cpp/godot-cpp/` have been modified, you will need to run `scons` again in both `cpp/godot-cpp/` and `cpp/src/`.
- Changes to standard Godot files (`.gd`, `.tscn`, etc) will not require a re-build, as GDScript is an interpreted language.