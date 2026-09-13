// GPL-3.0-or-later, part of InfiniPaint. Diagnostic rendering is not Skia parity.
#include "../src/BrushSampleWidths.hpp"
#include "../src/BrushCurveRendering.hpp"
#include "../src/PenStabilizer.hpp"
#include "core.hpp"
#include "trace_io.hpp"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>
int main(int argc,char** argv){
 try {
    pt::Session session;
    size_t index=0;
    const char* destination=nullptr;
    if(argc==3 && std::string(argv[1])=="--self-test") {
        destination=argv[2];
        for(unsigned i=0;i<120;++i) {
            pt::Sample s;s.sequence=i;s.device=1;s.pointer=1;s.frame=i;s.qpc=i+1;
            s.kind=pt::Kind::Pen;s.clock=pt::Clock::Qpc;s.time=s.receipt=i/120.0;
            s.contact=true;s.down=i==0;s.mapped=true;s.mask=1;s.pressure=i==60?1024:128;
            s.p={double(i),std::sin(i*.2)*5};session.samples.push_back(s);
        }
        std::stringstream encoded;pt::writeTrace(encoded,session);session=pt::readTrace(encoded);
    } else {
        if(argc!=4) throw std::runtime_error("Usage: pen_replay recording.pentrace NEW-output-directory stroke-index");
        size_t used=0;const std::string indexText=argv[3];
        if(indexText.empty()||indexText[0]=='-') throw std::runtime_error("Invalid stroke index");
        index=std::stoull(indexText,&used);
        if(used!=indexText.size()) throw std::runtime_error("Invalid stroke index");
        std::ifstream in(argv[1]);if(!in)throw std::runtime_error("Cannot read recording");
        session=pt::readTrace(in);destination=argv[2];
    }
    pt::Processor processor(pt::clockCalibration(session));
    for(const auto& s:session.samples)processor.consume(s);
    if(index>=processor.strokes().size())throw std::runtime_error("Stroke index outside recording");
    const auto& stroke=processor.strokes()[index];
    if(stroke.kind!=pt::Kind::Pen)throw std::runtime_error("Selected stroke is not a pen stroke");
    if(stroke.points.empty()||stroke.points.size()>20000)throw std::runtime_error("Select a stroke with 1..20000 samples");
    const auto raw=pt::rawPath(stroke);
    const auto motion=pt::motion(stroke,raw);
    std::vector<pen_stabilizer::Sample> input;
    std::vector<bool> continuous;
    size_t missingPressure=0,untrustedTiming=0;
    double minx=raw[0].x,maxx=minx,miny=raw[0].y,maxy=miny;
    for(size_t i=0;i<raw.size();++i){
        const auto& s=stroke.points[i];
        // Windows PEN_MASK_PRESSURE bit 1; native range 0..1024.
        // Unavailable pressure is visibly reported, not guessed from zero.
        const bool pressureValid=(s.mask&1u)!=0 && s.pressure<=1024;
        if(!pressureValid)++missingPressure;
        if(s.clock==pt::Clock::ReceiptFallback)++untrustedTiming;
        const float width=pressureValid ? 15.0f*s.pressure/1024.0f : 15.0f;
        input.push_back({{s.p.x,s.p.y},s.time,width});
        continuous.push_back(i==0 || motion[i].valid());
        minx=std::min(minx,s.p.x);maxx=std::max(maxx,s.p.x);
        miny=std::min(miny,s.p.y);maxy=std::max(maxy,s.p.y);
    }
    const std::filesystem::path dest=destination;
    if(std::filesystem::exists(dest))throw std::runtime_error("Output directory already exists; choose a new name");
    if(!std::filesystem::create_directory(dest))throw std::runtime_error("Cannot create output directory");
    std::ofstream svg(dest/"comparison.svg"), csv(dest/"vertices.csv"), info(dest/"README.txt");
    if(!svg||!csv||!info)throw std::runtime_error("Cannot create outputs");
    svg.imbue(std::locale::classic());csv.imbue(std::locale::classic());csv<<std::setprecision(17);
    const double spanx=std::max(1.0,maxx-minx)+24,spany=std::max(1.0,maxy-miny)+24;
    svg<<"<svg xmlns='http://www.w3.org/2000/svg' width='1440' height='2560' viewBox='0 0 1440 2560'><rect width='1440' height='2560' fill='#1c2025'/>";
    csv<<"correction,pressure,rendering,vertex,x_dip,y_dip,width_dip\n";
    int panel=0;
    const char* modes[]={"preserve","time-40ms","peak","legacy-factor-0.707"};
    for(bool enabled:{false,true}){
        const auto positions=pen_stabilizer::Stabilizer::filterBatch(input,continuous,{enabled,12,.120,4});
        for(int mode=0;mode<4;++mode){
            BrushPressure::SampleWidths widths;
            if(mode==1) widths.resetTime(input.front().width,input.front().time,40);
            else widths.reset(mode==2,input.front().width,mode==3,.707f);
            for(size_t i=1;i<input.size();++i)widths.append(input[i].width,input[i].time,continuous[i]);
            std::vector<BrushRendering::Point> samples;
            for(size_t i=0;i<input.size();++i)
                samples.push_back({positions[i].x,positions[i].y,widths.output(input[i].width,i)});
            for(bool curves:{false,true}){
                const auto rendered=curves?BrushRendering::boundedCurves(samples,.25):BrushRendering::polyline(samples);
                const char* render=curves?"bounded":"polyline";
                const int x=(panel%2)*720,y=(panel/2)*320;++panel;
                svg<<"<text x='"<<x+12<<"' y='"<<y+23<<"' fill='white' font-family='sans-serif' font-size='15'>"
                   <<(enabled?"Correction ON":"Correction OFF")<<" / "<<modes[mode]<<" / "<<render<<"</text>";
                svg<<"<svg x='"<<x+12<<"' y='"<<y+35<<"' width='696' height='273' viewBox='"<<minx-12<<' '<<miny-12<<' '<<spanx<<' '<<spany<<"'>";
                // Independent opaque segments approximate varying width. The CSV is
                // authoritative geometry; these are not the app's outline/AA pixels.
                if(rendered.size()==1)svg<<"<circle cx='"<<rendered[0].x<<"' cy='"<<rendered[0].y<<"' r='"<<rendered[0].width/2<<"' fill='#aa9fff'/>";
                for(size_t i=1;i<rendered.size();++i)svg<<"<path d='M "<<rendered[i-1].x<<' '<<rendered[i-1].y<<" L "<<rendered[i].x<<' '<<rendered[i].y
                   <<"' stroke='#aa9fff' stroke-width='"<<(rendered[i-1].width+rendered[i].width)/2<<"' stroke-linecap='round' fill='none'/>";
                svg<<"<polyline fill='none' stroke='#ffb86b' stroke-width='0.6' stroke-dasharray='2 2' points='";
                for(const auto& p:raw)svg<<p.x<<','<<p.y<<' ';
                svg<<"'/></svg>";
                for(size_t i=0;i<rendered.size();++i)csv<<enabled<<','<<modes[mode]<<','<<render<<','<<i<<','<<rendered[i].x<<','<<rendered[i].y<<','<<rendered[i].width<<'\n';
            }
        }
    }
    svg<<"</svg>";
    info<<"Selected stroke "<<index<<" of "<<processor.strokes().size()<<"; samples "<<input.size()
        <<"\nPosition library "<<pen_stabilizer::package_version<<" revision "<<pen_stabilizer::algorithm_revision
        <<"\nMissing/invalid pressure samples "<<missingPressure<<" (fixed width 15 DIP fallback)"
        <<"\nReceipt-clock samples "<<untrustedTiming<<" (no invented report cadence)"
        <<"\nOrange dashed: measured centerline; purple: diagnostic variable-width preview."
        <<"\nPreset brush: 15 DIP, minimum 0; smoothing 40 ms; correction 12 DIP / 120 ms / 4 DIP."
        <<"\nSame contact reports across 16 combinations. Input recording is never modified."
        <<"\nNot full upstream compatibility replay: original brush-size spacing, tip edits, Skia outline, antialiasing and document simplification are not simulated."
        <<"\nVertex CSV uses actual InfiniPaint pressure/curve helpers and shared batch position core."
        <<"\nNo physical ground truth or end-to-end input/render latency is measured.\n";
    svg.close();csv.close();info.close();
    if(!svg||!csv||!info)throw std::runtime_error("Output write failed; incomplete output directory retained");
    std::cout<<"Wrote "<<dest.string()<<" (16 variants); "<<processor.strokes().size()<<" strokes available\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
