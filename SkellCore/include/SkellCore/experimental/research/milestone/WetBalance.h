#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace skell_balance {
using Frame=std::array<double,2>;
// Independent wet-level control. Dry path is never compressed. A linked
// nonnegative wet gain is restricted to the feasible sample-headroom interval.
// This is not an intersample peak limiter. If dry alone exceeds ceiling, the
// wet path is muted; the dry overload is reported, not silently clipped.
class WetBalance {
    double fs=48000.,energyStep=0.,gainStep=0.,releaseStep=0.;
    double inputEnergy=0.,wetEnergy=0.,adaptiveGain=1.,headGain=1.;
    double mix=0.,targetMix=0.,ceiling=1.,maxGain=2.,lastGain=1.;
    bool enabled=false,dryOverload=false;
    static double safe(double v) noexcept {return std::isfinite(v)?std::clamp(v,-64.,64.):0.;}
    static double bound(double x,double lo,double hi,double f) noexcept {return std::isfinite(x)?std::clamp(x,lo,hi):f;}
    static double energy(Frame x) noexcept {return .5*(x[0]*x[0]+x[1]*x[1]);}
public:
    void prepare(double sampleRate,double ceilingDb=0.,double maximumBoostDb=9.) noexcept {
        fs=bound(sampleRate,8000.,384000.,48000.);ceiling=std::pow(10.,bound(ceilingDb,-12.,0.,0.)/20.);
        maxGain=std::pow(10.,bound(maximumBoostDb,0.,12.,9.)/20.);
        energyStep=-std::expm1(-1./(.125*fs));gainStep=-std::expm1(-1./(.080*fs));releaseStep=-std::expm1(-1./(.025*fs));reset();
    }
    void setMix(double amount) noexcept {targetMix=bound(amount,0.,1.,0.);}
    void setEnabled(bool on) noexcept {enabled=on;}
    void reset() noexcept {inputEnergy=wetEnergy=0.;adaptiveGain=headGain=lastGain=1.;mix=targetMix;dryOverload=false;}
    double gain() const noexcept{return lastGain;}
    bool dryExceeded() const noexcept{return dryOverload;}
    Frame process(Frame dry,Frame wet) noexcept {
        for(int c=0;c<2;++c){dry[c]=safe(dry[c]);wet[c]=safe(wet[c]);}
        const double step=100./fs;mix=targetMix>mix?std::min(targetMix,mix+step):std::max(targetMix,mix-step);
        // Track both signals even when the correction is off, avoiding cold start
        // when enabled. Estimate energy before the user mix, not after it.
        inputEnergy+=energyStep*(energy(dry)-inputEnergy);wetEnergy+=energyStep*(energy(wet)-wetEnergy);
        if(inputEnergy<1e-24)inputEnergy=0.;
        if(wetEnergy<1e-24)wetEnergy=0.;
        // Freeze near silence instead of boosting numerical noise or wet tails.
        double desired=1.;if(inputEnergy>1e-8&&wetEnergy>1e-8)desired=std::clamp(std::sqrt(inputEnergy/wetEnergy),.5,maxGain);
        adaptiveGain+=gainStep*(desired-adaptiveGain);
        if(!enabled){lastGain=1.;headGain=1.;dryOverload=false;return {(1.-mix)*dry[0]+mix*wet[0],(1.-mix)*dry[1]+mix*wet[1]};}
        Frame d{(1.-mix)*dry[0],(1.-mix)*dry[1]},w{mix*wet[0],mix*wet[1]};
        dryOverload=std::max(std::abs(d[0]),std::abs(d[1]))>ceiling;
        double allowed=adaptiveGain;
        if(dryOverload)allowed=0.;else for(int c=0;c<2;++c){
            if(w[c]>0.)allowed=std::min(allowed,(ceiling-d[c])/w[c]);
            else if(w[c]<0.)allowed=std::min(allowed,(-ceiling-d[c])/w[c]);
        }
        allowed=std::max(0.,allowed);
        headGain=allowed<headGain?allowed:std::min(allowed,headGain+releaseStep*(allowed-headGain));
        lastGain=headGain;
        if(mix==0.)return dry;
        return {d[0]+w[0]*headGain,d[1]+w[1]*headGain};
    }
};
}
