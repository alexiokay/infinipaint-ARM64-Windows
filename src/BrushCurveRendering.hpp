#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

// Pure render-stage geometry used by InfiniPaint and the replay utility.
// Does not depend on pressure policy, clocks, native APIs or the position filter.
namespace BrushRendering {
struct Point {
    double x=0, y=0;
    float width=1;
};
inline std::vector<Point> polyline(const std::vector<Point>& input) {
    std::vector<Point> out;
    for(const auto& p:input) {
        if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.width)||p.width<0) continue;
        if(!out.empty() && p.x==out.back().x && p.y==out.back().y)
            out.back().width=std::max(out.back().width,p.width);
        else out.push_back(p);
    }
    return out;
}
// Cubic controls stay inside a capsule around each input chord. By Bezier's
// convex-hull property, the centerline stays within maxDeviation of that chord.
// Every original vertex is retained and widths are interpolated linearly.
// Not Catmull-Rom parity. Neighboring spans may revise when a new vertex arrives.
inline std::vector<Point> boundedCurves(const std::vector<Point>& input, double maxDeviation) {
    auto p=polyline(input);
    if(p.size()<2 || !std::isfinite(maxDeviation) || maxDeviation<=0) return p;
    std::vector<Point> out{p.front()};
    for(size_t i=0;i+1<p.size();++i) {
        const auto a=p[i], b=p[i+1];
        const double dx=b.x-a.x,dy=b.y-a.y,len=std::hypot(dx,dy);
        if (!std::isfinite(len) || len<=0) { out.push_back(b); continue; }
        const double ux=dx/len,uy=dy/len,cap=std::min(maxDeviation,len*.1);
        auto control=[&](double x,double y) {
            const double along=std::clamp((x-a.x)*ux+(y-a.y)*uy,0.0,len);
            const double normal=std::clamp(-(x-a.x)*uy+(y-a.y)*ux,-cap,cap);
            return Point{a.x+along*ux-normal*uy,a.y+along*uy+normal*ux,1};
        };
        auto tangent=[&](size_t k) {
            if(k==0) return Point{dx,dy,1};
            if(k+1==p.size()) return Point{dx,dy,1};
            double ax=p[k].x-p[k-1].x,ay=p[k].y-p[k-1].y;
            double bx=p[k+1].x-p[k].x,by=p[k+1].y-p[k].y;
            const double al=std::hypot(ax,ay),bl=std::hypot(bx,by);
            ax/=al;ay/=al;bx/=bl;by/=bl;
            // Retain sharp reversals/corners, do not round across them.
            if(ax*bx+ay*by<=0) return Point{0,0,1};
            return Point{ax+bx,ay+by,1};
        };
        auto t0=tangent(i),t1=tangent(i+1);
        auto handle=[&](Point end,Point t,double sign) {
            const double n=std::hypot(t.x,t.y);
            return n==0 ? end : control(end.x+sign*t.x/n*len/3,end.y+sign*t.y/n*len/3);
        };
        const auto c0=handle(a,t0,1),c1=handle(b,t1,-1);
        // Eight subdivisions per measured span; no unbounded adaptive recursion.
        for(int k=1;k<=8;++k) {
            if(k==8) { out.push_back(b); continue; }
            const double t=k/8.0,s=1-t;
            out.push_back({s*s*s*a.x+3*s*s*t*c0.x+3*s*t*t*c1.x+t*t*t*b.x,
                           s*s*s*a.y+3*s*s*t*c0.y+3*s*t*t*c1.y+t*t*t*b.y,
                           static_cast<float>(a.width+(b.width-a.width)*t)});
        }
    }
    return out;
}
}
