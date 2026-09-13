# Pen pressure UX and reusable stabilization core

Current pressure/geometry controls and migration are described in [BRUSH_PIPELINE.md](BRUSH_PIPELINE.md); older pressure-engine descriptions below are historical.

Status: **historical design proposal**. Recorded 2026-09-13.
The subsequent [pressure modes and library integration](PRESSURE_MODES.md)
implement the selector, explicit peak policy and source extraction on `graphite-ui`.
Read that status document for current behavior; the original proposal below
records the starting point and includes longer-term ideas not yet implemented.
This document does not change defaults, create a package, or propose an immediate
Rust rewrite. Implementation notes are in [PEN_INPUT.md](PEN_INPUT.md) and
[GRAPHITE_UI.md](GRAPHITE_UI.md). Graphite UI work is on the separate `graphite-ui`
branch; the existing upstream pen PRs do not include that UI redesign.

## What exists today

- InfiniPaint has upstream pressure-width smoothing plus an opt-in direct pen
  path that preserves the width associated with each contact sample.
- The preservation option defaults off. Original smoothing remains the default.
- The direct path optionally uses the streaming local-normal correction in
  [PenStabilizer.hpp](../src/PenStabilizer.hpp). It is adapted from PenTraceLab
  0.4.0, commit `ef6555a6defd12b8dde5afc408df4975eb4492b2`.
