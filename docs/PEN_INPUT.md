# Pen brush integration: local correction v1

This fork now uses the PenTraceLab 0.4.0 local-normal filter, ported from
`ef6555a6defd12b8dde5afc408df4975eb4492b2`. The old One Euro presets,
prediction overlay, and pen-up raw-point append have been removed.

## What to build

Use the current `windows-arm64-build` branch (or updated `main` after integration
is published), not an old release/tag. Existing executables do not change.
The creator's ARM64/Vulkan build scripts remain unchanged.

On your build PC, use a fresh build folder or re-export dependencies first.
From the repository root in the appropriate Visual Studio build environment:

```bat
git submodule update --init --recursive
set "PEN_BUILD_REPO=%CD%"
call "%PEN_BUILD_REPO%\windowsinstall\conan_init_arm64.bat"
call "%PEN_BUILD_REPO%\windowsinstall\build_portable_arm64.bat"
call "%PEN_BUILD_REPO%\windowsinstall\package_portable_arm64.bat"
```

The upstream portable packager uses `windowsinstall/dlls-arm64` for runtime DLLs;
supply the DLLs required by your build there. It recreates its
`portable_package` staging folder and portable ZIP. Do not keep unrelated files
in that staging folder. The executable is built under
`build-arm64/build/Release`; the portable ZIP is under `windowsinstall`.
You can instead run upstream's full build/package script if also making an installer.
The absolute paths above matter: upstream's scripts change the working directory.
Check that each step succeeds before continuing to the next.

IMPORTANT: `conan_init_arm64.bat` exports the updated SDL recipe. Rebuilding just
InfiniPaint against a cached OLD SDL library does not install the input fix.
Ensure Conan selects/builds the new recipe revision. The input patch is applied
to pinned SDL 3.4.16 and fails if expected source entrypoints change.

## Settings and behavior

Tablet settings in both interfaces now show **Pen brush: local wobble correction
(experimental)**, enabled by default. Test 6 defaults are radius 12 DIP, revision
window 0.120 seconds, cap 4 DIP. Settings are captured at contact-down.

- The live endpoint stays at the last in-contact report, without prediction.
  Recent points can revise as future samples arrive; this is not zero-latency
  recovery of the true physical trajectory.
- The filter integrates over symmetric arc-length neighborhoods, applies only
  the local-normal component, tapers at corners/endpoints and limits movement.
  Larger windows/radii may suppress more wobble but can soften intentional detail.
- A streaming frozen prefix avoids re-filtering the entire stroke on every
  report. Rebuilding the mesh still uses upstream's whole-stroke rendering path;
  very long strokes need performance testing.
- Off bypasses positional filtering AND the old midpoint/Catmull-Rom path for pen
  brushes. It is a direct reported-point centerline, not the legacy Off behavior.
- Pen pressure is paired with each contact sample. No additional pressure
  smoothing or last-two-point tip-width rewriting is applied to this pen path.
  Mouse/touch drawing and erasing retain upstream stroke-generation behavior.
- The eraser deliberately does not use a revising filter: moving an erase path
  after it has already erased content is not a safe drop-in operation.
- Moving/rescaling the window or changing camera transforms ends an active pen
  brush stroke; lift and start again. Old points are never reinterpreted through
  a new transform. Gaps over 50 ms and non-increasing times start new filter runs.
- Existing documents are not re-filtered. Undo, saves, exports, screenshots and
  network updates use the same derived mesh; no separate prediction layer exists.
  Source positions/times/widths are immutable while generating a stroke, but the
  existing mesh file format does NOT persist a recoverable raw recording.
  Continue using PenTraceLab for recordings and algorithm comparisons.

Old positive strength values do not translate to a new strength. Defaults apply.
An old explicit Off setting is respected. New settings use versioned
`tablet.penLocalFilter` configuration keys.

## Windows input

History is read while the native pointer message is current, with bounded
buffer-growth retries and latest-only fallback if retrieval fails. Reports are
processed oldest-first. A bounded recent-report cache removes identical reports,
not every report sharing a timestamp. Different pointer/device owners terminate
the preceding SDL contact.

Fractional HIMETRIC-to-client mapping respects device/display origins. Application
filter units use display scale, not merely pixel density. QPC uses a stable
calibration; slight report-clock offsets do not cause per-sample clock switching.
An unusable report clock disables filtering across that sample boundary.

Pressure precedes its contact motion, including stationary contact reports.
Contact ends before hover/lift movement. Cancellation/proximity loss/focus loss
ends drawing without adding a hover endpoint. SDL still exposes one logical
Windows pen, not independently identified physical pen tools.

Upstream's **Disable touch for drawing** blocks finger drawing; pen AND mouse
drawing remain enabled. This is independent of wobble correction.

## Verification and remaining acceptance

`Pen integration checks` on GitHub builds and tests the dependency-free filter
against every prefix of the original PenTraceLab implementation (pinned commit),
including straight/noisy paths, curves, corners, stationary samples and timing
discontinuities at 60/120/240/672 Hz. Invariants cover immutable widths, Off,
rotation symmetry, displacement caps, fixed endpoints and frozen prefixes.

The Windows jobs compile the patched SDL source for x64 and ARM64; x64 also runs
history-identity tests. These are NOT a full InfiniPaint executable build, GPU
rendering validation, or physical pen test. No build or prerequisite installation
is performed on the development PC.

Before replacing your everyday executable, test slow diagonals, small letters,
corners, dots/pressure changes, lifts, touch rejection, cancellation, zoom and DPI,
transparent strokes, long strokes, undo, save/reopen, export and collaboration.
The Test 6 results justify an experimental integration, not a promise of perfect
wobble removal or unaltered intent. Preserve your previous portable executable.
