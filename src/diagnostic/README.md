# PSPemu read-only diagnostic probe (source only)

This folder contains the first native plugin prototype for investigating the stock PSP emulator's compatibility framebuffer call.

## What it does

- Targets Vita firmware 3.60.
- Is intended to load from taiHEN's *ALL section.
- Calls taiGetModuleInfo("ScePspemu") and returns non-resident in every other app.
- Hooks only SceDisplaySetFrameBufForCompat in the ScePspemu module.
- Inspects the fifth argument as the optional source framebuffer descriptor and records the sixth argument only as an optional conversion/capture destination.
- Copies metadata from at most eight non-NULL source descriptors; counts calls where the source pointer is NULL.
- Does not read pixel data, modify framebuffer memory, change flags, replace buffers, or alter the original arguments/return value.
- Writes one small report to ux0:data/PSPSharp_compat_probe.txt when the plugin unloads. There is no per-frame file I/O.

## Build

Requires VitaSDK and taiHEN headers/stubs:

~~~sh
cmake -S src/diagnostic -B build/diagnostic \
  -DCMAKE_TOOLCHAIN_FILE="$VITASDK/share/vita.toolchain.cmake" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build/diagnostic
~~~

Expected output: PSPSharpProbe.suprx.

## Important status

**Not compiled or tested yet.** The current development environment does not contain the VitaSDK cross-compiler. This source must be built and reviewed before any installation is considered.

Do not add it to the live ur0:tai/config.txt yet. The first hardware test should be performed only after a successful build, on a recoverable test setup, with the existing plugin configuration backed up. Remove the single *ALL entry and reboot to disable it.

The report is a temporary diagnostic artifact, not production logging. Delete ux0:data/PSPSharp_compat_probe.txt after collecting it. No graphics filter is implemented by this probe.


## ABI correction

The hook uses the six-argument SceDisplayUser wrapper ABI. The source framebuffer is argument 5; argument 6 is the optional conversion/capture destination; argument 3 is not dereferenced. The source descriptor may be 0x18 or 0x1C bytes. This is still uncompiled and untested.