- [PenTraceLab](https://github.com/PenTraceTools/pen-trace-lab) already has an internal
  `pentrace_core` CMake library target, including analysis and trace I/O. That is
  not yet an independently versioned stabilization package shared by both apps.
- InfiniPaint currently contains its own streaming port. Tests compare live
  prefixes against the pinned diagnostic implementation; this is not yet one
  shared production implementation.

These are separate concerns: Windows report acquisition, position correction,
pressure-to-width policy, mesh generation, and UI. A filter library would not
replace all five or automatically integrate with every drawing application.

## Why the pressure controls are confusing

The user reports that both checkbox states can look similar, and that the
difference becomes obvious with the pressure smoothing factor at `1.0`.
**Preservation does not require a factor of 1.0.** The factor controls the
original path and is bypassed by the preserving path.

In [BrushComponentCode.cpp](../src/CanvasComponents/BrushComponentCode.cpp),
`smooth_out_points` propagates larger widths backward with attenuation. For a
factor `f`, a preceding width can be raised to `next_width * f`, repeatedly.
The newest width is also bounded below by `previous_width * f`.

An illustrative width-only sequence, not a full rendering test:

| Input widths | Original, f = 0.707 | Original, f = 1 | Preserve samples |
| --- | --- | --- | --- |
| 1, 1, 1, 10 | 3.534, 4.998, 7.070, 10 | 10, 10, 10, 10 | 1, 1, 1, 10 |

At `1`, maximum width propagates through this sequence. At the upstream default
`0.707`, width variation can remain visible even with preservation off. Thus
"restore upstream behavior" is not the same as "always use one peak width".
Preserving widths also does not mean preserving raw electrical pressure: the
app still maps Windows pressure through its size/minimum-width settings.

Source inspection explains this distinction but does not establish that the
checkbox works in a particular running executable. A controlled new stroke and
known build are still needed to check a suspected runtime or configuration bug.

There is another real UX issue: the current checkbox changes **both** pressure
policy and the geometry pipeline. Off uses the upstream midpoint/Catmull-Rom
path; on uses the direct sample path. Local correction currently requires the
latter. A pressure-only label hides this dependency.

## Proposed Brush inspector

Keep size and round caps readily available. Put pressure choices in the Brush
inspector, with one authoritative control rather than contradictory controls
in Brush and Tablet settings.

### Pressure

Use a named **Pressure response** selector:

- **Original smoothing (default)**: preserve upstream behavior. Show the
  existing numeric factor under Advanced, named "Width propagation", with a
  short explanation: "Higher values spread stronger pressure to nearby earlier
  points; 1 spreads the peak width through the stroke."
- **Preserve samples**: retain each sample's mapped width. Hide or visibly
  disable the original factor: "Original width propagation is not used."

Keep **Pressure affects size** explicit. If disabled, explain that the brush
uses a constant selected width; do not imply preservation can then create width
variation. Keep minimum width with pressure controls. Round caps affects stroke
end shape, not pressure preservation or wobble correction.

An optional future **Uniform width from peak pressure** mode would make that
specific intent discoverable. Do not add it merely as another ambiguous slider
preset: define and test whole-stroke peak-width behavior, including stationary
pressure updates and final tips, before presenting it as a guarantee. It is not
the proposed default.

Do not silently change existing factor values or enable preservation during a
configuration migration. Capture choices at contact-down; changes affect the
next stroke, not previously saved drawings or a stroke in progress.

### Path correction

Present **Wobble correction: Off / Local correction (experimental)** separately
from pressure response. Keep the upstream-facing default off. While the current
dependency exists, show a clear disabled-state reason under Original smoothing:
"Requires the sample-based pen path. Choose Preserve samples to use it."
Do not silently switch pressure policy when the user enables correction.

Longer term, separate pressure policy from positional processing in the engine,
so users can choose each independently. This requires actual integration work:
sample/attribute mapping, mesh generation, and compatibility tests. Relabeling
the checkbox alone does not achieve that separation. Preserve a clearly named
Classic path for exact upstream compatibility if necessary.

Expose radius, maximum displacement and revision window only under Advanced;
name their units and explain the detail-versus-correction tradeoff. A larger
window is not a free improvement, particularly for intentional small curves.

A small light-heavy-light brush preview could demonstrate settings using the
actual brush engine. Label it a **settings preview**, not a hardware test.
Actual pen recordings remain essential for diagnosing diagonal wobble.

## What the current correction algorithm does

It is a streaming, bounded-revision **local-normal position filter**, not One
Euro, prediction, a global straight-line fit, or a pen-up beautification pass.

For each revisable point it computes an arc-length-weighted local mean over a
symmetric neighborhood, estimates the local tangent from neighborhood endpoints,
and applies only the mean offset perpendicular to that tangent. Corner and
endpoint/time tapers reduce correction, and a displacement cap limits movement.
The integration preset uses 12 DIP radius, a 120 ms revision window and 4 DIP cap.

The current tip remains at the newest contact report. Recent geometry may revise
as future samples arrive; older geometry freezes. Pressure-associated widths
are not filtered. Discontinuities split filter runs. This is real-time drawing
with a revisable tail, **not zero-delay recovery of the physical trajectory**.

Slow hardware wobble can overlap the spatial scale of intentional detail.
Position samples alone do not identify which was intended. No claim of perfect
correction without tradeoffs is justified by our recordings. A magnetic overlay
or independent motion tracker would be a separate hardware experiment, not
something this software library can measure from Windows reports.

The present implementation retains whole-stroke vectors. A bounded revision
time does not imply bounded total memory or a fixed number of samples to process.
InfiniPaint also rebuilds its stroke mesh. Long/high-rate stroke benchmarks are
required before promising predictable production performance.

## Proposed reusable module: source first

Extract the portable correction core into a small, independently versioned
repository once its contract and conformance tests are ready. Initial language:
**C++**, matching the existing code. Consumers build it into their application
from source, using a pinned tag/commit and a small CMake target or vendored files.
No required DLL, separate executable, background process, network service,
registry setting or new pen driver. No mandatory runtime downloads.

Suggested ownership:

| Component | Owns | Does not own |
| --- | --- | --- |
| Portable core | Position correction, immutable sample mapping, revision boundaries, numerical validation | Windows input, UI, rendering, document format |
| PenTraceLab | Capture, replay, comparisons, diagnostics and evaluation datasets | A separate drifting production filter |
| InfiniPaint adapter | SDL reports, coordinate/time conversion, width policy, brush mesh updates | A copied algorithm implementation |
| Other app adapters | Their input and rendering integration | Changes to the shared algorithm hidden in local copies |

Both applications should eventually consume the **same production core**.
Retain an independent, pinned reference implementation in tests as an oracle;
comparing the shared code against itself would not verify an extraction.
Start with stabilization only, not all of `pentrace_core`: trace files and the
comparison UI should not become dependencies of every drawing app.

Call this a **library/module with an API contract**, not a universal pen
protocol or a new application framework. A small common replay-fixture format
can support tests without becoming a required document or input format.

### Contract to settle before a stable release

- Coordinate units and transform ownership: explicit display-DIP normalization
  in the host; no silent conversion from arbitrary document coordinates.
- Monotonic report time, discontinuities, duplicate/stationary samples, and
  invalid/nonfinite input behavior. Do not synthesize timing from frame rate.
- Begin, append/batch, finish and cancel semantics; no hover point appended at
  lift and no surprise finish-time reshaping.
- Stable source-sample indexing. The core changes positions, not pressure,
  tilt, timestamps or host-owned width attributes.
- Output includes the earliest changed index and an explicit committed-prefix
  boundary. Specify which points may revise and when references become invalid.
- Exact-tip and displacement-cap guarantees, ownership, allocation/error
  behavior, and documented cost/memory limits.
- No global mutable stroke state; independent instances for simultaneous
  contacts, with routing and device identity handled by the host.

These are proposed public requirements, not claims that the current header
already exposes every API above. Hosts that only append irreversible marks must
support a temporary/revisable tail or choose a different latency policy. In
particular, retroactively moving an already destructive eraser path is not a
safe drop-in use of this filter.

### Versioning, distribution and provenance

Use immutable release tags and pinned dependency revisions. Track **API/package
version** separately from an **algorithm behavior revision**: compatible function
signatures can still produce different strokes. Record both, parameters and
coordinate units in PenTraceLab comparison exports. Never update a consumer's
algorithm implicitly when it opens an existing drawing.

Publish minimal native integration and replay examples, changelogs, CI across
supported compilers/architectures, and conformance fixtures with numerical
tolerances. Share only recordings explicitly approved for publication; keep
private drawings and device logs out of package tests.

Carry the existing MIT attribution for the PenTraceLab-derived core. Audit every
extracted file's provenance; do not accidentally include InfiniPaint-specific
application code under a newly asserted license. Keep application adapters in
their respective projects unless separately reviewed for extraction.

## Rust and other languages: later, optional

No rewrite is necessary for the first extraction. Native C++ apps can consume
the source directly. Other languages need an appropriate binding; source
availability alone does not make C++ directly callable everywhere.

An optional narrow C ABI could later expose opaque handles, plain sample arrays
and explicit buffer ownership without C++ STL types or exceptions crossing the
boundary. It need not require a shared DLL; distribution/linkage can remain a
host choice. A Rust wrapper or future Rust core is a separate decision, justified
by concrete maintenance/safety needs and the same conformance tests. Do not
maintain two independent production filters or rewrite the whole app just to
package this algorithm. A Rust binary is not the proposed integration mechanism.

## Implementation and validation sequence

1. Reproduce the pressure UX report on a known build. Draw new light-heavy-light
   strokes with Original at 0.707, Original at 1, and Preserve at both factors.
   Preserve should be independent of that factor. Check saved settings/restart,
   pressure-disabled behavior, minimum width and stationary pressure updates.
2. Implement the named response UI in a focused change, preserving defaults and
   saved choices. Verify desktop/phone and remove contradictory active controls.
3. Specify and test pressure/path independence before enabling new combinations.
   Check mouse/touch/eraser behavior, stroke-end handling, undo, save/reopen,
   transparent strokes and exports. Do not infer rendering fidelity from only
   a width-array unit test.
4. Extract the core with no intended algorithm change. Compare every live prefix
   to the pinned oracle, covering corners, circles, slow diagonals, gaps,
   rotations, endpoints, frozen prefixes, caps and immutable attributes.
5. Migrate PenTraceLab and InfiniPaint to pinned releases of the same core. Test
   application rendering and benchmark long strokes at high report rates.
6. Tune algorithms only in subsequent behavior-versioned changes, evaluated on
   real recordings and intentional-detail controls. Publish bindings only when
   a real second-language consumer establishes their requirements.

## How to approach upstream

Keep existing input, pressure and optional-correction PRs focused. The Graphite
theme and this broader library roadmap should not be added to those PRs as an
unrelated dependency.

First offer a short issue or Discussion describing the pressure UX problem,
default-preserving proposal and sample comparison. Ask whether the maintainer
wants a checked-in design note. A focused documentation PR is useful if they do;
it should clearly distinguish accepted behavior from future ideas. Keep the
independent library's broader roadmap in its own repository once one exists.

This document does **not** announce a new upstream documentation PR, package
release, validated executable, or approved redesign.
