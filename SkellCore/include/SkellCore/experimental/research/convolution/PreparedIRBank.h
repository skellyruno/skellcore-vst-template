#pragma once
#include "NativeStereoIR.h"
#include <memory>

namespace skellcore::experimental::convolution {
using IRFrame=skell::research::Stereo;
using IRTap=skell::research::Tap;
using Impulse=std::vector<IRTap>;
struct IRShape {
    double earlyMs=50., transitionMs=20., earlyGain=1., lateGain=1.;
    double extraDecayDbPerSecond=0., lateLowpassHz=0., width=1.;
};
// Off-thread only. Input and output are LL, LR, RL, RR (output first).
// Decay is additional attenuation, not an inferred RT60. No hidden normalization.
inline Impulse shapeIR(const Impulse& ir,double fs,const IRShape& s={}) {
    const auto valid=[](double x,double lo,double hi){return std::isfinite(x)&&x>=lo&&x<=hi;};
    if(!valid(fs,8000.,192000.)||ir.empty()||ir.size()>1920000||!valid(s.earlyMs,0.,1000.)||!valid(s.transitionMs,1.,200.)||!valid(s.earlyGain,0.,2.)||!valid(s.lateGain,0.,2.)||!valid(s.extraDecayDbPerSecond,0.,120.)||!valid(s.lateLowpassHz,0.,.45*fs)||(s.lateLowpassHz!=0.&&s.lateLowpassHz<20.)||!valid(s.width,0.,2.))throw std::invalid_argument("IR shape");
    for(const auto& t:ir)for(double v:t)if(!valid(v,-64.,64.))throw std::invalid_argument("IR samples");
    if(s.earlyGain==1.&&s.lateGain==1.&&s.extraDecayDbPerSecond==0.&&s.lateLowpassHz==0.&&s.width==1.)return ir;
    Impulse out(ir.size());IRTap low{};
    const double alpha=s.lateLowpassHz==0.?1.:-std::expm1(-6.283185307179586*s.lateLowpassHz/fs);
    for(std::size_t i=0;i<ir.size();++i){
        const double ms=1000.*double(i)/fs;
        const double w=std::clamp((ms-s.earlyMs)/s.transitionMs,0.,1.);
        const double blend=w*w*(3.-2.*w);
        const double g=(s.earlyGain+blend*(s.lateGain-s.earlyGain))*std::exp(-.1151292546497023*s.extraDecayDbPerSecond*double(i)/fs);
        IRTap t{};for(unsigned c=0;c<4;++c){
            if(s.lateLowpassHz==0.)t[c]=g*ir[i][c];
            else {low[c]+=alpha*(ir[i][c]-low[c]);t[c]=g*(ir[i][c]+blend*(low[c]-ir[i][c]));}
        }
        if(s.width==1.)out[i]=t;
        else for(unsigned input=0;input<2;++input){const double mid=.5*(t[input]+t[input+2]),side=.5*(t[input]-t[input+2])*s.width;out[i][input]=mid+side;out[i][input+2]=mid-side;}
        for(double v:out[i])if(!valid(v,-64.,64.))throw std::invalid_argument("shaped IR exceeds bounds");
    }
    return out;
}
// Prepare immutable banks off-thread. All request/tick/status calls share one
// audio-thread owner. No live construction, locks, allocation, or deletion.
// Queue latest request during input fade / old-tail drain. At most two engines
// run concurrently. Tail drain includes a conservative extra 4096 zero frames
// to clear both partition tiers before reusing an idle bank.
class PreparedIRBank {
public:
    PreparedIRBank()=default;
    PreparedIRBank(const PreparedIRBank&)=delete;
    PreparedIRBank& operator=(const PreparedIRBank&)=delete;
    PreparedIRBank(PreparedIRBank&&)=delete;
    PreparedIRBank& operator=(PreparedIRBank&&)=delete;
    static constexpr unsigned bufferingLatency=0, maxBanks=3, none=maxBanks;
    static constexpr std::size_t maxTotalTaps=1920000, drainGuard=4096;
    void prepare(const std::vector<Impulse>& irs,double fs,double fadeMs=20.) {
        if(irs.empty()||irs.size()>maxBanks||!std::isfinite(fs)||fs<8000.||fs>192000.||!std::isfinite(fadeMs)||fadeMs<1.||fadeMs>200.)throw std::invalid_argument("IR bank configuration");
        std::size_t total=0;for(const auto& ir:irs){if(ir.empty()||ir.size()>maxTotalTaps-total)throw std::invalid_argument("IR bank memory budget");total+=ir.size();}
        std::array<std::unique_ptr<skell::research::NativeStereoIR>,maxBanks> next{};std::array<std::size_t,maxBanks> lens{};
        for(unsigned j=0;j<irs.size();++j){next[j]=std::make_unique<skell::research::NativeStereoIR>(irs[j]);lens[j]=irs[j].size();}
        banks_=std::move(next);lengths_=lens;count_=static_cast<unsigned>(irs.size());fadeFrames_=std::max<std::size_t>(1,std::size_t(std::lround(fs*fadeMs*.001)));active_=requested_=0;incoming_=retiring_=none;fadePos_=drain_=0;
    }
    bool request(unsigned index) noexcept {if(index>=count_)return false;requested_=index;return true;}
    unsigned active() const noexcept {return active_;}
    unsigned requested() const noexcept {return requested_;}
    bool busy() const noexcept {return incoming_!=none||retiring_!=none;}
    std::size_t fadeFrames() const noexcept {return fadeFrames_;}
    IRFrame tick(IRFrame x) noexcept {
        if(!count_)return {};
        for(auto& v:x)if(!std::isfinite(v)||std::abs(v)>64.)v=0.;
        if(!busy()&&requested_!=active_){incoming_=requested_;fadePos_=0;}
        if(incoming_!=none){
            const double w=double(++fadePos_)/double(fadeFrames_);
            const auto a=banks_[active_]->tick({x[0]*(1.-w),x[1]*(1.-w)}),b=banks_[incoming_]->tick({x[0]*w,x[1]*w});
            if(fadePos_==fadeFrames_){retiring_=active_;drain_=lengths_[retiring_]+drainGuard;active_=incoming_;incoming_=none;}
            return {a[0]+b[0],a[1]+b[1]};
        }
        auto y=banks_[active_]->tick(x);
        if(retiring_!=none){const auto tail=banks_[retiring_]->tick({});y[0]+=tail[0];y[1]+=tail[1];if(--drain_==0)retiring_=none;}
        return y;
    }
    void process(const IRFrame* in,IRFrame* out,std::size_t n) noexcept {if(!in||!out)return;for(std::size_t i=0;i<n;++i)out[i]=tick(in[i]);}
private:
    std::array<std::unique_ptr<skell::research::NativeStereoIR>,maxBanks> banks_{};std::array<std::size_t,maxBanks> lengths_{};
    unsigned count_=0,active_=none,requested_=none,incoming_=none,retiring_=none;
    std::size_t fadeFrames_=0,fadePos_=0,drain_=0;
};
}
