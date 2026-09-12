# Windows pen wobble: findings, diagnostic app, and implementation plan

Date: 2026-09-12

Integration update: the diagnostic stage is complete and the Test 6 local-normal
filter is now ported to the pen brush path. See `PEN_INPUT.md` for the actual
implementation, build instructions, test scope and remaining acceptance work.
The historical proposal below is not a claim that every proposed feature shipped
(in particular, saved raw-stroke recovery and calibrated hardware correction did not).

Original status: proposed design, not a validated final implementation. This document
supersedes conflicting quality claims or coordinate explanations in
`PEN_INPUT.md`. It does not mean the portable executable has been updated.
No build or prerequisite installation is authorized by this plan.

## 1. Objective and honest limits

Make InfiniPaint feel responsive and faithful when drawing with a Windows pen,
especially during slow diagonals, without silently straightening intentional
curves, rounding corners, losing small details, or adding tails.

There is no universal algorithm that guarantees both complete wobble removal and
perfect preservation of intended freehand movement. The same measured movement
can represent digitizer error or an intentional detail. Preserving the original
samples guarantees recoverability, not recovery of the true physical pen path.

Correct input handling is a correctness requirement. Smoothing is a user-selected
tradeoff. These must remain separate in the implementation and interface.

## 2. Findings established so far

- Windows exposes pen input through pointer APIs; there is no hidden pen-data file
  that the application must read. `GetPointerPenInfoHistory` exposes samples
  coalesced into the current message. Its results are newest-first and include
  the latest sample. Retrieve them before advancing to the next message. [1]
- Fractional coordinate conversion, timestamps, pressure pairing, event ordering,
  contact transitions and capture handling all require validation. Receiving more
  samples does not itself remove errors already present in those samples.
- Microsoft documents predicted and non-predicted `POINTER_INFO` coordinate fields
  as equal for pointer types other than touch. Switching between these fields is
  not an established pen calibration or wobble fix. The prototype's description
  of this switch as using specially calibrated coordinates was misleading. [2]
- Microsoft has a moving-jitter device test. This establishes that device-level
  moving jitter is a real test category, not that this user's device is necessarily
  the source of the observed problem. [3]
- One Euro filtering explicitly trades jitter reduction against lag. Its settings
  depend on coordinate units and movement characteristics. It is a candidate
  baseline, not a proven best solution for this device. [4]
- Procreate documents that stronger filtering can make strokes too straight and
  smooth. Its public description does not disclose enough to reproduce its
  proprietary filtering algorithm. [5]
- Apple recommends collecting coalesced samples and treating prediction as
  temporary, replaceable data. These are useful architecture principles, not a
  promise of equivalent hardware quality or Apple Pencil behavior. [6, 7]
- Windows Ink provides a managed inking/rendering path, including low-latency wet
  ink. Replacing this application's renderer with InkPresenter is a separate
  integration decision, not a demonstrated cure for wobble. [8]
- Upstream's Disable touch for drawing excludes finger drawing, while pen and
  mouse drawing remain possible. It is not a pen-coordinate stabilization fix.

## 3. Why build a diagnostic mini app first?

Yes: a small, local drawing diagnostic should precede further filter tuning. We
already know which APIs to use; the diagnostic validates how they behave on the
actual device and locates where error enters our pipeline.

It must distinguish three observable stages:

1. Windows-reported input samples, with no application positional filtering.
2. Points produced by a selected filter from exactly those recorded samples.
3. Geometry and pixels produced by the renderer from those points.

If a wave is present at stage 1, it predates our filtering and rendering. That
alone cannot distinguish hand motion, pen electronics, digitizer, firmware or
driver behavior. If it first appears at stage 2 or 3, the corresponding application
stage is implicated. Independent physical reference measurements are needed to
prove physical pen-path error.

### Scope and interface

Build a portable Windows diagnostic with no installer, no telemetry and no changes
to system pen settings. Building it will require a separately authorized build
environment; do not install tools on this system as part of this plan.

The first version should contain:

- A drawing canvas with switchable overlays: reported samples, direct polyline,
  filtered path and InfiniPaint-style rendered path. Distinct colors and a zoomed
  inspection view make differences visible.
- Unfiltered mode as the initial mode. Disable prediction for baseline capture.
- A test selector for diagonals in both directions, horizontal/vertical strokes,
  shallow arcs, circles, small loops, handwriting, sharp corners, dots and lifts.
- Slow, normal and fast trials, plus repeated trials rather than one example.
- A statistics panel and local Save recording / Load and replay controls.
- Explicit labels separating reported-input variation, filter displacement and
  renderer deviation. Never display a single unsupported "accuracy percentage."

For a repeatable straight-line reference, an optional suitable non-scratching,
non-conductive physical guide can reduce hand-path variation. An on-screen line
alone is not ground truth. Do not use a guide that risks the display or changes pen
behavior through interference. Record whether a guide was used.

