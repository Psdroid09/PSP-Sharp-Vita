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

The current Adrenaline patch's sixth argument is not the primary source framebuffer. The six-argument firmware wrapper maps argument 5 to the source framebuffer pointer and argument 6 to the optional conversion/capture destination. Adrenaline substitutes its POPS buffer into argument 6 when NULL; that is the optional conversion destination, not the source framebuffer.

### Proposed diagnostic-only experiment

On firmware 3.60 retail, build a user plugin that follows the proven `*ALL` + `taiGetModuleInfo("ScePspemu")` pattern, then hooks only the `SceDisplaySetFrameBufForCompat` import in `ScePspemu`. The hook must use the firmware-verified six-argument wrapper signature and inspect the **fifth argument** as the optional source framebuffer descriptor. It must:

1. Never change the arguments, option block, nested framebuffer descriptors, or return value.
2. Validate the source descriptor size before reading it; record `base`, `pitch`, `pixelformat`, `width`, `height`, and `flags` only when its size is valid. Treat argument 6 as a separate optional conversion destination.
3. Treat the fifth/sixth sized parameters as optional capture-related arguments; do not confuse the sixth argument with the source framebuffer.
4. Rate-limit logging or capture only the first few calls; no per-frame file I/O.
5. Verify the import hook succeeds and that PSP and PS1 titles still launch/exit normally.
6. Be disabled by removing its single `*ALL` entry and rebooting.

This experiment would reveal whether PSPemu submits a source compatibility framebuffer through this import and provide its descriptor metadata without touching pixels. It is not yet an image filter and must not be installed on the user's primary Vita until the binary is reviewed and a recovery path is prepared.

### Firmware-specific evidence

Adrenaline checks `module_nid == 0x2714F07D` as the 3.60 retail ScePspemu build before applying several internal-offset hooks. Its `sceDisplaySetFrameBufForCompat` import hook is installed before that conditional, so the source demonstrates the import hook's use in the PSPemu process; still verify the exact import resolution on the user's 3.60 retail firmware before compiling a diagnostic.

## Follow-up audit: display descriptor semantics (Vita SDK headers)

The Vita SDK header defines the SceDisplayFrameBuf structure as exactly 0x18 bytes:

- size
- base (framebuffer pointer)
- pitch (in pixels)
- pixelformat
- width
- height

Source: https://github.com/vitasdk/vita-headers/blob/e66ebe90b73d1fa4cce005a5b2072cec27322544/include/psp2common/display.h

The same header documents an extended 0x1C-byte descriptor, SceDisplayFrameBufExt, accepted by firmware 3.60. Its additional flags field includes SCE_DISPLAY_FRAMEBUF_FLAG_BILINEAR (0x00100000) and SCE_DISPLAY_FRAMEBUF_FLAG_SHARED (0x00040000). It also says that when the 0x18-byte descriptor is used, the display driver sets the bilinear flag internally.

### Interpretation and limits

This is useful evidence about the Vita display driver's framebuffer descriptor and a possible built-in bilinear presentation mode. It is not proof that the PSPemu compat import exposes a writable game-render target, nor that changing its descriptor flags from a plugin is safe or even honored by that compatibility API.

The proposed diagnostic hook should therefore copy only the six documented fields from a non-NULL pParam after validating pParam->size is at least the known descriptor size. It must not dereference base, modify flags, substitute a buffer, or call display APIs recursively. The pointer may describe a surface being handed to the display subsystem rather than a stable PSP source texture.

### Revised decision on the Adrenaline lead

The sceDisplaySetFrameBufForCompat import remains a read-only observation candidate, not an implementation route. Adrenaline's patch only changes the NULL case and supplies a POPS buffer; its code does not show a PSP-game-specific pixel filter. The next experiment must first establish whether ordinary PSP game sessions call this import with a non-NULL descriptor and whether the reported dimensions/format match the expected PSP frame. If no useful calls are observed, stop pursuing this hook rather than forcing a patch.

No filter, memory write, descriptor mutation, kernel patch, or console configuration change is authorized by this research stage.


## Follow-up audit: exact global-plugin startup pattern

The current NoPspEmuDrm source was inspected directly:

- `user/Main.c` calls `taiGetModuleInfo("ScePspemu", &tai_info)` during user-plugin startup. If found, it dispatches to `pspemu_module_start(tai_info)`; if not, it returns `SCE_KERNEL_START_NO_RESIDENT`. This is the exact low-residency pattern suitable for a PSPemu-only diagnostic plugin loaded from `*ALL`.
- `user/PspEmu.c` calls `sceKernelGetModuleInfo`, obtains the first segment's runtime virtual address, and installs import hooks with `taiHookFunctionImport` scoped to module name `ScePspemu`.
- Its `pspemu_module_stop()` releases its import hooks. A future diagnostic plugin must likewise release every successfully installed hook and must not assume a hook ID is valid when installation failed.

Sources:
- https://github.com/LiEnby/NoPspEmuDrm/blob/main/user/Main.c
- https://github.com/LiEnby/NoPspEmuDrm/blob/main/user/PspEmu.c

