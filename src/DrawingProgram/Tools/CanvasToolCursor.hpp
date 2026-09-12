#pragma once
#include "../../ToolCursorMath.hpp"
#include <include/core/SkCanvas.h>
#include <include/core/SkPaint.h>

namespace ToolCursor {
// Screen-space overlay only; never feeds back into brush or erase geometry.
inline void draw(SkCanvas* canvas, float x, float y, float radius, float scale) {
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(radius) || radius < 0) return;
    scale = displayScale(scale);
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeCap(SkPaint::kRound_Cap);
    for (int pass = 0; pass < 2; ++pass) {
        paint.setColor4f(pass == 0 ? SkColor4f{0, 0, 0, 1} : SkColor4f{1, 1, 1, 1});
        paint.setStrokeWidth((pass == 0 ? 3.0f : 1.0f) * scale);
        if (radius > 0) canvas->drawCircle(x, y, radius, paint);
        // Locate tiny/zero-pressure footprints without pretending the eraser is larger.
        if (radius < 3.0f * scale) {
            const float inner = 4.0f * scale, outer = 7.0f * scale;
            canvas->drawLine(x - outer, y, x - inner, y, paint);
            canvas->drawLine(x + inner, y, x + outer, y, paint);
            canvas->drawLine(x, y - outer, x, y - inner, paint);
            canvas->drawLine(x, y + inner, x, y + outer, paint);
        }
    }
}
}
