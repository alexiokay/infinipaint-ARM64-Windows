#pragma once
#include <algorithm>
#include <cmath>

namespace UIControlGeometry {
inline float unit(float value) { return std::isfinite(value) ? std::clamp(value,0.0f,1.0f) : 0.0f; }
inline float sliderInset(float width) { return std::min(8.0f,std::max(0.0f,width)*.5f); }
inline float sliderPosition(float width,float fraction) {
    const float inset=sliderInset(width);
    return inset+unit(fraction)*std::max(0.0f,width-2*inset);
}
inline float sliderFraction(float width,float x) {
    const float inset=sliderInset(width), span=width-2*inset;
    return span>0 ? unit((x-inset)/span) : 0;
}
inline float panelOffset(float available,float fraction) { return std::max(0.0f,available)*unit(fraction); }
inline float panelFraction(float available,float offset) { return available>0 ? unit(offset/available) : 0; }
inline float scrollbarGutter(bool visible,bool touch) { return visible ? (touch ? 28.0f : 16.0f) : 0.0f; }
}
