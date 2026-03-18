## Dependency Management

- Windows x64: vendored dependencies in `third_party/lib/windows/x64` and headers in `third_party/include` (`GL/*` and `glm/*`).
- Linux x64: vendored static libraries in `third_party/lib/linux/x64` and headers in `third_party/include` (`GL/*` and `glm/*`).
- macOS: system frameworks (`OpenGL`, `GLUT`) plus Homebrew `freeimage`.
- macOS and non-bundled builds use system-provided `glm`.

## macOS (Apple Clang)

Prerequisites:

- Xcode command line tools
- Homebrew
- `brew install freeimage glm`

Configure and build:

```bash
cmake -S . -B build-mac
cmake --build build-mac -j4
```

Run:

```bash
./build-mac/bin/RayTracer 1 1 1
```

## Linux x64 (GCC/Clang)

Prerequisites:

- OpenGL/X11 development packages for your distro (for `GL`, `GLU`, `glut`, `X11`, `pthread`, `dl`, `m`)
- Vendored archives present:
  - `third_party/lib/linux/x64/libGLEW.a`
  - `third_party/lib/linux/x64/libfreeimage.a`
  - `third_party/include/glm/glm.hpp`

Configure and build:

```bash
cmake -S . -B build-linux
cmake --build build-linux -j4
```

Run:

```bash
./build-linux/bin/RayTracer 1 1 1
```

## Windows x64 (Visual Studio 2022)

Prerequisites:

- Visual Studio 2022 (Desktop C++)
- **Microsoft Visual C++ Redistributable** installed
- `FreeImage.dll` currently depends on `VCOMP140.DLL` (OpenMP runtime)

Configure and build:

```powershell
cmake -S . -B build-win -G "Visual Studio 17 2022" -A x64
cmake --build build-win --config Release
```

Run:

```powershell
.\build-win\bin\Release\RayTracer.exe 1 1 1
```

`glew32.dll`, `freeglut.dll`, and `FreeImage.dll` are copied post-build to the executable directory.

## Useful CMake Options

- `-DRAYTRACER_USE_BUNDLED_DEPS=ON` (default): use vendored Windows/Linux libs.
- `-DRAYTRACER_THIRD_PARTY_DIR=/abs/path/to/third_party`: override vendored dependency location.
- Single-config generators (for example Ninja/Unix Makefiles) default to `Release` for performance. Set `-DCMAKE_BUILD_TYPE=Debug` when debugging.
