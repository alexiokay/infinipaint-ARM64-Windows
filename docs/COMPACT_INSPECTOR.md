# Compact movable inspector and control fixes

Current pressure/geometry controls and migration are described in [BRUSH_PIPELINE.md](BRUSH_PIPELINE.md); older pressure-engine descriptions below are historical.

Implemented on `graphite-ui`; build on the other PC. No local build, prerequisite
installation or system/registry setting change is part of this work.

## Using it

- The shared desktop tool inspector starts compact: quick Brush/Eraser size,
  three swatches from the selected palette and **More** for the existing radial
  paint menu. Other tools use the same inspector container.
- **Options / Hide** expands/collapses the inspector. Beginning a canvas stroke
  collapses an unpinned inspector without consuming that first drawing point.
- **Pin** holds the expanded inspector open. Its preference is saved.
- Drag the **Drag** header with mouse, pen or one finger. **Reset** restores its
  position. Position is saved as fractions, clamped to available space when the
  window/UI scale changes. The header is outside scrolling content.
- The phone keeps its existing popup presentation and shared tool controls;
  this is not a new phone window manager. Colour pickers and general dialogs
  have not all been made draggable.
- Tab still hides/shows the desktop UI. Existing custom palettes and the
  right-click radial menu are retained; the quick swatches are not a new colour
  history system. Their selection retains the current alpha.

## Rendering/layout fixes

Scroll areas reserve a permanent scrollbar gutter for enabled scrollbar axes,
including wider touch hit areas. Tool content can shrink into the available
width rather than forcing a fixed width underneath that gutter.

Slider tracks are inset to contain the thumb and outline at both endpoints.
Drawing and pointer mapping use the same inset; minimum and maximum remain
reachable. Thumb height is bounded during its held animation.

Uniform rounded borders use one closed inset Skia outline rather than separate
corner/edge segments. Asymmetric borders retain their existing rendering path.

## Pressure and position correction are independent

The default label is now **Smoothed pressure**, alongside Preserve samples and
Uniform peak width. Local correction can be enabled with any of the three.
Switching pressure response no longer disables correction. Settings are captured
at contact-down for the sample-based path, including its width propagation factor.

Smoothed + correction Off retains the creator's original position/width pipeline.
Smoothed + correction On uses corrected report positions plus backward width
propagation on the sample widths. It does not claim to reproduce the original
point-spacing/interpolation pipeline while also changing its position filter.
Per-report attenuation can feel different from attenuation on the original
sparser generated points; this combination requires physical-pen acceptance.
Preserve keeps individual widths and Peak uses the greatest width so far.
All keep raw source attributes unchanged. Eraser behavior is not switched to a
revising correction path.

A one-time `correctionIndependent` migration runs after both configuration
sections load. Older Original-mode configurations had correction inactive even
if its stored toggle was true: migration preserves that effective Off state.
Older Preserve/Peak choices keep their existing toggle. After migration, pressure
mode changes never write the correction toggle. No arbitrary old stroke is
reprocessed. The current response selector replaces, rather than supplements,
the former preservation checkbox.

## Verification and limits

Pure C++ tests cover slider mapping/endpoints, panel coordinate bounds and JSON
roundtrip, scrollbar gutter size, pressure-policy behavior and one-time migration.
Source checks cover wiring; existing filter/oracle and Windows input tests remain.
These do not render the full application or exercise drag capture in a live GUI.

Before daily use, build and check on Surface Pro 11 / Metapen M2: slider minimum
and maximum, held animation, light/dark custom themes, 100/150/200% scaling,
short/narrow windows, mouse/pen/single-finger dragging, second-finger cancellation,
focus loss, pin/hide/reset, auto-collapse with the first stroke point retained,
all pressure/correction combinations, dots, slow curves, save/restart and erasing.
No claim of complete GUI validation or perfect hardware-wobble removal is made.
