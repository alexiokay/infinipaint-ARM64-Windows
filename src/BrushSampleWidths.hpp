#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

namespace BrushPressure {
// Attribute policies never modify the immutable source or corrected positions.
class SampleWidths {
public:
    void reset(bool uniformPeak, float firstWidth, bool smooth=false, float factor=.707f) {
        peak=uniformPeak; maximum=firstWidth; smoothing=smooth && !peak;
        timeBased=false; times.clear(); runStart=0;
        propagation=std::isfinite(factor) ? std::clamp(factor,0.0f,1.0f) : .707f;
        widths.clear(); if(smoothing) widths.push_back(firstWidth);
    }
    // Time-domain maximum envelope: exp(-deltaSeconds/tau). This is width
    // propagation, not position filtering and not a zero-latency pressure estimator.
    void resetTime(float firstWidth, double firstTime, double milliseconds) {
        reset(false, firstWidth, true);
        timeBased=true;
        tau=std::isfinite(milliseconds) ? std::clamp(milliseconds,0.0,200.0)*.001 : .040;
        times.push_back(firstTime);
    }
    bool append(float width, double time=0, bool continuous=true) {
        const float previous=maximum;
        maximum=std::max(maximum,width);
        if(smoothing) {
            widths.push_back(width);
            if (timeBased) {
                const double dt=time-times.back();
                times.push_back(time);
                if (!continuous || !std::isfinite(dt) || dt<=0 || dt>.050 || tau==0) {
                    runStart=widths.size()-1;
                    return false;
                }
            }
            const auto attenuation=[&](size_t right) {
                return timeBased ? static_cast<float>(std::exp(-(times[right]-times[right-1])/tau)) : propagation;
            };
            widths.back()=std::max(widths.back(),widths[widths.size()-2]*attenuation(widths.size()-1));
            for(size_t i=widths.size()-1;i>runStart;--i) {
                const float raised=widths[i]*attenuation(i);
                if(raised<=widths[i-1]) break;
                widths[i-1]=raised;
            }
        }
        return smoothing || (peak && maximum>previous);
    }
    float output(float sourceWidth, size_t index=0) const {
        return peak ? maximum : smoothing ? widths.at(index) : sourceWidth;
    }
private:
    bool peak=false, smoothing=false, timeBased=false;
    double tau=.040;
    size_t runStart=0;
    std::vector<double> times;
    float maximum=0, propagation=.707f;
    std::vector<float> widths;
};
}
