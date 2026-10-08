<!-- Silicon LogiX / SLX SynthForge · Copyright (c) 2026 Marco Pezzullo. -->
# Silicon LogiX SDK

| Item | Value |
| --- | --- |
| Header | `core/include/SiliconLogiX/SynthForge/Core.h` |
| Namespace | `SiliconLogiX::SynthForge` |
| Core ABI | 4 |
| DLL | `SiliconLogiX.SynthForge.Core.dll` |
| MinGW import library | `SiliconLogiX.SynthForge.Core.dll.a` |
| Included kit | Windows x64, Qt 6.12.0, MinGW GCC 13.1.0 |

```cpp
#include <SiliconLogiX/SynthForge/Core.h>
using SiliconLogiX::SynthForge::Engine;
using SiliconLogiX::SynthForge::Camera;
Engine engine(assetDirectory);
auto acquisition = engine.renderAssembly(edits, Camera{}, {640, 480});
auto report = engine.inspectAssembly(acquisition.image, Camera{}, modelPath, 0.80);
```

This example describes the demonstration API. The Evaluation License does not
authorize commercial reuse in another product. Integration requires a separate
Silicon LogiX license. Export names and Windows company/product metadata identify
the origin; they are not DRM or protection against reverse engineering.

The C function `SiliconLogiX_SynthForge_CoreIdentity()` returns a static UTF-8
string that must not be freed. `slx/core.h` offers a source alias only; ABI 3
binaries are not compatible with ABI 4. Compiler, Qt and architecture must match.
For another approved kit, configure both `SLX_CORE_BINARY` and `SLX_CORE_DLL`.

Inspector acquisitions use 640 × 480, azimuth −65°, tilt 32° and span 133 mm.
The core validates the model and asset identity and requests REVIEW for
unsupported captures, insufficient visibility or scores below threshold.

An Engine may be read by concurrent jobs after construction. Keep the Engine and
cancellation flag alive until all jobs finish. Generation callbacks execute on
the worker thread. Model files contain numeric weights and are readable.

Qt is linked dynamically and can be replaced in accordance with its licenses.
See [runtime replacement instructions](../THIRD_PARTY_SOURCE.md).
