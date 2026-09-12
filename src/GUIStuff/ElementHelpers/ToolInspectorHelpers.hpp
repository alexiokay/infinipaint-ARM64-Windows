#pragma once
#include <algorithm>
#include "ButtonHelpers.hpp"
#include "LayoutHelpers.hpp"
#include "TextLabelHelpers.hpp"

namespace GUIStuff::ElementHelpers {
inline void tool_inspector(GUIManager& gui, std::string_view title, const std::function<void()>& content) {
    const float width = std::min(280.0f, std::max(80.0f, gui.io.windowSize.x() - 4.0f * gui.io.theme->padding1));
    CLAY_AUTO_ID({.layout = {
        .sizing = {.width = CLAY_SIZING_FIXED(width), .height = CLAY_SIZING_FIT(0)},
        .childGap = gui.io.theme->childGap1,
        .layoutDirection = CLAY_TOP_TO_BOTTOM
    }}) {
        text_label_size(gui, title, 1.12f);
        content();
    }
}

inline void inspector_hint(GUIManager& gui, std::string_view text) {
    CLAY_TEXT(gui.strArena.std_str_to_clay_str(text), CLAY_TEXT_CONFIG({
        .textColor = convert_vec4<Clay_Color>(gui.io.theme->frontColor2),
        .fontSize = static_cast<uint16_t>(gui.io.fontSize * 0.85f)}));
}

inline void inspector_section(GUIManager& gui, std::string_view title, const std::function<void()>& content) {
    CLAY_AUTO_ID({.layout = {
        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0)},
        .padding = {.top = 6},
        .childGap = 6,
        .layoutDirection = CLAY_TOP_TO_BOTTOM
    }}) {
        inspector_hint(gui, title);
        content();
    }
}

template<typename T>
void inspector_choice(GUIManager& gui, const char* id, T* value,
                      std::string_view firstLabel, T firstValue,
                      std::string_view secondLabel, T secondValue) {
    gui.new_id(id, [&] {
        left_to_right_layout(gui, CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0), [&] {
            text_button(gui, "first", firstLabel, {.isSelected = *value == firstValue, .wide = true,
                .onClick = [value, firstValue] { *value = firstValue; }});
            text_button(gui, "second", secondLabel, {.isSelected = *value == secondValue, .wide = true,
                .onClick = [value, secondValue] { *value = secondValue; }});
        });
    });
}
}
