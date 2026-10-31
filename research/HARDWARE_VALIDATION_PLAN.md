# Hardware validation plan — PSP-Sharp-Vita

Status: **NOT APPROVED FOR INSTALLATION**. This plan describes the next evidence-gathering stage; it does not authorize changes to a user's live taiHEN configuration.

## Purpose

Determine whether stock PSPemu invokes the hooked compatibility-framebuffer import during a real PSP game session, and whether the optional source descriptor contains stable, plausible metadata. This stage does not read pixel memory and does not apply a filter.

## Preconditions

- A separately backed-up, known-good Vita setup and a verified recovery route.
- Confirmed firmware/module identity for the exact console being tested. The current probe intentionally refuses module NIDs other than the 3.60 retail target `0x2714F07D`.
- A disposable PSP game bubble or test title; do not begin with a frequently used game or a title that stores progress during exit.
- A reviewed build from the current source commit, not an older artifact.
- A way to remove the temporary plugin entry and reboot without relying on the plugin itself.
- No concurrent plugin/configuration changes during the test.

## Test stages

### Stage 0 — no-console static review

- Confirm the artifact came from the reviewed commit and record its SHA-256.
- Confirm it is a Vita SELF/SUPRX output and contains no unexpected files.
- Review imports, hook target, start/stop behavior, and file output.
- Reject the build if it reads the framebuffer base, changes arguments, calls display APIs recursively, or writes from the frame callback.

### Stage 1 — load-path validation

- Determine the actual process/title ID used while a Chovy-Sign PSP bubble is running. Do not assume that the visible bubble title ID and PSPemu process ID are interchangeable.
- Prefer a title-scoped plugin entry if the process identity is confirmed. Do not add the probe to `*ALL` merely for convenience.
- Keep the test entry separate from all existing plugin lines. Do not reorder, remove, or replace existing plugins.
- If process identity cannot be confirmed, stop; do not trial-and-error taiHEN sections.


### Process scope note

Chovy-Sign's PSP builder uses the UMD disc ID as its PSP output folder (`LibChovy/PspParameters.cs` returns `Umd.DiscId` as `OutputFolder`). Therefore do **not** assume every Chovy-Sign bubble is the generic `NPXS10028` title. The game bubble's own title/disc ID must be identified for a title-scoped taiHEN entry. The module name `ScePspemu` and the app title ID are separate identifiers; confirm the actual process context before selecting a taiHEN section. If this cannot be established, stop rather than falling back to `*ALL`.

### Stage 2 — one read-only run

- Start one disposable PSP game.
- Play normally for roughly one minute, then exit normally to LiveArea.
- Do not suspend/resume, change resolution, overclock, or run other diagnostic plugins during this first pass.
- Retrieve `ux0:data/PSPSharp_compat_probe.txt` only after the PSPemu process has exited and the module unload report has been written.
- If the report is absent, do not infer that the import was not called; first determine whether module_stop ran and whether the report path was writable.

### Stage 3 — interpret metadata

A useful first result requires:
- one or more successful calls with a non-NULL fifth argument;
- a recognized descriptor size (0x18 or 0x1C);
- plausible, repeatable width/height/pitch/format values across calls;
- a nonzero base address.

These fields are only observations. They do **not** prove the pixel address is mapped for CPU access, that pitch units are bytes or pixels, that the buffer is owned by PSPemu, or that the buffer is safe to modify.

Repeat with a second title only after the first report is understood. Compare metadata; do not write to either buffer.

## Stop conditions

Immediately remove the temporary test entry and return to the known-good configuration if:
- PSPemu fails to launch or crashes;
- a PSP game hangs on exit;
- LiveArea or other apps behave abnormally;
- SD2Vita is not detected normally;
- the report shows impossible values or the plugin cannot be unloaded cleanly.

Do not run a pixel-processing experiment based solely on a nonzero base pointer. A later phase would need separate evidence for memory mapping, cache maintenance, display/GPU synchronization, buffer lifetime, pixel format, pitch units, and frame timing.

## Rollback

1. Exit the PSP game and power off normally.
2. Remove only the temporary probe entry from the exact taiHEN section used for the test.
3. Delete the probe file from `ur0:tai/` only if it was copied there for this test; do not touch other plugins.
4. Reboot and confirm the existing SD2Vita and normal applications work.
5. Keep the report separately for analysis, then delete it from `ux0:data` if desired.

**No step in this document requires editing the user's existing configuration now.** The test should only be prepared after the user explicitly approves a concrete, reversible installation procedure.