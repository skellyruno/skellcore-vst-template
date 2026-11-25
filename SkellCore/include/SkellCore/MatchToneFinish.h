#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace skellcore::dsp {
// Fixed-recipe, causal four-band tone / linked dynamics diagnostic stage.
// No generated side signal; no target waveform input, allocation or lookahead.
class MatchToneFinish final {
public:
    struct Frame { float left=0.f,right=0.f; };
    struct Parameters { std::array<float,4> bandDb{}; float sideDb=0.f, gainDb=0.f, slope=0.f; };
    bool prepare(double rate, Parameters p) noexcept {
        if(!std::isfinite(rate)||rate<16000.||rate>192000.||!valid(p.sideDb,-9,9)||!valid(p.gainDb,-12,12)||!valid(p.slope,-.5f,.5f)) return false;
        for(float d:p.bandDb) if(!valid(d,-6,6)) return false;
        constexpr double freq[3]={200.,2200.,6500.};
        for(int i=0;i<3;++i) alpha[i]=float(1.-std::exp(-6.283185307179586*freq[i]/rate));
        for(int i=0;i<4;++i) band[i]=db(p.bandDb[i]);
        side=db(p.sideDb); gain=db(p.gainDb); slope=p.slope;
        attack=float(1.-std::exp(-1./(.015*rate))); release=float(1.-std::exp(-1./(.12*rate)));
        reset();return true;
    }
    void reset() noexcept { low={};env=0.f; }
    Frame process(Frame in) noexcept {
        const float l=clean(in.left),r=clean(in.right);
        const float m=.5f*l+.5f*r,s=.5f*l-.5f*r;
        const float a=std::max(std::abs(l),std::abs(r));
        env=clean(env+(a-env)*(a>env?attack:release));
        // bounded, source-only level-dependent gain, pivot at -24 dBFS.
        const float dyn=slope==0.f?1.f:db(std::clamp(slope*20.f*std::log10(std::max(env,1.e-5f)/.063095734f),-6.f,6.f));
        const float om=shape(m,0)*gain*dyn,os=shape(s,1)*side*gain*dyn;
        return {clean(om+os),clean(om-os)};
    }
    static constexpr int latencySamples() noexcept { return 0; }
private:
    static bool valid(float v,float lo,float hi) noexcept { return std::isfinite(v)&&v>=lo&&v<=hi; }
    static float db(float d) noexcept {return std::pow(10.f,d*.05f);}
    static float clean(float x) noexcept {return std::isfinite(x)&&std::abs(x)>=1.e-20f?x:0.f;}
    float shape(float x,int ch) noexcept {
        auto& v=low[ch];for(int i=0;i<3;++i) v[i]=clean(v[i]+alpha[i]*(x-v[i]));
        // Difference formulation keeps unity recipe transparent.
        return x*band[3]+v[2]*(band[2]-band[3])+v[1]*(band[1]-band[2])+v[0]*(band[0]-band[1]);
    }
    std::array<std::array<float,3>,2> low{}; std::array<float,3> alpha{}; std::array<float,4> band{{1,1,1,1}};
    float side=1.f,gain=1.f,slope=0.f,env=0.f,attack=0.f,release=0.f;
};
}
