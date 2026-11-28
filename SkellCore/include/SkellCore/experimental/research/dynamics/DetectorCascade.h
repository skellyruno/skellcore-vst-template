#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace skellcore::experimental::dynamics {
// Original amplitude-detector stages. Settings are explicit effective seconds,
// not vendor knob calibration. Prepare/reset off the audio thread.
class DetectorCascade {
public:
    using Frame = std::array<double,2>;
    struct Stage { double thresholdDb=-24., slope=.75, attackSeconds=.001, releaseSeconds=.025, link=0.; };
    bool prepare(double fs, const std::array<Stage,2>& settings, unsigned stages) noexcept {
        if(!std::isfinite(fs)||fs<8000||fs>192000||stages<1||stages>2)return false;
        std::array<Coefficients,2> next{};
        for(unsigned j=0;j<stages;++j){
            const auto& p=settings[j];
            if(!std::isfinite(p.thresholdDb)||p.thresholdDb< -80||p.thresholdDb>0||!std::isfinite(p.slope)||p.slope<0||p.slope>1||!std::isfinite(p.attackSeconds)||p.attackSeconds<.00005||p.attackSeconds>1||!std::isfinite(p.releaseSeconds)||p.releaseSeconds<.00005||p.releaseSeconds>5||!std::isfinite(p.link)||p.link<0||p.link>1)return false;
            next[j]={std::pow(10.,p.thresholdDb/20.),p.slope,std::exp(-1/(fs*p.attackSeconds)),std::exp(-1/(fs*p.releaseSeconds)),p.link};
        }
        coefficients_=next;count_=stages;reset();return true;
    }
    void reset() noexcept {state_={};}
    Frame process(Frame x) noexcept {
        for(auto& v:x)v=std::isfinite(v)?std::clamp(v,-4.,4.):0.;
        for(unsigned j=0;j<count_;++j){
            const auto& p=coefficients_[j];
            const Frame a{std::abs(x[0]),std::abs(x[1])};const double peak=std::max(a[0],a[1]);
            for(unsigned c=0;c<2;++c){
                const double v=a[c]+p.link*(peak-a[c]);double& s=state_[j][c];
                const double pole=v>s?p.attack:p.release;s=v+pole*(s-v);if(s<1e-25)s=0.;
                if(s>p.threshold)x[c]*=std::pow(s/p.threshold,-p.slope);
            }
        }
        return x;
    }
private:
    struct Coefficients {double threshold=1.,slope=0.,attack=0.,release=0.,link=0.;};
    std::array<Coefficients,2> coefficients_{};std::array<Frame,2> state_{};unsigned count_=0;
};
}
