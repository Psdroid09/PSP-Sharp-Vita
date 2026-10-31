# PSPemu read-only diagnostic probe

This is a diagnostic user plugin for investigating the stock PSP emulator's compatibility framebuffer call. It is **not a graphics filter**.

## Behavior

- Target: Vita firmware 3.60 retail PSPemu module (`module_nid` 0x2714F07D).
- Intended loading pattern: taiHEN `*ALL`; non-PSPemu processes return non-resident immediately.
- Hooks only `SceDisplaySetFrameBufForCompat` in `ScePspemu`.
- Preserves the six-argument ABI: argument 5 is the optional source descriptor; argument 6 is only recorded as present/absent; argument 3 is never dereferenced.
- Calls the original firmware function first. It inspects the source descriptor only if the original call succeeds, and accepts only exact descriptor sizes 0x18 or 0x1C.
- Retains at most eight metadata records. Counters/slot allocation use atomic increments.
- Never dereferences the framebuffer pixel address, changes descriptors/flags, replaces buffers, changes the return value, or calls display APIs recursively.
- Writes one small report to `ux0:data/PSPSharp_compat_probe.txt` on plugin unload; no per-frame file I/O.

The source descriptor remains caller-owned. Reading its metadata after a successful firmware call reduces risk because the wrapper has already accepted/copied the descriptor, but this is not a formal memory-safety guarantee. Hardware validation is still required.

## Build

Requires VitaSDK and taiHEN headers/stubs:

~~~sh
cmake -S src/diagnostic -B build/diagnostic \
  -DCMAKE_TOOLCHAIN_FILE="$VITASDK/share/vita.toolchain.cmake" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build/diagnostic
~~~

Expected output: `PSPSharpProbe.suprx`.

## Validation status

- **CI build passed** for the initial diagnostic source at commit `bf5048e`; artifact `PSPSharpProbe-suprx` was uploaded.
- A follow-up source-hardening change is being built separately; the earlier artifact does not contain those changes.
- No test has been performed on a physical Vita.
- No evidence yet proves that PSP game sessions submit a non-NULL descriptor, that its base is CPU-readable, or that a filter can run without racing GPU/display access.

**Do not install this plugin yet and do not add it to the live `ur0:tai/config.txt`.** The first hardware test requires a reviewed build, a reversible test setup, and a prepared recovery route. No changes to the user's Vita configuration or SD2Vita are part of this work.

The report is temporary diagnostic output, not production logging. Delete `ux0:data/PSPSharp_compat_probe.txt` after collecting it. No graphics filter is implemented by this probe.