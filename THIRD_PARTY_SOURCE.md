<!-- Silicon LogiX / SLX SynthForge · Copyright (c) 2026 Marco Pezzullo. -->
# Corresponding runtime sources

The Windows distribution dynamically links unmodified Qt 6.12.0 and the
GCC 13.1.0 / MinGW-w64 11 runtime. `runtime-sources/` contains the complete
source archives for Qt Base, Declarative, SVG, Shader Tools, GCC and MinGW-w64,
along with MinGW build scripts, patches, build information and Qt SPDX files.
`runtime-sources/manifest.json` records download origins, sizes, SHA-256 and
runtime DLL fingerprints. Python and NumPy are separately installed and are not bundled.

Source origins: [Qt 6.12.0](https://download.qt.io/archive/qt/6.12/6.12.0/submodules/),
[GCC 13.1.0](https://ftp.gnu.org/gnu/gcc/gcc-13.1.0/),
[MinGW-w64 11.0.0](https://github.com/mingw-w64/mingw-w64/releases/tag/v11.0.0),
[MinGW build scripts](https://github.com/niXman/mingw-builds).
The build script snapshot is selected at the installed runtime build timestamp;
all GCC runtime patches listed by that build are included. The installed
`mingw-build-info.txt` supplies compiler/runtime configuration and provenance.
No changes to Qt or these runtime DLLs were made by Silicon LogiX.

## Replace or rebuild Qt

1. Unpack the matching Qt module archives. Follow each module's build instructions
   and use the Windows x64 compiler kit identified by the SDK.
2. Build an ABI-compatible dynamic Qt installation with the needed modules and
   plugins. Preserve the required runtime names, plugin paths and QML modules.
3. Back up the demo folder and replace the Qt DLLs, plugins and QML libraries
   with that build. Run `slx-synthforge.exe --smoke-test` to check startup.
4. The source demo can also be rebuilt with `scripts/build-windows.ps1` and the
   matching SDK. A different compiler/Qt ABI requires a corresponding Silicon
   LogiX SDK binary.

There is no signature or licensing check preventing replacement of Qt libraries.
The Evaluation License preserves LGPL replacement, relinking and related
reverse-engineering rights. Applicable LGPL/GPL texts, runtime exception and
third-party notices are in `licenses/` and the provided source archives.

When distributing binaries, keep these source materials available with the
same release. The source checkout itself does not contain the Qt runtime;
its prebuilt SDK requires a separately installed matching Qt kit.
