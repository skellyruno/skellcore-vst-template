#pragma once
// Independent compressor experiment: reduction exposure controls recovery.
// Not a recovered vendor model. All time constants are original research choices.
#include "OriginalDynamics.h"
namespace skellcore::experimental::dynamics {
class HistoryCompressor {
    double threshold, slope, width, exposureAmount, attack, releaseShort, releaseLong;
    double detectorPole, exposureRise, exposureFall, det=0, history=0, reduction=0;
public:
    HistoryCompressor(double fs,double thresholdDb=-18,double ratio=4,double kneeDb=6,double amount=1) {
        validRate(fs);
        if(!std::isfinite(thresholdDb)||thresholdDb<-90||thresholdDb>0||!std::isfinite(ratio)||ratio<1||ratio>30||
           !std::isfinite(kneeDb)||kneeDb<0||kneeDb>24||!std::isfinite(amount)||amount<0||amount>1)
            throw std::invalid_argument("history compressor settings");
        threshold=thresholdDb;slope=1-1/ratio;width=kneeDb;exposureAmount=amount;
        detectorPole=decay(fs,1);attack=decay(fs,4);releaseShort=decay(fs,65);releaseLong=decay(fs,400);
        exposureRise=decay(fs,250);exposureFall=decay(fs,800);
    }
    void reset() noexcept {det=history=reduction=0;}
    double reductionDb()const noexcept{return reduction;}
    double exposure()const noexcept{return history;}
    Frame process(Frame x) noexcept {
        for(auto& s:x)s=clean(s);
        const double p=peak(x);
        det=detectorPole*det+(1-detectorPole)*p*p;
        if(det<1e-24)det=0;
        const double over=level(std::sqrt(det))-threshold;
        double target=0;
        if(width==0 || over>=width*.5)target=slope*std::max(0.,over);
        else if(over> -width*.5){const double d=over+width*.5;target=slope*d*d/(2*width);}
        // Accumulate actual demanded reduction, not silence or a fast/slow ratio.
        const double stress=std::clamp(target/12.,0.,1.);
        const double h=stress>history?exposureRise:exposureFall;
        history=h*history+(1-h)*stress;
        if(history<1e-12)history=0;
        const double release=releaseShort+exposureAmount*history*(releaseLong-releaseShort);
        const double a=target>reduction?attack:release;
        reduction=a*reduction+(1-a)*target;
        if(reduction<1e-12)reduction=0;
        if(reduction==0)return x;
        const double g=db(-reduction);
        for(auto& s:x)s*=g;
        return x;
    }
};
}
