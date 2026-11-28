#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
namespace skell::research {
// Independent experimental delay. Single audio-thread owner; setTarget on that thread.
class StatefulStereoMod {
public:
    struct Controls { double center=7.4, depth=3., rate=.91, spread=1.57, feedback=.35, cross=.5, drive=2., mix=.5; };
    static constexpr unsigned capacity=8192;
    bool prepare(double fs) noexcept {
        if (!std::isfinite(fs)||fs<8000||fs>192000) return false;
        rate_=fs; alpha_=1.-std::exp(-1./(.01*fs)); reset(); return true;
    }
    void reset() noexcept { for(auto& c:history_) c.fill(0.); head_=0; phase_=0.; current_=target_; }
    bool setTarget(const Controls& p) noexcept {
        const double vals[]{p.center,p.depth,p.rate,p.spread,p.feedback,p.cross,p.drive,p.mix};
        for(auto v:vals) if(!std::isfinite(v))return false;
        if(p.center<1||p.center>25||p.depth<0||p.center-p.depth<.25||p.center+p.depth>40||p.rate<.05||p.rate>5||p.spread<0||p.spread>3.141592653589793||std::abs(p.feedback)>.92||p.cross<0||p.cross>1||p.drive<.25||p.drive>8||p.mix<0||p.mix>1)return false;
        target_=p;return true;
    }
    std::array<double,2> process(double l,double r) noexcept {
        auto smooth=[this](double& a,double b){a+=alpha_*(b-a);};
        smooth(current_.center,target_.center);smooth(current_.depth,target_.depth);smooth(current_.rate,target_.rate);smooth(current_.spread,target_.spread);smooth(current_.feedback,target_.feedback);smooth(current_.cross,target_.cross);smooth(current_.drive,target_.drive);smooth(current_.mix,target_.mix);
        constexpr double tau=6.2831853071795864769;
        const double x[]{safe(l),safe(r)};
        double wet[2]{};
        for(unsigned c=0;c<2;++c){
            const double d=(current_.center+current_.depth*std::sin(phase_+c*current_.spread))*rate_*.001;
            const auto k=static_cast<unsigned>(d);const double f=d-k;
            const auto a=(head_+capacity-k)%capacity,b=(a+capacity-1)%capacity;
            wet[c]=(1.-f)*history_[c][a]+f*history_[c][b];
        }
        for(unsigned c=0;c<2;++c){
            const double v=(1.-current_.cross)*wet[c]+current_.cross*wet[1-c];
            // Convex crossfeed and unity-slope bounded color; gain magnitude <= .92.
            history_[c][head_]=safe(x[c]+current_.feedback*std::tanh(current_.drive*v)/current_.drive);
        }
        head_=(head_+1)%capacity;phase_+=tau*current_.rate/rate_;if(phase_>=tau)phase_-=tau;
        return {(1.-current_.mix)*x[0]+current_.mix*wet[0],(1.-current_.mix)*x[1]+current_.mix*wet[1]};
    }
private:
    static double safe(double v) noexcept {return std::isfinite(v)&&std::abs(v)>=1e-25?std::clamp(v,-8.,8.):0.;}
    double rate_=48000.,alpha_=1.-std::exp(-1./480.),phase_=0.; unsigned head_=0;
    Controls current_{},target_{};std::array<std::array<double,capacity>,2> history_{};
};
}