This confirms that the global plugin can immediately become non-resident in ordinary applications and that hook installation can be restricted to the PSPemu module. It does not remove the need to test the plugin against other installed global plugins.

## Build environment status

The current development container does not have the VitaSDK cross-compiler (`arm-vita-eabi-gcc`) or `vita-make-fself` installed. Therefore no native `.suprx`/VPK has been compiled or validated in this environment. Source code must not be described as a tested binary until a VitaSDK build completes and the output is inspected.

## Display descriptor clarification

The SceDisplayFrameBuf `pitch` field is documented in the Vita SDK as a pixel pitch, not a byte count. Its structure is 0x18 bytes. Do not calculate a buffer length from width × height alone; row pitch and pixel format must both be known. The SDK's 0x1C-byte extended descriptor has a bilinear flag, but that is a display-driver descriptor feature and not proof of a custom programmable filter in PSPemu.


## Superseded argument-map note (incorrect; see correction below)

A deeper read of the firmware's own user-mode display wrapper corrected the interpretation of the Adrenaline hook's final argument.

The firmware 3.60 implementation of sceDisplaySetFrameBufForCompat has the six-argument wrapper form:

- arg1: display head
- arg2: framebuffer index
- arg3: crop-width argument; do not dereference it
- arg5: optional source framebuffer pointer (`pParam`)
- arg6: optional conversion/capture destination (`convDst`)
- arg4: sync mode
- arg5: optional sized parameter
- arg6: optional sized parameter

The options block is 0x18 bytes and contains scale_y, frame_buf, capture_frame_buf, frame_buf_size, capture_frame_buf_size, and reserved. The actual source descriptor is nested in options.frame_buf and has type SceDisplayFrameBufForCompat (0x1C bytes), with base, pixel pitch, pixel format, dimensions, and flags. The optional capture destination is separate.

The firmware wrapper packages the fifth and sixth arguments and their size fields into the syscall argument block. Its source comments explicitly state that the export is available only to a PSP emulator process.

Primary references:
- Firmware wrapper: https://github.com/shoui520/psp2/blob/0261b5803d8108135d6cd8dc8660834d865b4e24/sys/internal/usermode/src/driver_us/display.c
- Firmware export definition and PSPemu-only note: https://github.com/shoui520/psp2/blob/0261b5803d8108135d6cd8dc8660834d865b4e24/sys/internal/usermode/src/driver_us/exports.yml
- VitaSDK option and descriptor layouts: https://github.com/vitasdk/vita-headers/blob/e66ebe90b73d1fa4cce005a5b2072cec27322544/include/psp2/display.h

### Updated technical assessment

This is now the strongest lead in the project: the PSPemu-only compatibility display API has an explicit source-framebuffer descriptor. A read-only hook on this import may let us observe the actual buffer submitted by PSPemu without guessing PSP VRAM addresses. It still does not prove that the pointed memory is CPU-readable at the hook point, that PSPemu is finished writing it, or that a second CPU pass can safely modify it before scanout. Those are separate cache, ownership, synchronization, and performance questions.

The diagnostic target is therefore the source descriptor in arg5, not Adrenaline's sixth argument. No pixel reads/writes, flag changes, or framebuffer substitution should occur in the first prototype.


## Critical correction: wrapper argument mapping

A source-level audit of the firmware wrapper and kernel syscall corrected the earlier assumption that the third hook argument points to an options block.

The hooked SceDisplayUser import at library NID 0x4FAACD11 / function NID 0x8C36B628 is the six-argument user wrapper:
- arg1: head
- arg2: index
- arg3: compatibility crop-width parameter (do not dereference it as an options structure)
- arg4: sync
- arg5: optional source framebuffer pointer (the wrapper passes this as the kernel syscall's pParam)
- arg6: optional conversion/capture destination pointer (the wrapper passes this as convDst)

The wrapper reads the size field from arguments 5 and 6 and forwards both pointers and sizes through its syscall argument block. The kernel implementation then copies arg5 into a local source framebuffer descriptor and arg6 into a separate conversion destination descriptor. The source descriptor is therefore the fifth argument, not the third or sixth.

Primary source:
- User wrapper: https://github.com/shoui520/psp2/blob/0261b5803d8108135d6cd8dc8660834d865b4e24/sys/internal/usermode/src/driver_us/display.c
- Kernel syscall copy/argument mapping: https://github.com/shoui520/psp2/blob/0261b5803d8108135d6cd8dc8660834d865b4e24/sys/internal/driver/src/display/display_syscall.c
- VitaSDK compat descriptor: https://github.com/vitasdk/vita-headers/blob/e66ebe90b73d1fa4cce005a5b2072cec27322544/include/psp2/display.h

Adrenaline's six-argument hook is consistent with the wrapper ABI. Its final argument is the optional conversion/capture destination; replacing that argument when NULL is a POPS-specific path. This does not mean the sixth argument is the PSP source framebuffer.

The diagnostic prototype has been corrected to observe arg5 as the source descriptor and arg6 only as the optional conversion destination. It does not dereference arg3. It remains uncompiled and untested.