### Acquisition and recording

Use one authoritative native `WM_POINTER` acquisition path for pen capture. Do not
also treat mouse-emulation or SDL pen events as independent copies of the same
stroke. Structure the recorder so the tested acquisition logic can later be
shared with, or replay-tested against, the SDL integration.

Capture per sample where available:

- Session/stroke ID, pointer ID, source-device identity and frame ID.
- Message type, pointer flags, button transitions, contact/cancel state.
- Both HIMETRIC and pixel coordinate fields, retained as reported.
- Pressure, tilt, rotation and their validity masks; missing is not zero.
- Original QPC / millisecond timestamps, local message-receipt time and sequence.
- History count, batch index, retrieval failures and fallback decisions.
- Converted floating-point client/display coordinates and mapping metadata.

Record session metadata: device and pen model entered by the user, relevant
software versions, display resolution/scaling/orientation, window location,
coordinate units, test instructions, settings and recorder version. Device handles
are session-local identifiers, not persistent hardware IDs.

Store original reports before deduplication so suspected duplicates and state
transitions can be audited. Store a separate normalized stream for replay. Log
invalid or missing timing; do not silently claim reconstructed times are hardware
measurement times. Use a versioned local recording format with explicit units.
Raw here means Windows-reported, not untouched electrical digitizer data.

### Useful measurements

For intended straight-line trials, report perpendicular RMS, a high percentile and
maximum deviation from an orthogonal best-fit line. Show results in display units;
use millimeters only when physical scaling is known or validated. Describe these
as straightness measurements, not proof of pen accuracy.

Also report:

- Sample interval distribution, batch sizes, timing gaps and ordering anomalies.
- Filter displacement relative to input, including maximum displacement.
- Renderer centerline deviation from the supplied point path.
- Endpoint displacement, corner rounding and small-loop/detail changes.
- Processing and event-to-render-submission timing under light and heavy load.

Temporal analysis and error versus distance traveled may help distinguish
speed-dependent filtering effects from spatially repeating errors. These are
diagnostic clues, not automatic hardware-fault classifications.

Do not label software submission timing "input-to-photon latency." Measuring
physical pen-to-visible-pixel delay requires suitable external measurement.

### Controlled comparisons

Replay the identical recording through all filter and renderer candidates. This
avoids comparing different hand movements when judging algorithms.

Separately repeat physical trials at different screen positions, speeds, pen
orientations and display scales. Compare with another drawing application with
its smoothing disabled where possible. Different app results are clues; without
knowing their input and processing paths, they do not isolate the hardware.

## 4. Proposed final InfiniPaint architecture

### A. Loss-aware input adapter

Keep one Windows pen backend. Drain history oldest-first, avoiding duplication of
the latest sample. Handle insufficient history buffers and API failures explicitly.
Preserve sample attributes together before translating to app callbacks.

Use fractional coordinate mapping with validated device/display origins and DPI
behavior. Convert timestamps consistently to one monotonic application clock while
retaining original timestamps for diagnostics. Do not deduplicate solely by time:
equal timestamps can accompany distinct states or data. Handle invalid timing
without losing pen-up, cancellation or button transitions.

Track stroke ownership by pointer/device, not only by "pen" versus "mouse." Handle
capture loss, window changes and device removal without connecting unrelated
strokes. Define how camera changes during contact affect coordinates; either
support them with corresponding transform history or prevent them during drawing.

### B. Immutable source samples and reversible processing

Preserve original stroke samples separately from derived geometry. Record filter
version, parameters and coordinate transform context so replay is deterministic.
Provide a deliberate way to recover the unfiltered stroke.

Existing saved drawings must not be reprocessed on load. New persistence fields
require versioning and compatibility tests. Exports and network clients should
receive the intended committed geometry, never speculative preview samples.

### C. One controlled positional-processing stage

Expose Off, Gentle correction and Deliberate stabilization as distinct choices.
Off bypasses positional filtering and automatic shape fitting. It still performs
necessary coordinate transforms and normal stroke rendering.

Start evaluation with the existing timestamp-based, rotation-symmetric One Euro
filter. Tune using real recordings. Only introduce a more complex algorithm if
replay tests demonstrate a better wobble/detail/latency tradeoff.

Gentle correction should have an explicit displacement limit relative to reported
input. That bounds application changes, not error relative to unknowable intent.
Express settings in stable display-space units, independent of brush width and
canvas zoom. Test sharp direction changes and low-speed intentional curves; do
not assume an automatic corner/noise classifier is infallible.

Do not stack hidden positional filters. Keep pressure processing separately
controlled and preserve its association with position samples. No automatic line
snapping or global post-stroke reshaping unless the user explicitly selects it.

