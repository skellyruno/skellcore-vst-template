#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>
namespace skellcore::dsp {
// Causal fractional-delay chorus/flanger. Distinct time-varying stereo taps;
// not a channel polarity trick. Frozen recipe, prepare/reset off audio thread.
class MatchModulation final {
public:
    struct Frame {float left=0.f,right=0.f;};
    struct Parameters {double centerMs=8.,depthMs=2.,rateHz=.7,phaseRadians=1.5707963267948966;float feedback=0.f,wet=.3f,dry=1.f;};
    bool prepare(double sr,Parameters p) {
        if(!valid(sr,16000.,192000.)||!valid(p.centerMs,.2,40.)||!valid(p.depthMs,0.,20.)||p.centerMs-p.depthMs<1000./sr||!valid(p.rateHz,.05,10.)||!valid(p.phaseRadians,0.,3.14159265358979323846)||!valid(p.feedback,-.7,.7)||!valid(p.wet,0.,2.)||!valid(p.dry,0.,2.))return false;
        const auto n=static_cast<std::size_t>(std::ceil((p.centerMs+p.depthMs)*sr*.001))+2;
        std::vector<Frame> next(n);buffer.swap(next);params=p;center=p.centerMs*sr*.001;depth=p.depthMs*sr*.001;
        const double w=6.283185307179586*p.rateHz/sr;stepSin=std::sin(w);stepCos=std::cos(w);phaseSin=std::sin(p.phaseRadians);phaseCos=std::cos(p.phaseRadians);reset();return true;
    }
    void reset() noexcept {std::fill(buffer.begin(),buffer.end(),Frame{});pos=0;oscSin=0.;oscCos=1.;ticks=0;}
    Frame process(Frame in) noexcept {
        in={clean(in.left),clean(in.right)};if(buffer.empty())return in;
        const float l=read(center+depth*oscSin,false),r=read(center+depth*(oscSin*phaseCos+oscCos*phaseSin),true);
        buffer[pos]={clean(in.left+params.feedback*l),clean(in.right+params.feedback*r)};
        if(++pos==buffer.size())pos=0;
        const double s=oscSin*stepCos+oscCos*stepSin;oscCos=oscCos*stepCos-oscSin*stepSin;oscSin=s;
        if(++ticks==4096){const double inv=1./std::sqrt(oscSin*oscSin+oscCos*oscCos);oscSin*=inv;oscCos*=inv;ticks=0;}
        return {clean(params.dry*in.left+params.wet*l),clean(params.dry*in.right+params.wet*r)};
    }
    static constexpr int latencySamples() noexcept{return 0;}
private:
    static bool valid(double x,double lo,double hi) noexcept{return std::isfinite(x)&&x>=lo&&x<=hi;}
    static float clean(float x) noexcept{return std::isfinite(x)&&std::abs(x)>=1.e-20f?x:0.f;}
    float read(double delay,bool right) const noexcept {
        delay=std::clamp(delay,1.,static_cast<double>(buffer.size()-2));const auto i=static_cast<std::size_t>(delay);const float frac=static_cast<float>(delay-i);
        const auto a=(pos+buffer.size()-i)%buffer.size(),b=(a+buffer.size()-1)%buffer.size();const float u=right?buffer[a].right:buffer[a].left,v=right?buffer[b].right:buffer[b].left;return u+frac*(v-u);
    }
    std::vector<Frame> buffer;std::size_t pos=0;unsigned ticks=0;Parameters params{};
    double center=0.,depth=0.,oscSin=0.,oscCos=1.,stepSin=0.,stepCos=1.,phaseSin=0.,phaseCos=1.;
};
}
