#include "../src/UIControlGeometry.hpp"
#include "../src/ToolPanelPreferences.hpp"
#include <iostream>
#include <stdexcept>
#include <limits>
void require(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
int main() {
    try {
        using namespace UIControlGeometry;
        for(float width : {0.0f,8.0f,16.0f,100.0f,320.0f}) {
            for(float fraction : {0.0f,.25f,.5f,.707f,1.0f}) {
                const float x=sliderPosition(width,fraction);
                require(std::isfinite(x) && x>=0 && x<=width,"slider bounds");
                if(width>16) {
                    require(x>=8 && x<=width-8,"thumb/outline outside track bounds");
                    require(std::abs(sliderFraction(width,x)-fraction)<1e-5,"draw/input mapping mismatch");
                }
            }
            require(sliderFraction(width,-100)==0,"minimum not reachable");
            if(width>16) require(sliderFraction(width,width+100)==1,"maximum not reachable");
        }
        require(sliderFraction(100,std::numeric_limits<float>::quiet_NaN())==0,"invalid slider input");
        for(float available : {-5.0f,0.0f,120.0f,1000.0f})
            for(float offset : {-100.0f,0.0f,20.0f,2000.0f}) {
                const float p=panelOffset(available,panelFraction(available,offset));
                require(p>=0 && p<=std::max(0.0f,available),"panel outside available canvas");
            }
        require(scrollbarGutter(true,true)>=24 && scrollbarGutter(true,false)>=12,"scrollbar gutter too narrow");
        require(scrollbarGutter(false,true)==0,"disabled scrollbar gutter");
        const auto old=nlohmann::json::object().get<ToolPanelPreferences>();
        require(!old.pinned && old.x==1 && old.y==.25f,"old panel preferences default");
        const ToolPanelPreferences prefs{true,.2f,.8f};
        const auto restored=nlohmann::json(prefs).get<ToolPanelPreferences>();
        require(restored.pinned && restored.x==prefs.x && restored.y==prefs.y,"panel persistence");
        std::cout<<"UI geometry and panel preferences passed\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
