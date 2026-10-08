<!-- Silicon LogiX / SLX SynthForge · Copyright (c) 2026 Marco Pezzullo. -->
# PowerBoard visual assets

The demonstration uses original Silicon LogiX geometry and a component manifest.
The board is a visual reference only, with no electrical or manufacturing validation.

| Asset | Role |
| --- | --- |
| `SLX_PowerBoard_nominal.glb` | Nominal 3D assembly |
| `SLX_PowerBoard_missing_C1.glb` | Missing capacitor example |
| `SLX_PowerBoard_misaligned_U4.glb` | Displaced IC example |
| `SLX_PowerBoard_reversed_C1.glb` | Reversed polarity example |
| `SLX_PowerBoard_solder_bridge_U4.glb` | Solder bridge example |
| `instances.json` | Component references, nominal poses and instance IDs |

The board measures 96 × 72 mm. The manifest has 50 references: 46 mounted
components and four mounting holes. The editable assembly targets the 46 mounted
references. Scene geometry, visible labels and instance annotations share the
same asset identity. The core checks that identity against models and experiments.

The published demonstration includes GLB scenes and the manifest. Native CAD
projects, STEP files and asset-generation tooling are not required to run it.
