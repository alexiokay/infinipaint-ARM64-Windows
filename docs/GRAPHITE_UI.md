# Graphite UI and tool cursor

Latest UI behavior: [compact movable inspector and control fixes](COMPACT_INSPECTOR.md).
The inspector can now collapse, pin and move; the initial implementation below
is retained as context. Pressure/correction independence is described in that update.

This is a separate UI branch based on the pen integration; it does not change the
three existing upstream pen PRs, graphics backend, ARM64 build scripts, pressure
defaults, eraser hit testing, or stored drawing geometry.

Pressure-mode controls and source-level library extraction now follow the
[implemented pressure modes](PRESSURE_MODES.md). The earlier
[design proposal](PEN_UX_AND_LIBRARY_PROPOSAL.md) is retained as historical context.

## Appearance and controls

The built-in `Default` interface theme is now Graphite: neutral dark surfaces,
restrained violet highlights, consistent spacing, readable secondary text, and
32-unit control rows. This is a colour scheme, **not Skia's Graphite renderer**.
Canvas/paper colours and saved custom themes are untouched. Existing custom themes
still load; missing control metrics receive defaults. `Settings > Theme` supports
control height (24–48) and control corner radius (0–12), in addition to the existing
palette/spacing fields. Save As creates a custom copy as before.

Text buttons, numeric/text fields, sliders, checkbox and radio rows use a shared height; full checkbox
and radio labels are clickable. Toggle glyphs stay stable instead of morphing on
hover. The two tool inspectors share their desktop/phone implementation.

Brush: size first, then stroke options, with advanced pen response tucked away.
Original smoothing remains the default. A three-mode selector replaces the
preservation checkbox: Original / Preserve samples / Uniform peak width. Local
correction supports either sample-based mode and cannot override Original.

Eraser: size, Current layer / All visible, and Whole objects / Portions. Portions
cuts mesh strokes, while fully enclosed objects can still be removed by existing
eraser logic. The inspector explains real-time versus release-to-apply behavior.
Desktop tool options are vertically scrollable when they exceed the available
height; the phone already uses a scrollable popup.

## Cursor visibility

Previously the eraser drew two translucent fills of its path and gated them on
mouse focus/touch state. Real-time erasing repeatedly resets that path. The new
cursor is an independent screen-space black/white outline, visible with pen
proximity/contact even if SDL does not report mouse focus. An active stroke uses
its originating device to choose cursor position. It is hidden when the app is
unfocused, outside the canvas/window, or over controls when not drawing.

The eraser ring uses the current generated width and coordinate scale during a
stroke; hover previews the selected full size. Very small/zero footprints get
four locator marks, not an enlarged erase radius. Outline weight follows display
scale. Non-real-time erasing retains a shaded pending-region preview. Cursor code
does not call erase operations or appear in document exports/screenshots.

## Validation and remaining checks

Dependency-free C++ tests cover cursor visibility, radius, scaling and invalid
numbers; existing filter/history tests remain. Python checks cover source wiring,
theme defaults/serialization registration, shared inspectors and default text
contrast. These are **not a full app compilation, GUI rendering test, or device
test**. No build prerequisites were installed on the user's PC.

Before merging, build on the other PC and check: mouse/pen hover and contact,
light/dark/busy artwork, tiny sizes, pressure, 100/150/200% display scaling, zoom,
window focus, touch coexistence, realtime/release erasing, whole/partial erase,
phone/narrow windows, long labels, control clicks, advanced-panel scrolling,
custom-theme reload, undo, and export without cursor overlays.
