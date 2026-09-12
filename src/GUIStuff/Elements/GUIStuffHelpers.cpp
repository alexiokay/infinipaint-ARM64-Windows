/*  
 * InfiniPaint
 * Copyright (C) 2025-2026 Yousef Khadadeh
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "Element.hpp"
#include "Helpers/MathExtras.hpp"
#include <chrono>

namespace GUIStuff {
    SCollision::AABB<float> clay_bounding_box_to_aabb(const Clay_BoundingBox& bb) {
        return {{bb.x, bb.y}, {bb.x + bb.width, bb.y + bb.height}};
    }

    SkFont get_setup_skfont() {
        SkFont font;
        font.setLinearMetrics(true);
        font.setHinting(SkFontHinting::kNormal);
        //font.setForceAutoHinting(true);
        font.setSubpixel(true);
        font.setBaselineSnap(true);
        font.setEdging(SkFont::Edging::kSubpixelAntiAlias);
        //paint.setAntiAlias(true);
        return font;
    }

    SkFont UpdateInputData::get_font(float fSize) const {
        SkFont f = get_setup_skfont();
        f.setTypeface(textTypeface);
        f.setSize(fSize);
        return f;
    }

    std::shared_ptr<Theme> get_default_dark_mode() {
        std::shared_ptr<Theme> theme(std::make_shared<Theme>());
        // Graphite: neutral surfaces, restrained violet accents, readable secondary text.
        // This affects app chrome only, never the document's canvas colour.
        theme->fillColor1 = {0.67f, 0.65f, 0.98f, 1.0f};
        theme->fillColor2 = {0.42f, 0.44f, 0.50f, 1.0f};
        theme->backColor0 = {0.055f, 0.060f, 0.070f, 1.0f};
        theme->backColor1 = {0.110f, 0.120f, 0.140f, 1.0f};
        theme->backColor2 = {0.170f, 0.185f, 0.215f, 1.0f};
        theme->frontColor1 = {0.93f, 0.94f, 0.96f, 1.0f};
        theme->frontColor2 = {0.70f, 0.73f, 0.78f, 1.0f};
        theme->errorColor = {1.0f, 0.42f, 0.44f, 1.0f};
        theme->warningColor = {0.98f, 0.76f, 0.38f, 1.0f};
        theme->padding1 = 12;
        theme->childGap1 = 8;
        theme->windowCorners1 = 10;
        theme->controlHeight = 32;
        theme->controlCorners = 6;
        return theme;
    }
    
    void SelectionHelper::update(bool isHovering, bool isLeftClick, bool isLeftHeld, const Vector2f& cursorPos) {
        hovered = isHovering;
    
        clicked = false;
        tapped = false;
        justUnselected = false;
        bool oldSelected = selected;
    
        if(isLeftClick) {
            if(hovered) {
                held = true;
                clicked = true;
                selected = true;
                timeClicked = std::chrono::steady_clock::now();
                clickPos = cursorPos;
            }
            else
                selected = false;
        }
        else if(!isLeftHeld) {
            if(held && hovered && (std::chrono::steady_clock::now() - timeClicked) < std::chrono::milliseconds(250) && vec_distance_sqrd(clickPos, cursorPos) < (25 * 25))
                tapped = true;
            held = false;
        }

        if(!selected && oldSelected)
            justUnselected = true;
    }
}
