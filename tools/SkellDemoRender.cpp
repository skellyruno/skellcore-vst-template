// Offline, deterministic proof using the actual shared LiveMultibandDucker.
// Builds without JUCE. No plugin host, live DAW, private audio or UI is implied.
#include <SkellCore/LiveEngine.h>
#include <SkellCore/DemoTrace.h>
#include <filesystem>
#include <iostream>
#include <vector>
#include <cstdint>
#include <fstream>
#include <regex>
namespace fs=std::filesystem;
static constexpr int rate=48000, block=480, seconds=12, frames=rate*seconds;
static void u16(std::ofstream& f, std::uint16_t v) { f.put(char(v)); f.put(char(v>>8)); }
static void u32(std::ofstream& f, std::uint32_t v) { u16(f,v&65535);u16(f,v>>16); }
static void writeWav(const fs::path& path,const std::vector<float>& x){
    std::ofstream f(path,std::ios::binary);f.write("RIFF",4);u32(f,36+2*x.size());f.write("WAVEfmt ",8);
    u32(f,16);u16(f,1);u16(f,1);u32(f,rate);u32(f,2*rate);u16(f,2);u16(f,16);
    f.write("data",4);u32(f,2*x.size());
    for(auto s:x){if(!std::isfinite(s)||std::abs(s)>1.f)throw std::runtime_error("Invalid/clipped fixture audio");u16(f,static_cast<std::uint16_t>(std::lround(s*32767)));}
    if(!f)throw std::runtime_error("WAV write failed");
}
int main(int argc,char** argv){
    try {
        if(argc!=3 && argc!=6)throw std::invalid_argument("Usage: SkellDemoRender NEW_OUTPUT_DIR EXACT_CORE_COMMIT [AMOUNT_1 AMOUNT_2 AMOUNT_3]");
        std::array<float,3> amounts{0.f,.5f,1.f};
        if(argc==6)for(int i=0;i<3;++i){std::size_t end=0;const std::string text=argv[i+3];const float v=std::stof(text,&end);if(end!=text.size()||!std::isfinite(v)||v<0||v>1)throw std::invalid_argument("Amounts must be normalized 0..1");amounts[i]=v;}
        const std::string rev=argv[2];if(!std::regex_match(rev,std::regex("[a-f0-9]{40}")))throw std::invalid_argument("Require exact source revision");
        const fs::path out=argv[1];if(!fs::create_directory(out))throw std::runtime_error("Output exists or parent missing");
        std::vector<float> dry(frames),wet(frames),side(frames);
        constexpr double pi=3.141592653589793;
        // Three known tones + a periodic sidechain; not a vocal or song benchmark.
        for(int i=0;i<frames;++i){const double t=double(i)/rate;
            dry[i]=float(.19*std::sin(2*pi*960*t)+.16*std::sin(2*pi*280*t)+.12*std::sin(2*pi*3270*t));
            const double pulse=.15+.85*std::pow(std::sin(pi*t*1.5),4);
            side[i]=float(.45*pulse*std::sin(2*pi*960*t));
        }
        wet=dry;skellcore::live::LiveMultibandDucker engine;engine.prepare(rate);
        skellcore::live::LiveDuckerParams p;p.attackMs=5;p.releaseMs=100;p.smooth=.3f;
        skellcore::demo::Trace trace((out/"events.csv").string(),(out/"meters.csv").string(),frames);
        for(int i=0;i<frames;i+=block){
            if(i%(4*rate)==0){p.amount=amounts[static_cast<std::size_t>(i/(4*rate))];engine.setParameters(p);trace.parameter(i,"amount",p.amount);}
            float* main[]={wet.data()+i};const float* sc[]={side.data()+i};engine.process(main,sc,1,1,block);
            std::array<float,10> cut{};engine.copyDisplayBands(0,nullptr,nullptr,cut.data());trace.block(i,block,dry.data()+i,wet.data()+i,cut);
        }
        trace.finish();writeWav(out/"dry.wav",dry);writeWav(out/"processed.wav",wet);writeWav(out/"sidechain.wav",side);
        std::ofstream info(out/"capture.json");info<<"{\n  \"schema\": \"skell.demo.capture.v1\",\n  \"product\": \"SkellCore Live Multiband Ducker\",\n  \"revision\": \""<<rev<<"\",\n  \"origin\": \"offline_core\",\n  \"stimulus\": \"synthetic three-tone and pulsing sidechain\",\n  \"sample_rate\": "<<rate<<",\n  \"frames\": "<<frames<<",\n  \"engine_latency_samples\": "<<engine.latencySamples()<<",\n  \"parameter\": \"amount\",\n  \"meter\": \"LiveMultibandDucker.copyDisplayBands channel 0\"\n}\n";
        if(!info)throw std::runtime_error("Metadata write failed");
        std::cout<<"Captured "<<frames<<" samples, "<<frames/block<<" real DSP blocks\n";
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
