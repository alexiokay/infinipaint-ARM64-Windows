#pragma once
#include <algorithm>
#include <cmath>

namespace ToolCursor {
// Pen contact/proximity need not imply SDL mouse focus. Finger-only hover has
// no meaningful cursor, but an active finger stroke still gets a footprint.
inline bool visible(bool windowFocus, bool mouseFocus, bool touchInput,
                    bool penProximity, bool penDown, bool activeStroke) {
    return windowFocus && (mouseFocus || penProximity || penDown || activeStroke) &&
        (!touchInput || penProximity || penDown || activeStroke);
}

inline float displayScale(float scale) {
    return std::isfinite(scale) && scale > 0 ? scale : 1.0f;
}

inline float radius(float diameter, float coordinateScale = 1.0f) {
    const float value = diameter * coordinateScale * 0.5f;
    return std::isfinite(value) && value >= 0 ? value : 0.0f;
}
}
