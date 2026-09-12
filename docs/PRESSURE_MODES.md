# Pressure modes and shared filter source

Implemented on `graphite-ui`, following the earlier
[design proposal](PEN_UX_AND_LIBRARY_PROPOSAL.md). This is not yet a change to the
three existing upstream pen PRs, nor a replacement of the creator's default path.

## One response selector, not a preservation checkbox

Brush now offers:

| Mode | Width behavior | Position path | Original factor |
| --- | --- | --- | --- |
| Original smoothing (default) | Upstream width propagation | Original midpoint/Catmull-Rom | Available in Advanced; saved value retained |
| Preserve samples | Each contact sample keeps its mapped width | Sample-based; local correction optional | Not used; hidden |
| Uniform peak width | Entire stroke uses its greatest mapped width so far | Sample-based; local correction optional | Not used; hidden |

For the requested "stronger pressure widens the whole line" behavior, select
**Uniform peak width**. No slider value of 1 is necessary. This mode has an
explicit whole-stroke width policy; it does not claim to reproduce every spatial
detail of the original brush at factor 1. Choose Original for that old path.

Preserve is independent of the original factor. Original at 0.707 can still show
width variation; 1 propagates larger widths farther backward. The slider is now
called **Width propagation**, only shown for Original under Brush Advanced.
Its stored global value still also affects the original eraser generator; the
eraser has not been switched to either new mode or a revising position filter.

Pressure affects size and minimum width remain explicit shared settings. With
pressure size off, the selected width is constant. Round Caps only controls end
shape. Duplicate brush-specific sliders/filter controls were removed from the
Tablet/general settings screens, which point to Brush instead.

Local correction is a separate Brush section. It can run with Preserve or Peak,
but never silently overrides Original. The dependency is on the sample-based
path, no longer specifically on preserving widths. The existing stored filter
choice is retained. Mode and filter choices are captured when contact starts;
changing mode does not reinterpret an existing or in-progress stroke.

Configuration now stores `toolConfig.brush.pressureResponse` as `original`,
`preserve`, or `peak`. A missing new key migrates the former
`preservePenPressure` boolean. Missing/unknown modes safely default to Original.
The compatibility boolean is still written for older fork builds; those builds
cannot represent Peak and fall back to Original. No user configuration file is
edited outside the app, and no registry settings are involved.

Peak changes rendered widths, not stored source samples or the positional
filter's frozen prefix. Stronger stationary contact samples count. A new peak
can rebuild every width; it is not a constant-cost operation on very long
strokes. Stroke end does not append a pressureless hover sample.

## Source-level library

The core now lives in the public
[pen-stabilizer repository](https://github.com/alexiokay/pen-stabilizer), initially
package 0.1.0 / algorithm revision 1. Both InfiniPaint and PenTraceLab pin the
same source revision via `deps/pen-stabilizer`. `src/PenStabilizer.hpp` is only
an app adapter retaining existing setting clamps, not another algorithm copy.

After pulling this branch, run on the build PC:

```sh
git submodule update --init --recursive
```

Then use the existing upstream ARM64 build scripts. The C++17 header and CMake
INTERFACE target compile into the host: no new DLL, Rust binary or runtime
service. Library versioning, lifecycle and limits are documented in its repo.
Existing MIT attribution remains shipped with the app.

The library owns position math; this app owns width policies, SDL input and mesh
rendering. No universal Windows protocol, raw electrical measurement or Rust
rewrite is introduced. Other languages can gain optional bindings later.

## Validation

CI runs actual C++ JSON migration/roundtrip and peak/preserve width-policy tests,
including peaks after a frozen position prefix and filter on/off combinations.
The existing pinned independent-oracle filter tests and Windows SDL checks remain.
Source checks cover UI wiring, Original fallback and eraser opt-out.

These checks are not full InfiniPaint compilation, rendered-mesh comparisons,
or a Surface Pro 11 / Metapen M2 test. On the build PC check all three modes with
new light-heavy-light strokes, stationary pressure, caps, minimum size, pressure
disabled, filter off/on, save/restart, undo, export, tiny curves and long strokes.
The Original path's midpoint/width/tip routines are intentionally retained.
Do not infer universal wobble removal from synthetic regression tests.
