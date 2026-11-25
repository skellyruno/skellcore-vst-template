#include <SkellCore/MatchStereoDelay.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
using skellcore::dsp::MatchStereoDelay;
// Native-endian interleaved IEEE float32 stereo. Reference audio is never accepted.
int main(int argc, char** argv) {
    try {
        static_assert(sizeof(float)==4 && std::numeric_limits<float>::is_iec559);
        if (argc==2 && std::string(argv[1])=="--version") { std::cout<<"match-stereo-delay-v1\n"; return 0; }
        if (argc==2 && std::string(argv[1])=="--capabilities") { std::cout<<"match-stereo-delay-v2: optional duckDepth duckThreshold outputGain duckAttackMs duckReleaseMs\n"; return 0; }
        if (argc!=12 && argc!=17) throw std::runtime_error("usage: render input.f32 output.f32 rate delayMs cutoffHz feedback cross wet midGain sideGain block [duckDepth duckThreshold outputGain duckAttackMs duckReleaseMs]");
        auto parse=[&](int i,double lo,double hi) { std::size_t n=0; std::string s=argv[i]; double v=std::stod(s,&n); if(n!=s.size() || !std::isfinite(v) || v<lo || v>hi) throw std::runtime_error("invalid parameter"); return v; };
        const double rate=parse(3,8000.,192000.), block=parse(11,1.,8192.);
        if(rate!=std::floor(rate)||block!=std::floor(block)) throw std::runtime_error("integer rate/block required");
        MatchStereoDelay::Parameters p{parse(4,1.,2000.),parse(5,20.,rate*.5),float(parse(6,-.95,.95)),float(parse(7,0.,1.)),float(parse(8,0.,2.)),float(parse(9,0.,4.)),float(parse(10,0.,4.))};
        if(argc==17) {
            p.duckDepth=float(parse(12,0.,1.)); p.duckThreshold=float(parse(13,.0001,1.));
            p.outputGain=float(parse(14,.01,4.)); p.duckAttackMs=parse(15,1.,200.); p.duckReleaseMs=parse(16,10.,2000.);
        }
        if(std::filesystem::exists(argv[2]) || std::filesystem::is_symlink(argv[2])) throw std::runtime_error("output already exists");
        std::ifstream src(argv[1],std::ios::binary|std::ios::ate); if(!src) throw std::runtime_error("input open failed");
        const auto bytes=src.tellg(); if(bytes<=0 || bytes%8 || bytes>std::streamoff(192000)*8*600) throw std::runtime_error("bad input length");
        std::vector<float> x(static_cast<std::size_t>(bytes)/4); src.seekg(0); src.read(reinterpret_cast<char*>(x.data()),bytes); if(!src) throw std::runtime_error("input read failed");
        for(float v:x) if(!std::isfinite(v)) throw std::runtime_error("nonfinite input");
        MatchStereoDelay d; if(!d.prepare(rate,p)) throw std::runtime_error("invalid delay recipe");
        const auto start=std::chrono::steady_clock::now();
        const auto frames=x.size()/2, bl=static_cast<std::size_t>(block);
        for(std::size_t b=0;b<frames;b+=bl) for(std::size_t i=b;i<std::min(frames,b+bl);++i) { const auto o=d.process({x[2*i],x[2*i+1]}); x[2*i]=o.left; x[2*i+1]=o.right; }
        const auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
        for(float v:x) if(!std::isfinite(v)) throw std::runtime_error("nonfinite output");
        std::ofstream dst(argv[2],std::ios::binary|std::ios::trunc); if(!dst) throw std::runtime_error("output open failed");
        dst.write(reinterpret_cast<const char*>(x.data()),bytes); dst.close(); if(!dst) throw std::runtime_error("output write failed");
        std::cout<<"{\"frames\":"<<frames<<",\"processing_seconds\":"<<seconds<<",\"latency_samples\":"<<d.latencySamples()<<"}\n"; return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
