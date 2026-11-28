#pragma once
// Independent Skell color research: 2x/4x FIR oversampling of our original transfer.
// No external DSP code or vendor calibration. Fixed latency, prepare off thread.
#include "IndependentEngines.h"
#include <array>

namespace skellcore::experimental::spectral {
template<int Factor, int BaseSpan=40, int CutoffMilli=450, int DownCutoffMilli=CutoffMilli> class ColorSplitSymmetric {
    static_assert(Factor==2 || Factor==4,"Choose 2x or 4x");
    static_assert(BaseSpan>=16 && BaseSpan<=128 && BaseSpan%2==0,"Even bounded research span");
    static_assert(CutoffMilli>=400 && CutoffMilli<500,"Cutoff below base Nyquist");
    static_assert(DownCutoffMilli>=400 && DownCutoffMilli<500,"Down cutoff below base Nyquist");
public:
    static constexpr int factor=Factor, taps=BaseSpan*factor+1, history=(taps-1)/factor+1;
    static constexpr int latency=(taps-1)/factor;
private:
    std::array<double,taps> coeff{},downCoeff{};
    std::array<double,(taps-1)/2> paired{};
    std::array<Frame,2*history> input{};
    std::array<Frame,2*taps> output{};
    std::array<Frame,latency> dry{};
    int inPos=0, outPos=0, dryPos=0;
    double drive=1, bias=0, mix=1, makeup=1, norm=1, dcPole=0;
    double prevIn[2]{},prevOut[2]{};
    double shape(double x,int c) noexcept {
        x=std::clamp(x,-8.0,8.0);
        const double s=std::tanh(drive*(x+bias*x*x))*norm;
        const double y=s-prevIn[c]+dcPole*prevOut[c];
        prevIn[c]=s;prevOut[c]=std::abs(y)<1e-30?0.0:y;
        return prevOut[c];
    }
public:
    void prepare(double sampleRate,double driveDb=12,double asymmetry=.12,double wetMix=.6,double outputDb=0) noexcept {
        const double fs=bound(sampleRate,8000,384000,48000);
        drive=fromDb(bound(driveDb,0,24,12));bias=bound(asymmetry,-.3,.3,.12);
        mix=bound(wetMix,0,1,.6);makeup=fromDb(bound(outputDb,-18,18,0));
        norm=1.0/std::max(1e-6,std::tanh(drive));dcPole=std::exp(-2*pi*5/(fs*factor));
        // Blackman-windowed sinc. Default cutoff 0.45 cycles/base-rate sample.
        const double fc=(CutoffMilli/1000.0)/factor;double sum=0;
        for(int i=0;i<taps;++i){
            const int d=i-(taps-1)/2;
            const double sinc=d?std::sin(2*pi*fc*d)/(pi*d):2*fc;
            const double w=.42-.5*std::cos(2*pi*i/(taps-1))+.08*std::cos(4*pi*i/(taps-1));
            coeff[i]=sinc*w;sum+=coeff[i];
        }
        for(auto &v:coeff)v/=sum;
        if constexpr(DownCutoffMilli!=CutoffMilli){
            const double df=(DownCutoffMilli/1000.0)/factor;double total=0;
            for(int i=0;i<taps;++i){const int d=i-(taps-1)/2;const double sinc=d?std::sin(2*pi*df*d)/(pi*d):2*df;const double w=.42-.5*std::cos(2*pi*i/(taps-1))+.08*std::cos(4*pi*i/(taps-1));downCoeff[i]=sinc*w;total+=downCoeff[i];}
            for(auto& v:downCoeff)v/=total;
        }
        for(int k=0;k<(taps-1)/2;++k){auto& c=DownCutoffMilli==CutoffMilli?coeff:downCoeff;paired[k]=.5*(c[k]+c[taps-1-k]);}
        reset();
    }
    void reset() noexcept {input.fill({0,0});output.fill({0,0});dry.fill({0,0});inPos=outPos=dryPos=0;for(int c=0;c<2;++c)prevIn[c]=prevOut[c]=0;}
    Frame process(Frame x) noexcept {
        x={finite(x.l),finite(x.r)};
        const Frame delayed=dry[dryPos];dry[dryPos]=x; if(++dryPos==latency)dryPos=0;
        input[inPos]=input[inPos+history]=x;
        Frame wet{0,0};
        for(int phase=0;phase<factor;++phase){
            Frame up{0,0};int ix=inPos+history;
            for(int k=phase;k<taps;k+=factor){
                const double h=coeff[k]*factor;up.l+=h*input[ix].l;up.r+=h*input[ix].r;
                --ix;
            }
            output[outPos]=output[outPos+taps]={shape(up.l,0),shape(up.r,1)};
            // Decimate at phase zero: both FIRs total exactly BaseSpan base-rate samples.
            if(phase==0){
                const int j=outPos+taps;const auto& c=DownCutoffMilli==CutoffMilli?coeff:downCoeff;
                for(int k=0;k<(taps-1)/2;++k){const double h=paired[k];const auto a=output[j-k],b=output[j-(taps-1-k)];wet.l+=h*(a.l+b.l);wet.r+=h*(a.r+b.r);}
                const int mid=(taps-1)/2;wet.l+=c[mid]*output[j-mid].l;wet.r+=c[mid]*output[j-mid].r;
            }
            if(++outPos==taps)outPos=0;
        }
        if(++inPos==history)inPos=0;
        return {delayed.l*(1-mix)+wet.l*mix*makeup,delayed.r*(1-mix)+wet.r*mix*makeup};
    }
};
} // namespace skellcore::experimental::spectral
