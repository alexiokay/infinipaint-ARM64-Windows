#pragma once
#include <nlohmann/json.hpp>
struct ToolPanelPreferences {
    bool pinned = false;
    float x = 1.0f, y = .25f; // Fractions of available space, not screen pixels.
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(ToolPanelPreferences, pinned, x, y)
};
