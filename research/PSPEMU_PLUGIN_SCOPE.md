# PSPemu plugin-scope research (stock PSPemu / Chovy-Sign)

**Date:** 2026-10-31  
**Target:** PS Vita 1000, firmware 3.60 + Ensō; official PSP emulator; Chovy-Sign bubbles; no Adrenaline.

## Executive result

A process-scoped research path is now demonstrated more concretely than the generic SceCompat framebuffer route:

- The official PSP emulator application is `NPXS10028` and its native process/module is identified as `ScePspemu`.
- A user-mode taiHEN plugin loaded from `*ALL` can detect `ScePspemu` with `taiGetModuleInfo("ScePspemu", ...)` and then install hooks against imports belonging to that module.
- The open-source NoPspEmuDrm plugin uses exactly this pattern. It has a global `*ALL` user plugin and a kernel plugin; its user module detects `ScePspemu`, obtains module information, and hooks `sceIoOpen` / `sceIoGetstat` in that module.
- A separate project, remaPSV, explicitly skips initialization when the current title is `NPXS10028` or a `PSPEMU` bubble, noting that some apps refuse to start if its plugin is active. This is a warning against blindly placing a generic user plugin in `*NPXS10028`.

This establishes a viable **plugin discovery/injection pattern**, not yet a viable framebuffer-filter implementation.

## Primary source evidence

### 1. taiHEN plugin loading

taiHEN documents:
- `*KERNEL` for kernel plugins loaded at taiHEN startup.
- Title-ID sections for user plugins loaded with the corresponding application.
- `*ALL` as the global application section in common configurations.

Source: https://github.com/yifanlu/taiHEN

### 2. PSPemu identity

The Vita firmware module map identifies `vs0:app/NPXS10028/eboot.bin` as `compat/pspemu`, with the process named `ScePspemu`.

Source: https://github.com/shoui520/psp2/blob/0261b5803d8108135d6cd8dc8660834d865b4e24/release/modules.tsv

### 3. Working user-plugin pattern in NoPspEmuDrm

Source repository: https://github.com/LiEnby/NoPspEmuDrm

Its README config uses:
```
*KERNEL
ur0:/tai/NoPspEmuDrm_kern.skprx
*ALL
ur0:/tai/NoPspEmuDrm_user.suprx
```

Its user `Main.c` calls `taiGetModuleInfo("ScePspemu", &tai_info)`. If found, it calls `pspemu_module_start(tai_info)`. The PSPemu-specific module then obtains `SceKernelModuleInfo` and hooks `sceIoOpen` and `sceIoGetstat` imports in module `ScePspemu`.

Its `PspEmu.c` also documents internal helper functions `scePspemuConvertAddress` and `scePspemuWritebackCache`, with reverse-engineered offsets reported as stable across firmware 3.60–3.74. It uses those helpers to translate ePSP addresses into Vita-side memory and to maintain cache coherency.

**Important:** NoPspEmuDrm is a DRM patch, not a graphics filter. Its code is evidence that process-specific PSPemu user-mode hooks are possible, not that its memory routines are safe or suitable for a renderer.

### 4. Generic-plugin incompatibility warning

Source: https://github.com/Rinnegatamante/remaPSV/blob/ff3b6d607b1ef0019ac654b63720fcf1361fef23/main.c

remaPSV obtains the active title ID and returns without initializing for `NPXS10028` (commented as Adrenaline) and title IDs containing `PSPEMU` (commented as ABM bubbles), because some apps refuse to start while that plugin is active.

This does not prove all plugins are incompatible. NoPspEmuDrm demonstrates a PSPemu-aware plugin pattern. It does mean a test plugin must be minimal, avoid broad imports/hooks, and be tested in a reversible setup.

## What this changes for PSP-Sharp-Vita

### Better candidate architecture

1. A small user `.suprx` loaded globally using `*ALL`.
2. At startup, do only a cheap `taiGetModuleInfo("ScePspemu")` check.
3. If ScePspemu is absent, immediately return `SCE_KERNEL_START_NO_RESIDENT` (or otherwise unload cleanly).
4. If present, collect module metadata and verify the expected firmware/module identity before installing any hook.
5. Initially add diagnostics only to a development build; no framebuffer writes.
6. Later, identify a stable PSPemu presentation callback or source framebuffer with confirmed format, dimensions, stride, buffering, and synchronization.
7. Keep the production plugin silent and limited to PSPemu only.

