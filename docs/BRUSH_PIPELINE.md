# Explicit brush pipeline (experimental)

This supersedes pressure-engine coupling described in earlier notes. The source
library remains pen-stabilizer v0.1.0, revision 1; these are host-side changes.

## Controls

- **Original compatibility (default)** retains original point spacing, midpoint
  adjustment, Catmull-Rom interpolation, width propagation and tip handling.
  Sample-pipeline controls are inactive, not silently applied here. A saved On
  correction toggle is explicitly labeled inactive while compatibility is selected.
- **Sample pipeline** exposes independently selected pressure policy, position
  correction and render interpolation. Changing pressure does not switch engines,
  correction, or rendering. Options are captured at contact-down.
- Pressure choices: Preserve samples, Time-based width smoothing, Uniform peak width,
  and explicitly labeled Legacy per-report propagation for existing settings.
- Rendering under Advanced: Sample polyline (default) or Bounded curves (experimental).
- Pressure affects size / minimum width remain shared brush/eraser settings.
  Eraser/mouse/touch retain their original generation; sample policies are pen-only.

Correction On cannot be called exact original compatibility. Those are different
pipelines, and switching between them is now explicit instead of a pressure side effect.

## Time-domain width propagation

The new width policy uses exp(-deltaSeconds / tau), with tau controlled by
Width decay (ms), initially 40 ms for fresh sample settings; 0 means no propagation.
It is a maximum envelope, not a position filter or an averaging/prediction algorithm.
A later high width can propagate backward with exponential decay; earlier high widths
can carry forward. Preserve samples remains available when no backward edits are wanted.

For the same sampled peak and elapsed time, decay no longer depends on report count.
It does not reconstruct an unsampled pressure peak or guarantee identical results from
different devices. Equal/backward clocks, invalid continuity and >50 ms gaps stop
propagation across the boundary. No receipt-time cadence is fabricated in replay.

The older per-report factor (default 0.707) remains labeled Legacy rather than silently
converted to a time scale. There is no exact universal conversion between them.
Whole-stroke Peak deliberately ignores decay. Widths may revise beyond the position
filter's frozen prefix; position and pressure policies have different contracts.

## Render geometry and guarantees

Polyline connects measured/corrected sample knots, collapsing coincident positions
with the widest coincident width for valid outline normals. Bounded curves retain
every non-coincident knot, including the tip, and linearly interpolate width between
them. Cubic controls are confined to a capsule around each chord, so centerline
deviation is at most **0.25 DIP** from the polyline (often less).

This bound is additional to position correction. It is not an exact physical-intent
bound, not the original Catmull-Rom pipeline, and does not bound the final brush outline.
Neighbor-dependent curves can revise adjacent spans as a new point arrives; the
position filter's frozen-knot guarantee is not a promise that all rendered pixels freeze.
Curves currently use eight subdivisions per span and can increase mesh cost. Keep
Polyline for the closest representation of corrected samples.

## Migration

New files explicitly save engine, rendering and pressureTimeMs. Old configurations
infer their formerly effective engine once after both brush/filter configs load:
Original + effectively Off stays Compatibility; Preserve/Peak or an already-effective
correction selects Samples. The previous correctionIndependent migration still handles
older ignored filter toggles once. Existing Original sample pressure stays Legacy
per-report propagation; no silent new smoothing strength. Old rendering stays Polyline.
Unknown engine/rendering values safely fall back to Compatibility/Polyline.

Source-level regression checks compare original geometry functions and the unchanged
compatibility motion body against upstream commit ac6c2b0ea8af35483cf559aab5d9657c552d4717.
This is not full input-to-pixel equivalence; native history delivery was changed in #96.

## Replay comparisons on the build PC

The portable pen_replay tool is uploaded by the fork's Pen integration checks workflow
for x64 and ARM64. It uses the pinned PenTraceLab 0.4.1 parser/clock calibration/contact
processor and the actual InfiniPaint width/curve headers. Diagnostic dependencies do
not become dependencies of the app or position library.

Run:
```powershell
.\pen_replay.exe recording.pentrace new-comparison-folder 0
```
The final number is the zero-based stroke index, including other input kinds in the
recording. Select a pen stroke with at most 20,000 samples; the output folder must not
already exist. The output reports the number of strokes available.

- comparison.svg: 16 combinations (correction Off/On, four pressure policies, two renderers).
- vertices.csv: actual result centerline coordinates and widths.
- README.txt: parameters, selected sample count and unavailable-pressure/timing warnings.

Orange dashed means measured centerline, purple means the diagnostic result. Default
comparison brush is 15 DIP / minimum 0; unknown pressure uses a reported fixed-width
fallback. SVG uses segment previews, not Skia outlines, transparency, antialiasing or
document simplification. It does **not** emulate original compatibility point spacing.
Do not describe this as an exact screenshot comparison of the two full app engines.
No raw recording is modified or uploaded. Synthetic self-tests validate parser roundtrip,
all 16 outputs, SVG validity, endpoint retention and refusal to overwrite existing data.

For source builds:
```sh
cmake -S tests -B build-tests -DPENTRACE_REPLAY_SOURCE=/path/to/pen-trace-lab
cmake --build build-tests --config Release --target pen_replay
```
Initialize the pinned pen-stabilizer submodule in both repos first.

## Acceptance still required

CI covers matrix independence, migration, 60/120/240/672 Hz analytical width decay,
clock boundaries, curve knot/capsule/width bounds, original geometry source guards,
the existing independent position oracle, sanitizers, replay exports, and patched SDL
x64/ARM64 compilation. It does not compile/run the complete InfiniPaint UI/Skia app.

On Surface Pro 11 + Metapen M2, compare light-heavy-light, slow diagonals, intentional
curves/corners, stationary pressure, dots, long strokes, undo/save/reopen, selection,
erasing and DPI/zoom interruption. Measure frame time and mesh cost. No assertion of
zero delay, perfect intent recovery or complete wobble removal follows from these tests.
