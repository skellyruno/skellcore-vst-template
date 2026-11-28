#pragma once
// Original Skell DSP research prototypes. No vendor code, tables, or state formats.
// Configure/prepare off the audio thread; process() is allocation/lock/free of I/O.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>

namespace skellcore::experimental::spectral {
constexpr double pi = 3.1415926535897932384626433832795;

inline double finite(double x) noexcept { return std::isfinite(x) ? x : 0.0; }
inline double bound(double x, double lo, double hi, double fallback) noexcept {
    return std::clamp(std::isfinite(x) ? x : fallback, lo, hi);
}
inline double pole(double ms, double fs) noexcept {
    return std::exp(-1.0 / (std::max(0.01, ms) * 0.001 * fs));
}
inline double fromDb(double db) noexcept { return std::pow(10.0, db / 20.0); }
struct Frame { double l, r; };

// Transposed direct-form-II filter, with coefficients from a public Butterworth design.
struct Biquad {
    double b0=1, b1=0, b2=0, a1=0, a2=0, z1=0, z2=0;
    void setButterworth(double fs, double frequency, bool highpass) noexcept {
        const double g = std::tan(pi * std::min(frequency, 0.45*fs) / fs);
        const double d = 1.0 / (1.0 + std::sqrt(2.0)*g + g*g);
        const double n = highpass ? 1.0 : g*g;
        b0=n*d; b1=(highpass ? -2.0 : 2.0)*b0; b2=b0;
        a1=2.0*(g*g-1.0)*d; a2=(1.0-std::sqrt(2.0)*g+g*g)*d;
    }
    void reset() noexcept { z1=z2=0.0; }
    double process(double input) noexcept {
        const double y=b0*input+z1;
        z1=b1*input-a1*y+z2;
        z2=b2*input-a2*y;
        if (std::abs(z1)<1e-30) z1=0.0;
        if (std::abs(z2)<1e-30) z2=0.0;
        return y;
    }
};

// Standalone independent two-band dynamic presence design; LR4 magnitude-flat split.
// No explicit lookahead or delay buffer; all-pass group delay is not host latency.
class PresenceEngine {
    Biquad low[2][2], high[2][2];
    double env=0.0, g=1.0, rise=0.0, fall=0.0, gainPole=0.0;
    double boost=1.0, trigger=0.1, strength=0.0, wet=0.0;
public:
    void prepare(double sampleRate, double cutoffHz=2600, double boostDb=3,
                 double thresholdDb=-22, double amount=0.7, double wetMix=1) noexcept {
        const double fs=bound(sampleRate, 8000, 384000, 48000);
        const double cutoff=bound(cutoffHz, 80, fs*0.40, 2600);
        for (int c=0;c<2;++c) for(int i=0;i<2;++i) {
            low[c][i].setButterworth(fs,cutoff,false);
            high[c][i].setButterworth(fs,cutoff,true);
        }
        boost=fromDb(bound(boostDb,-12,12,0));
        trigger=fromDb(bound(thresholdDb,-72,0,-22));
        strength=bound(amount,0,2,0.7); wet=bound(wetMix,0,1,1);
        rise=pole(1.5,fs); fall=pole(70.0,fs); gainPole=pole(2.0,fs);
        reset();
    }
    void reset() noexcept {
        for (auto &channel:low) for(auto &b:channel) b.reset();
        for (auto &channel:high) for(auto &b:channel) b.reset();
        env=0.0; g=boost;
    }
    Frame process(Frame sample) noexcept {
        sample.l=finite(sample.l);sample.r=finite(sample.r);
        const double x[2]={sample.l,sample.r}; double lo[2],hi[2];
        for(int c=0;c<2;++c) {
            lo[c]=low[c][1].process(low[c][0].process(x[c]));
            hi[c]=high[c][1].process(high[c][0].process(x[c]));
        }
        const double peak=std::max(std::abs(hi[0]),std::abs(hi[1]));
        env=(peak>env ? rise : fall)*env+(1.0-(peak>env ? rise : fall))*peak;
        const double excess=std::max(0.0,env/trigger-1.0);
        const double target=boost/(1.0+strength*excess);
        g=gainPole*g+(1.0-gainPole)*target;
        return {(1.0-wet)*sample.l+wet*(lo[0]+hi[0]*g),
                (1.0-wet)*sample.r+wet*(lo[1]+hi[1]*g)};
    }
};

// Independently designed stereo-linked digital leveler with program-dependent
// envelope release (two fixed poles blended by an independent activity memory).
class ProgramLeveler {
    double attack=0.0, releaseFast=0.0, releaseSlow=0.0, memoryPole=0.0;
    double grAttack=0.0, grRelease=0.0, threshold=0.2, exponent=0.5;
    double makeup=1.0, wet=1.0, env=0.0, memory=0.0, gain=1.0;
public:
    void prepare(double sampleRate,double thresholdDb=-18,double ratio=3,
                 double attackMs=4,double releaseMs=120,double makeupDb=0,double mix=1) noexcept {
        const double fs=bound(sampleRate,8000,384000,48000);
        threshold=fromDb(bound(thresholdDb,-72,0,-18));
        exponent=1.0-1.0/bound(ratio,1,20,3);
        makeup=fromDb(bound(makeupDb,-18,18,0));
        wet=bound(mix,0,1,1);
        const double a=bound(attackMs,0.1,100,4), r=bound(releaseMs,5,3000,120);
        attack=pole(a,fs); releaseFast=pole(r,fs); releaseSlow=pole(3*r,fs);
        memoryPole=pole(2*r,fs);
        grAttack=pole(a,fs); grRelease=pole(r,fs);
        reset();
    }
    void reset() noexcept { env=memory=0; gain=1; }
    Frame process(Frame x) noexcept {
        x.l=finite(x.l);x.r=finite(x.r);
        const double p=std::max(std::abs(x.l),std::abs(x.r));
        memory=memoryPole*memory+(1.0-memoryPole)*p;
        const double activity=memory/(memory+threshold+1e-18);
        const double fall=releaseFast*(1.0-activity)+releaseSlow*activity;
        const double coeff=p>env ? attack : fall;
        env=coeff*env+(1.0-coeff)*p;
        const double target=(env>threshold && exponent>0) ? std::pow(threshold/env,exponent) : 1.0;
        const double smooth=target<gain ? grAttack : grRelease;
        gain=smooth*gain+(1.0-smooth)*target;
        const double g=makeup*gain;
        return {x.l*(1.0-wet+wet*g),x.r*(1.0-wet+wet*g)};
    }
};

// Asymmetric smooth distortion, with an independently developed one-pole DC
// blocker. The current research prototype intentionally DOES NOT oversample.
class ColorEngine {
    double drive=1, bias=0, wet=0, output=1, norm=1, dcPole=0.997;
    double prevIn[2]{}, prevOut[2]{};
    double channel(double x,int c) noexcept {
        const double a=std::clamp(x,-8.0,8.0);
        const double shaped=std::tanh(drive*(a+bias*a*a))*norm;
        const double y=shaped-prevIn[c]+dcPole*prevOut[c];
        prevIn[c]=shaped; prevOut[c]=std::abs(y)<1e-30 ? 0.0 : y;
        return (1.0-wet)*x+wet*output*prevOut[c];
    }
public:
    void prepare(double sampleRate,double driveDb=12,double asymmetry=0.12,
                 double mix=0.6,double outputDb=0) noexcept {
        const double fs=bound(sampleRate,8000,384000,48000);
        drive=fromDb(bound(driveDb,0,24,12));
        bias=bound(asymmetry,-0.3,0.3,0.12);
        wet=bound(mix,0,1,0.6);
        output=fromDb(bound(outputDb,-18,18,0));
        norm=1.0/std::max(1e-6,std::tanh(drive));
        dcPole=std::exp(-2*pi*5.0/fs);
        reset();
    }
    void reset() noexcept { for(int i=0;i<2;++i) prevIn[i]=prevOut[i]=0.0; }
    Frame process(Frame x) noexcept { x.l=finite(x.l);x.r=finite(x.r);return {channel(x.l,0),channel(x.r,1)}; }
};
} // namespace skellcore::experimental::spectral
