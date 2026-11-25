#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace skellcore::experimental {
// Original parallel five-band residual engine. Dry audio is never replaced by
// a crossover sum. Width acts on side only; color and saturation are separate.
// Intended for an oversampled processing path before a peak-safety limiter.
class FiveBandEnhancer {
public:
    struct Profile {
        std::array<double,4> crossHz{90.,500.,2600.,9300.};
        std::array<double,5> color{.11,.13,.10,.19,.24};
        std::array<double,5> spread{0.,.04,.55,1.05,1.25};
        std::array<double,5> drive{1.1,1.7,2.2,2.6,2.9};
    };
    void prepare(double sampleRate) noexcept { prepare(sampleRate,Profile{}); }
    void prepare(double sampleRate,const Profile& profile) noexcept {
        const double rate=std::isfinite(sampleRate)?std::clamp(sampleRate,44100.,768000.):48000.;
        const auto pole=[rate](double hz){return -std::expm1(-6.283185307179586*hz/rate);};
        double last=20.;
        for(int i=0;i<4;++i) {
            const double hz=std::clamp(clean(profile.crossHz[i],Profile{}.crossHz[i]),last,rate*.4);
            coeff[i]=pole(hz);last=hz;
        }
        for(int i=0;i<5;++i) {
            color[i]=std::clamp(clean(profile.color[i],0.),0.,.5);
            spread[i]=std::clamp(clean(profile.spread[i],0.),0.,1.5);
            drive[i]=std::clamp(clean(profile.drive[i],1.),.1,4.);
        }
        env=-std::expm1(-1./(.050*rate)); slew=-std::expm1(-1./(.025*rate)); dc=pole(35.); bassGuard=pole(210.);
        reset();
    }
    void reset() noexcept { mid={};side={};energy={};bias={};current={};target={};widthLow={};active=false; }
    void setAmounts(double sauce,double width,double saturation) noexcept {
        target={unit(sauce),unit(width),unit(saturation)};
    }
    void process(double& l,double& r) noexcept {
        l=clean(l,0.);r=clean(r,0.);
        for(int i=0;i<3;++i) {
            current[i]+=slew*(target[i]-current[i]);
            if(std::abs(current[i]-target[i])<1.e-9)current[i]=target[i];
        }
        // Clear stale histories after the smoothed fade to exact bypass.
        if(current[0]==0.&&current[1]==0.&&current[2]==0.) {
            if(active){mid={};side={};energy={};bias={};widthLow={};active=false;}
            return;
        }
        active=true;
        l=std::clamp(l,-32.,32.);r=std::clamp(r,-32.,32.);
        const double m=.5*(l+r),s=.5*(l-r);
        const auto mb=split(m,mid),sb=split(s,side);
        double dm=0.,ds=0.,widthDelta=0.;
        const double saturation=current[2]+.35*current[0]*(1.-current[2]);
        for(int i=0;i<5;++i) {
            const double bm=mb[i],bs=sb[i];auto& e=energy[i];
            follow(e.m,bm*bm,env);follow(e.s,bs*bs,env);
            const double power=e.m+e.s;
            const double protect=std::clamp((e.m-e.s)/(power+1.e-16)/.75,0.,1.);
            widthDelta+=current[1]*spread[i]*protect*bs;
            if(current[0]>0.||current[2]>0.) {
                const double d=drive[i]/(1.+4.*power);
                const double gain=current[0]*color[i]/(1.+3.*power);
                const double a=bm+bs,b=bm-bs;
                const double ra=shape(a,d,saturation)+gain*a;
                const double rb=shape(b,d,saturation)+gain*b;
                // Add the nonlinear residual with DC rejection. Keep the
                // low-frequency linear color out of the DC blocker.
                const double na=ra-gain*a,nb=rb-gain*b;
                follow(bias[i][0],na,dc);follow(bias[i][1],nb,dc);
                const double ca=ra-bias[i][0],cb=rb-bias[i][1];
                dm+=.5*(ca+cb);ds+=.5*(ca-cb);
            } else bias[i]={};
        }
        // A residual high-pass stage restricts added width below 210 Hz.
        follow(widthLow[0],widthDelta,bassGuard);widthDelta-=widthLow[0];
        ds+=widthDelta;
        l+=dm+ds;r+=dm-ds;
    }
private:
    struct Low {double a=0.,b=0.;};
    struct Energy {double m=0.,s=0.;};
    std::array<Low,4> mid{},side{};
    std::array<Energy,5> energy{};
    std::array<std::array<double,2>,5> bias{};
    std::array<double,4> coeff{};
    std::array<double,5> color{},spread{},drive{};
    std::array<double,3> current{},target{};
    std::array<double,2> widthLow{};
    double env=0.,slew=0.,dc=0.,bassGuard=0.; bool active=false;
    static double clean(double x,double fallback) noexcept {return std::isfinite(x)?x:fallback;}
    static double unit(double x) noexcept {return std::clamp(clean(x,0.),0.,1.);}
    static void follow(double& v,double x,double a) noexcept {v+=a*(x-v);if(std::abs(v)<1.e-28)v=0.;}
    std::array<double,5> split(double x,std::array<Low,4>& z) noexcept {
        std::array<double,5> b{};double prev=0.;
        for(int i=0;i<4;++i){follow(z[i].a,x,coeff[i]);follow(z[i].b,z[i].a,coeff[i]);b[i]=z[i].b-prev;prev=z[i].b;}
        b[4]=x-prev;return b;
    }
    static double shape(double x,double d,double amount) noexcept {
        const double u=x*d,q=u*u;
        // Bounded asymmetric harmonic residual; no per-sample transcendental.
        return amount*(.09*q-.16*u*q)/(1.+q)/d;
    }
};
}
