#pragma once
// Independent opt-in fixed-control spectral/dynamics study. No factory defaults changed.
// Configure off callback; no allocation or lookahead. Detector operates on complete
// previous 5 ms blocks and interpolates control over the next block.
#include <algorithm>
#include <array>
#include <cmath>
namespace skellcore::vocal {
namespace calibrated_detail {
inline bool in(double x,double lo,double hi) noexcept {return std::isfinite(x)&&x>=lo&&x<=hi;}
inline float safe(float x) noexcept {return std::isfinite(x)?std::clamp(x,-16.f,16.f):0.f;}
struct HighPass {
    double b0=1,b1=0,b2=0,a1=0,a2=0;std::array<double,2> z1{},z2{};
    void prepare(double sr,double hz,bool second) noexcept {
        const double w=6.2831853071795864769*hz/sr;
        if(second){const double c=std::cos(w),a=std::sin(w)/std::sqrt(2.),d=1+a;b0=(1+c)/(2*d);b1=-2*b0;b2=b0;a1=-2*c/d;a2=(1-a)/d;}
        else {const double g=std::tan(w*.5);b0=1/(1+g);b1=-b0;b2=0;a1=(g-1)/(g+1);a2=0;}
        reset();
    }
    void reset() noexcept {z1={};z2={};}
    float tick(float x,int c) noexcept {const double y=b0*x+z1[c];z1[c]=b1*x-a1*y+z2[c];z2[c]=b2*x-a2*y;if(std::abs(z1[c])<1e-30)z1[c]=0;if(std::abs(z2[c])<1e-30)z2[c]=0;return float(y);}
};
struct Detector {
    int hop=1,pos=0;double threshold=-30,ratio=1,cap=0,attack=0,release=0,previous=0,next=0,peak=0;
    void prepare(double sr,double th,double ra,double at,double re,double ca) noexcept {hop=std::max(1,int(std::nearbyint(sr*.005)));threshold=th;ratio=ra;cap=ca;attack=std::exp(-hop/(sr*at*.001));release=std::exp(-hop/(sr*re*.001));reset();}
    void reset() noexcept {pos=0;previous=next=peak=0;}
    double tick(float l,float r) noexcept {
        const double gain=std::pow(10.,-(previous+(next-previous)*double(pos)/hop)/20);
        peak=std::max(peak,double(std::max(std::abs(l),std::abs(r))));
        if(++pos==hop){const double target=std::clamp((20*std::log10(std::max(peak,1e-12))-threshold)*(1-1/ratio),0.,cap);previous=next;const double c=target>next?attack:release;next=target+(next-target)*c;pos=0;peak=0;}
        return gain;
    }
};
}
struct AirBandSettings {double midHz=3000,highHz=9000,thresholdDb=-30,ratio=2,releaseMs=90,dry=1,mid=0,high=0;};
class CalibratedAirBands {
public:
    bool prepare(double sr,const AirBandSettings& s) noexcept {using calibrated_detail::in;
        if(!in(sr,32000,192000)||!in(s.midHz,1000,6000)||!in(s.highHz,6000,std::min(14000.,sr*.45))||s.highHz<=s.midHz||!in(s.thresholdDb,-60,-10)||!in(s.ratio,1,8)||!in(s.releaseMs,5,500)||!in(s.dry,.7,1.4)||!in(s.mid,0,3)||!in(s.high,0,4))return false;
        settings=s;m.prepare(sr,s.midHz,true);h.prepare(sr,s.highHz,true);md.prepare(sr,s.thresholdDb,s.ratio,2,s.releaseMs,30);hd.prepare(sr,s.thresholdDb-7,s.ratio,2,s.releaseMs,30);return true;}
    void reset() noexcept {m.reset();h.reset();md.reset();hd.reset();}
    std::array<float,2> process(float l,float r) noexcept {using calibrated_detail::safe;l=safe(l);r=safe(r);const float ml=m.tick(l,0),mr=m.tick(r,1),hl=h.tick(l,0),hr=h.tick(r,1);const double gm=md.tick(ml,mr)*settings.mid,gh=hd.tick(hl,hr)*settings.high;return {float(settings.dry*l+gm*ml+gh*hl),float(settings.dry*r+gm*mr+gh*hr)};}
    static constexpr int latencySamples=0;
private: AirBandSettings settings{};calibrated_detail::HighPass m,h;calibrated_detail::Detector md,hd;
};
struct DeEssBandSettings {double hz=5500,thresholdDb=-29,ratio=4,attackMs=.5,releaseMs=65,rangeDb=12;bool wide=false;};
class CalibratedDeEssBand {
public:
    bool prepare(double sr,const DeEssBandSettings& s) noexcept {using calibrated_detail::in;
        if(!in(sr,32000,192000)||!in(s.hz,2000,std::min(16000.,sr*.45))||!in(s.thresholdDb,-80,0)||!in(s.ratio,1,20)||!in(s.attackMs,.1,50)||!in(s.releaseMs,5,500)||!in(s.rangeDb,0,24))return false;
        settings=s;h.prepare(sr,s.hz,false);d.prepare(sr,s.thresholdDb,s.ratio,s.attackMs,s.releaseMs,s.rangeDb);return true;}
    void reset() noexcept {h.reset();d.reset();}
    std::array<float,2> process(float l,float r) noexcept {using calibrated_detail::safe;l=safe(l);r=safe(r);const float hl=h.tick(l,0),hr=h.tick(r,1);const double g=d.tick(hl,hr)-1;return {float(l+g*(settings.wide?l:hl)),float(r+g*(settings.wide?r:hr))};}
    static constexpr int latencySamples=0;
private: DeEssBandSettings settings{};calibrated_detail::HighPass h;calibrated_detail::Detector d;
};
} // namespace skellcore::vocal
