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

**GitHub Actions compiles the SUPRX successfully, but it is not tested on Vita hardware.** A passing build is not proof of correct framebuffer behaviour or PSP/PS1 detection.

Do not add it to the live ur0:tai/config.txt yet. The first hardware test should be performed only after a successful build, on a recoverable test setup, with the existing plugin configuration backed up. Remove the single *ALL entry and reboot to disable it.

The diagnostic also records the SceAppMgr process CATEGORY in read-only mode; this can describe NPXS10028 rather than the actual game and must not activate filtering. See [automatic PSP detection](../../docs/AUTO_PSP_DETECTION.md). The report is a temporary diagnostic artifact, not production logging. Delete ux0:data/PSPSharp_compat_probe.txt after collecting it. No graphics filter is implemented by this probe.


## ABI correction

The hook uses the six-argument SceDisplayUser wrapper ABI. The source framebuffer is argument 5; argument 6 is the optional conversion/capture destination; argument 3 is not dereferenced. The source descriptor may be 0x18 or 0x1C bytes. This has compiled in GitHub Actions, but remains untested in hardware.
