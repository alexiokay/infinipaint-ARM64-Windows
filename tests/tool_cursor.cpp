#include "../src/ToolCursorMath.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>

void require(bool value) { if (!value) throw std::runtime_error("Tool cursor regression"); }
int main() {
    try {
        using namespace ToolCursor;
        require(visible(true, true, false, false, false, false)); // Mouse hover.
        require(visible(true, false, false, true, false, false)); // Pen hover, no SDL mouse focus.
        require(visible(true, false, true, false, true, true)); // Pen down after a touch event.
        require(visible(true, false, true, false, false, true)); // Active finger stroke.
        require(!visible(true, true, true, false, false, false)); // No finger hover cursor.
        require(!visible(true, false, false, false, false, false)); // Pointer outside window.
        for (unsigned flags = 0; flags < 32; ++flags)
            require(!visible(false, flags & 1, flags & 2, flags & 4, flags & 8, flags & 16));
        require(radius(15) == 7.5f);
        require(radius(15, 2) == 15);
        require(radius(15, .5f) == 3.75f);
        require(radius(0) == 0); // Never enlarge a tiny/zero footprint.
        require(radius(.2f) == .1f);
        require(radius(-1) == 0);
        require(radius(std::numeric_limits<float>::infinity()) == 0);
        require(radius(std::numeric_limits<float>::quiet_NaN()) == 0);
        require(displayScale(1.5f) == 1.5f && displayScale(2) == 2);
        require(displayScale(0) == 1 && displayScale(-1) == 1);
        require(displayScale(std::numeric_limits<float>::quiet_NaN()) == 1);
        std::cout << "Tool cursor visibility and geometry passed\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
