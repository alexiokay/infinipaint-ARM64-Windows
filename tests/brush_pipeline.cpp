#include "../src/BrushPressureConfig.hpp"
#include "../src/BrushSampleWidths.hpp"
#include "../src/BrushCurveRendering.hpp"
#include "../src/PenStabilizer.hpp"
#include <iostream>
#include <stdexcept>
#include <limits>
void check(bool c,const char* m){if(!c)throw std::runtime_error(m);}
int main(){
 try{
    using namespace BrushPressure;
    using nlohmann::json;
    for(auto mode:{Response::Original,Response::Preserve,Response::Peak,Response::Time})
      for(auto engine:{Engine::Compatibility,Engine::Samples})
        for(bool correction:{false,true})
          for(auto render:{Rendering::Polyline,Rendering::BoundedCurves}) {
            Config c;c.pressureResponse=mode;c.engine=engine;c.rendering=render;c.correctionIndependent=true;
            c.migrateCorrection(correction);
            check(c.samplePath()==(engine==Engine::Samples),"pressure or correction selected geometry engine");
            const auto restored=json(c).get<Config>();
            check(restored.engine==engine && restored.rendering==render && restored.pressureResponse==mode,"roundtrip");
          }
    for(bool oldMarker:{false,true}) for(bool filter:{false,true})
      for(auto mode:{"original","preserve","peak"}) {
        auto c=json{{"pressureResponse",mode},{"correctionIndependent",oldMarker}}.get<Config>();
        bool enabled=filter;c.migrateCorrection(enabled);
        const bool expected=std::string(mode)!="original" || (oldMarker && filter);
        check(c.samplePath()==expected,"old effective pipeline changed");
        c.pressureResponse=Response::Time;
        c.migrateCorrection(enabled);
        check(c.samplePath()==expected,"pressure remigrated geometry");
      }
    auto explicitNew=json{{"engine","samples"},{"pressureResponse","original"}}.get<Config>();
    bool explicitOn=true;explicitNew.migrateCorrection(explicitOn);
    check(explicitOn && explicitNew.samplePath(),"new engine schema incorrectly received legacy filter reset");
    auto future=json{{"engine",42},{"pressureResponse","unknown"},{"rendering","future"}}.get<Config>();
    bool on=true;future.migrateCorrection(on);
    check(!future.samplePath(),"invalid engine should fall back safely");

    // Same physical duration and peak event at every sample rate: analytical
    // pressure envelope is peak*exp(-elapsed/tau), not factor^sampleCount.
    for(int hz:{60,120,240,672}) {
        SampleWidths w;w.resetTime(10,0,40);
        const int n=hz/2;
        for(int i=1;i<=n;++i) w.append(0,double(i)/hz);
        const float expected=10*std::exp(-.5/.04);
        check(std::abs(w.output(0,n)-expected)<1e-7,"report-rate-dependent decay");
        w.resetTime(0,0,40);
        for(int i=1;i<=n;++i) w.append(i==n?10:0,double(i)/hz);
        check(std::abs(w.output(0,0)-expected)<1e-7,"report-rate-dependent backward spread");
    }
    SampleWidths w;w.resetTime(10,1,40);
    w.append(1,1);check(w.output(1,1)==1,"duplicate clock crossed boundary");
    w.append(1,.9);check(w.output(1,2)==1,"backward clock crossed boundary");
    w.append(1,2);check(w.output(1,3)==1,"gap crossed boundary");
    w.append(1,2.001,false);check(w.output(1,4)==1,"provenance boundary ignored");
    w.resetTime(10,0,0);w.append(1,.01);check(w.output(1,1)==1,"zero smoothing not off");

    using BrushRendering::Point;
    const std::vector<Point> input{{0,0,1},{10,0,2},{20,8,5},{30,8,1},{30,8,3},{20,8,2}};
    auto base=BrushRendering::polyline(input),curve=BrushRendering::boundedCurves(input,.25);
    check(curve.size()==1+8*(base.size()-1),"render count");
    for(size_t i=0;i+1<base.size();++i){
        auto a=base[i],b=base[i+1];
        const double dx=b.x-a.x,dy=b.y-a.y,len=std::hypot(dx,dy);
        for(int k=0;k<=8;++k){
            auto p=curve[i*8+k];
            const double along=((p.x-a.x)*dx+(p.y-a.y)*dy)/len;
            const double normal=std::abs(-(p.x-a.x)*dy+(p.y-a.y)*dx)/len;
            check(along>=-1e-8 && along<=len+1e-8 && normal<=.25+1e-8,"curve exceeds capsule");
            check(p.width>=std::min(a.width,b.width)-1e-6 && p.width<=std::max(a.width,b.width)+1e-6,"width overshoot");
        }
        check(curve[i*8].x==a.x && curve[i*8].y==a.y && curve[i*8].width==a.width,"source knot lost");
    }
    check(curve.back().x==base.back().x && curve.back().y==base.back().y && curve.back().width==base.back().width,"tip lost");
    // Width policies and render choice must not feed back into correction.
    for(bool enabled:{false,true}){
        PenInput::Stabilizer p;p.reset({enabled});
        for(int i=0;i<100;++i) p.append({{double(i),std::sin(i*.5)},i*.01,float(i%3+1)});
        const auto before=p.positions();
        for(bool curved:{false,true}) for(bool peak:{false,true}){
            SampleWidths widths;widths.reset(peak,1);
            std::vector<Point> points;
            for(size_t i=0;i<before.size();++i){
                widths.append(p.samples()[i].width);
                points.push_back({before[i].x,before[i].y,p.samples()[i].width});
            }
            for(auto& point:points) point.width=widths.output(point.width);
            const auto rendered=curved?BrushRendering::boundedCurves(points,.25):BrushRendering::polyline(points);
            check(!rendered.empty(),"empty render");
            for(size_t i=0;i<before.size();++i)
                check(p.positions()[i].x==before[i].x && p.positions()[i].y==before[i].y,"render changed filter");
        }
    }
    std::cout<<"Independent pipeline, time-domain pressure and bounded rendering passed\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