### D. Geometry that does not add avoidable distortion

Audit existing Catmull-Rom/curve generation after filtering. Specify a maximum
centerline approximation error and test overshoot, corners, uneven spacing and
tiny loops. Use a faithful polyline fallback when curve fitting cannot meet the
error bound. Antialiasing and brush-outline construction are separate concerns.

Process pen-up through a defined endpoint policy, not a blind raw-point append to
a lagging filtered path. Preserve terminal reports, distinguish contact from hover,
and test pressure taper and cancellation. If a small mutable live tail is used,
bound its length and revisions explicitly; it must not trigger whole-stroke
reshaping after lift. Do not promise an exact endpoint and zero lag simultaneously
for arbitrary filtered input.

### E. Optional prediction and responsive rendering

Prediction defaults off for the strict-fidelity profile. If enabled, it is a
short-lived preview only, replaced by measured data and disabled around uncertain
motion, turns, lifts and cancellation. Never commit extrapolated points.

Composite the live preview as one stroke contribution so translucent brushes do
not darken through overlap. Use the actual brush's caps and appearance. Minimize
input processing stalls and rendering queues before trying to conceal lag through
more prediction.

## 5. Current prototype: retain, revise, validate

Retain as a starting point:

- History retrieval and timestamp propagation.
- Display-space, shared-X/Y adaptive filtering.
- Separation of predicted preview from committed points.
- Dependency-free filter tests.

Known issues or unverified areas:

- `finish_pen` appends an unfiltered endpoint with the previous width, potentially
  introducing a kink or tail.
- SDL history deduplication currently uses only the performance timestamp and can
  skip distinct data/state transitions.
- The separate predicted line overlay needs transparency and brush-shape review.
- Existing downstream curve smoothing still needs geometric-fidelity tests.
- Per-pointer lifecycle, fallback timing and coordinate mapping need device tests.
- Presets were not calibrated against this user's pen recordings.
- Raw-source preservation and reversible stroke processing are not implemented.

The isolated synthetic test's roughly 46–80% reduction describes perpendicular
error amplitude for one artificial 12 Hz waveform across settings/sample rates.
It is not a percentage of real-world drawing problems fixed. A full application
build and real-device acceptance test have not established correctness. The
original portable executable has not been replaced with this prototype.

## 6. Delivery order and acceptance gates

1. Agree on this diagnostic specification and identify device/pen models.
2. Implement the minimal recorder, direct renderer and replay tools. Build only in
   an explicitly agreed environment; do not install prerequisites here.
3. Collect baseline recordings on the user's actual device.
4. Correct acquisition/coordinate/lifecycle defects before tuning smoothing.
5. Compare filter and renderer candidates on identical recordings. Set numerical
   error and latency budgets from device evidence and the user's detail tolerance.
6. Integrate the tested path into InfiniPaint, with versioned source preservation.
7. Validate pressure, eraser behavior, touch rejection, transparent brushes,
   cancellation, undo, save/reopen, export and collaboration. Test multiple display
   scales, rendering loads and Windows architectures supported by the release.
8. Produce a separate portable candidate in the authorized build environment.
   Preserve the prior executable until the user accepts device-test results.

Completion means reproducible input handling and demonstrated drawing quality
within declared limits—not an unsupported "100% fixed" claim. If baseline reports
already exceed the user's tolerance and faithful correction cannot satisfy it,
investigate the device/driver/pen combination rather than silently increasing
shape distortion.

## Sources

Primary documentation consulted during the preceding research:

1. [Microsoft: GetPointerPenInfoHistory](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getpointerpeninfohistory)
2. [Microsoft: POINTER_INFO](https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-pointer_info)
3. [Microsoft: Moving Jitter](https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/moving-jitter)
4. [Casiez, Roussel and Vogel: One Euro filter and tuning guidance](https://gery.casiez.net/1euro/)
5. [Procreate: Brush Studio settings, stabilization and motion filtering](https://help.procreate.com/procreate/handbook/5.2/brushes/brush-studio-settings)
6. [Apple: Leveraging touch input for drawing apps](https://developer.apple.com/documentation/uikit/leveraging-touch-input-for-drawing-apps)
7. [Apple: Minimizing latency with predicted touches](https://developer.apple.com/documentation/uikit/minimizing-latency-with-predicted-touches)
8. [Microsoft: InkPresenter custom drying and hosting](https://learn.microsoft.com/en-us/uwp/api/windows.ui.input.inking.inkpresenter.activatecustomdrying)

Local implementation references: `src/PenStabilizer.hpp`,
`conan/sdl/3.x/pen_history.inc`, `src/CanvasComponents/BrushComponentCode.cpp`,
`src/DrawingProgram/Tools/BrushTool.cpp`, and `tests/pen_stabilizer.cpp`.
