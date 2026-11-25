#pragma once
// Independent Skell color research: 2x/4x FIR oversampling of our original transfer.
// No external DSP code or vendor calibration. Fixed latency, prepare off thread.
#include "IndependentEngines.h"
#include <array>

namespace skellcore::experimental::spectral {
template<int Factor> class ColorOversampledFixed {
    static_assert(Factor==2 || Factor==4,"Choose 2x or 4x");
public:
    static constexpr int factor=Factor, taps=40*factor+1, history=(taps-1)/factor+1;
    static constexpr int latency=(taps-1)/factor;
private:
    std::array<double,taps> coeff{};
    std::array<Frame,history> input{};
    std::array<Frame,taps> output{};
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
        // Blackman-windowed sinc. Cutoff 0.45 cycles/base-rate sample.
        const double fc=.45/factor;double sum=0;
        for(int i=0;i<taps;++i){
            const int d=i-(taps-1)/2;
            const double sinc=d?std::sin(2*pi*fc*d)/(pi*d):2*fc;
            const double w=.42-.5*std::cos(2*pi*i/(taps-1))+.08*std::cos(4*pi*i/(taps-1));
            coeff[i]=sinc*w;sum+=coeff[i];
        }
        for(auto &v:coeff)v/=sum;
        reset();
    }
    void reset() noexcept {input.fill({0,0});output.fill({0,0});dry.fill({0,0});inPos=outPos=dryPos=0;for(int c=0;c<2;++c)prevIn[c]=prevOut[c]=0;}
    Frame process(Frame x) noexcept {
        x={finite(x.l),finite(x.r)};
        const Frame delayed=dry[dryPos];dry[dryPos]=x; if(++dryPos==latency)dryPos=0;
        input[inPos]=x;
        Frame wet{0,0};
        for(int phase=0;phase<factor;++phase){
            Frame up{0,0};int ix=inPos;
            for(int k=phase;k<taps;k+=factor){
                const double h=coeff[k]*factor;up.l+=h*input[ix].l;up.r+=h*input[ix].r;
                if(--ix<0)ix=history-1;
            }
            output[outPos]={shape(up.l,0),shape(up.r,1)};
            // Decimate at phase zero: both FIRs total exactly 40 base-rate samples.
            if(phase==0){int j=outPos;for(int k=0;k<taps;++k){wet.l+=coeff[k]*output[j].l;wet.r+=coeff[k]*output[j].r;if(--j<0)j=taps-1;}}
            if(++outPos==taps)outPos=0;
        }
        if(++inPos==history)inPos=0;
        return {delayed.l*(1-mix)+wet.l*mix*makeup,delayed.r*(1-mix)+wet.r*mix*makeup};
    }
};
using ColorOversampled2x=ColorOversampledFixed<2>;
using ColorOversampled=ColorOversampledFixed<4>;
} // namespace skellcore::experimental::spectral
