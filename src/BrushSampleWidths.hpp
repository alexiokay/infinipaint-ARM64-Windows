#pragma once
#include <algorithm>

namespace BrushPressure {
// Width policy is separate from positional correction. Source widths are never
// rewritten; peak mode revises the rendered widths, including the frozen path.
class SampleWidths {
public:
    void reset(bool uniformPeak, float firstWidth) { peak = uniformPeak; maximum = firstWidth; }
    bool append(float width) {
        const float previous = maximum;
        maximum = std::max(maximum, width);
        return peak && maximum > previous;
    }
    float output(float sourceWidth) const { return peak ? maximum : sourceWidth; }
private:
    bool peak = false;
    float maximum = 0;
};
}