### Why not start with `*NPXS10028`

Although `NPXS10028` is the PSPemu app title ID, remaPSV's explicit compatibility guard warns that user plugins loaded directly for that title can interfere with application startup. The safer research candidate is the proven `*ALL` + module-detection pattern used by NoPspEmuDrm, with a tiny no-op path in all non-PSPemu processes.

### Remaining graphics blockers

No source reviewed so far proves all of the following:
- which PSP-side address/buffer is the currently displayed frame for every Chovy-Sign PSP title;
- whether the source buffer is 480×272 or another size in all modes;
- pixel format, pitch/stride, and buffer count;
- whether the frame is CPU-readable/writable at the proposed hook point;
- safe cache maintenance and synchronization that do not race the GPU or display scanout;
- whether modifying the source buffer causes PSPemu to overwrite the changes before presentation.

The documented PSP memory map lists a PSP VRAM/framebuffer region at `0x04000000`, but that is not by itself proof of the active display surface or a safe write point:
https://www.psdevwiki.com/vita/PSP_Emulator

## Safety decision

**Proceed with research and a no-op loader prototype only. Do not install a graphics hook on the user's primary Vita yet.**

Do not add `*NPXS10028` to the user's `ur0:tai/config.txt` at this stage. Do not reuse NoPspEmuDrm's reverse-engineered PSPemu offsets for rendering without independently validating the exact firmware 3.60 binary and all memory/synchronization assumptions.

## Next technical task

Inspect the PSPemu module's display path and determine whether it exposes a stable import or internal function that receives the frame buffer immediately before presentation. Cross-check the 3.60 retail firmware specifically. Only after that should a diagnostic-only prototype be designed.

## New lead: SceDisplaySetFrameBufForCompat import

Adrenaline's public source provides a particularly useful lead for the display path:
https://github.com/TheOfficialFloW/Adrenaline/blob/7d382b7837d9d211d830017ba7aee982fa49a8c6/user/main.c

It installs an import hook in module `ScePspemu` for library NID `0x4FAACD11`, function NID `0x8C36B628`, using the patched function `sceDisplaySetFrameBufForCompatPatched`. The patch signature receives a `SceDisplayFrameBuf *pParam` as its final argument.

Implementation:
https://github.com/TheOfficialFloW/Adrenaline/blob/7d382b7837d9d211d830017ba7aee982fa49a8c6/user/pops.c

The current Adrenaline patch only substitutes a custom `pops_data` display buffer when `config.graphics_filtering != 0` and `pParam == NULL`. This is evidence that the compat display import can be intercepted, but it does **not** establish that the NULL case is the normal PSP-game presentation path; the `pops_data` naming and separate POPS handling indicate that this specific substitution is associated with PS1/POPS rendering.

### Proposed diagnostic-only experiment

On firmware 3.60 retail, build a user plugin that follows the proven `*ALL` + `taiGetModuleInfo("ScePspemu")` pattern, then hooks only the `SceDisplaySetFrameBufForCompat` import in `ScePspemu`. The hook must:

1. Never change `pParam`, its contents, or the return value.
2. Record only whether `pParam` is NULL; when non-NULL, record `size`, `base`, `pitch`, `pixelformat`, `width`, and `height`.
3. Rate-limit logging or capture only the first few calls; no per-frame file I/O.
4. Verify the import hook succeeds and that PSP and PS1 titles still launch/exit normally.
5. Be disabled by removing its single `*ALL` entry and rebooting.

This experiment would reveal whether the import exposes a useful PSP image surface and its metadata without touching pixels. It is not yet an image filter and must not be installed on the user's primary Vita until the binary is reviewed and a recovery path is prepared.

### Firmware-specific evidence

Adrenaline checks `module_nid == 0x2714F07D` as the 3.60 retail ScePspemu build before applying several internal-offset hooks. Its `sceDisplaySetFrameBufForCompat` import hook is installed before that conditional, so the source demonstrates the import hook's use in the PSPemu process; still verify the exact import resolution on the user's 3.60 retail firmware before compiling a diagnostic.