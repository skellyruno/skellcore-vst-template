#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace skellcore::experimental::spectral {
// Original causal 4x IIR candidate. No fixed sample buffer or lookahead.
// Nonlinear phase: frequency-dependent group delay, not zero acoustic delay.
class LowDelayColor {
public:
    using Frame=std::array<double,2>;
    static constexpr unsigned bufferingLatency=0;
    bool prepare(double fs,double drive=4.,double trim=1.) noexcept {
        if(!std::isfinite(fs)||fs<8000||fs>192000||!valid(drive,trim))return false;
        alpha_=1.-std::exp(-1./(.01*fs));targetDrive_=drive;targetTrim_=trim;reset();return true;
    }
    bool setTarget(double drive,double trim) noexcept {
        if(!valid(drive,trim))return false;
        targetDrive_=drive;targetTrim_=trim;return true;
    }
    void reset() noexcept {channels_={};drive_=targetDrive_;trim_=targetTrim_;}
    Frame process(Frame x) noexcept {
        drive_+=alpha_*(targetDrive_-drive_);trim_+=alpha_*(targetTrim_-trim_);
        Frame y{};
        for(unsigned c=0;c<2;++c){auto& s=channels_[c];const double v=clean(x[c]);
            // Two causal 2x interpolation/decimation stages, even output phases.
            const Frame p{s.upA.next(2*v,a_),s.upA.next(0.,a_)};
            Frame q{};
            for(unsigned j=0;j<2;++j){const double a=s.upB.next(2*p[j],b_),b=s.upB.next(0.,b_);
                q[j]=s.downB.next(std::tanh(drive_*a),b_);s.downB.next(std::tanh(drive_*b),b_);}
            y[c]=trim_*s.downA.next(q[0],a_);s.downA.next(q[1],a_);
        }return y;
    }
private:
    static bool valid(double drive,double trim) noexcept {return std::isfinite(drive)&&drive>=.1&&drive<=20&&std::isfinite(trim)&&trim>=0&&trim<=1;}
    static double clean(double v) noexcept {return std::isfinite(v)&&std::abs(v)>=1e-25?std::clamp(v,-4.,4.):0.;}
    using Coeff=std::array<double,5>;
    template<unsigned N> struct Filter {
        std::array<Frame,N> z{};
        double next(double x,const std::array<Coeff,N>& h) noexcept {
            for(unsigned i=0;i<N;++i){const auto& p=h[i];auto& s=z[i];const double y=p[0]*x+s[0];s[0]=p[1]*x-p[3]*y+s[1];s[1]=p[2]*x-p[4]*y;
                if(std::abs(s[0])<1e-30)s[0]=0.;
                if(std::abs(s[1])<1e-30)s[1]=0.;
                x=y;
            }return x;
        }
    };
    // Original elliptic SOS: stage A .49/.505 Nyquist, .01/100 dB; B .3/.7, .005/100 dB.
    inline static constexpr std::array<Coeff,9> a_{{
        {0x1.1f4df1fd1b40ep-9, 0x1.ddc6311ee3686p-9, 0x1.1f4df1fd1b40ep-9, -0x1.caee513fc38c5p-2, 0x0.0p+0},
        {0x1.0000000000000p+0, 0x1.0ddc70b8ecb9cp+0, 0x1.0000000000000p+0, -0x1.9bb23e66297adp-1, 0x1.240a4c2212bffp-2},
        {0x1.0000000000000p+0, 0x1.0000000000000p+0, 0x0.0p+0, -0x1.3213fb97215fep-1, 0x1.e535b8f2887aap-2},
        {0x1.0000000000000p+0, 0x1.2e2ea68b1f2afp-1, 0x1.0000000000000p+0, -0x1.920347e6e6930p-2, 0x1.52ccea539d9ccp-1},
        {0x1.0000000000000p+0, 0x1.416148d2f0b76p-2, 0x1.0000000000001p+0, -0x1.f068f266abf4dp-3, 0x1.99750cbf30a7fp-1},
        {0x1.0000000000000p+0, 0x1.4aaf49fc721d1p-3, 0x1.0000000000001p+0, -0x1.2f3a6bec60e29p-3, 0x1.c65523a67c8f1p-1},
        {0x1.0000000000000p+0, 0x1.48a0528baf3bbp-4, 0x1.ffffffffffffep-1, -0x1.808e97c282b26p-4, 0x1.e114de85c3bfdp-1},
        {0x1.0000000000000p+0, 0x1.403fcfba7c3f9p-5, 0x1.0000000000000p+0, -0x1.0bc88bbdee6d8p-4, 0x1.f10ae85955058p-1},
        {0x1.0000000000000p+0, 0x1.671268504286cp-6, 0x1.0000000000001p+0, -0x1.b557146e2a50cp-5, 0x1.fb8c873b90efep-1}
    }};
    inline static constexpr std::array<Coeff,3> b_{{
        {0x1.2cb48a57005f5p-8, 0x1.22036b2ca9084p-7, 0x1.2cb48a57005f4p-8, -0x1.c1d7e3245b7c3p-1, 0x1.cd23aaf818a48p-3},
        {0x1.0000000000000p+0, 0x1.87bf8da76be84p+0, 0x1.fffffffffffffp-1, -0x1.9ed8a728b2c45p-1, 0x1.afbe7a1944bedp-2},
        {0x1.0000000000000p+0, 0x1.36c609d99284ep+0, 0x1.fffffffffffffp-1, -0x1.99973b85172aap-1, 0x1.8821fc5c0cccdp-1}
    }};
    struct Channel {Filter<9> upA,downA;Filter<3> upB,downB;};
    std::array<Channel,2> channels_{};double alpha_=1.-std::exp(-1./480.),drive_=4.,trim_=1.,targetDrive_=4.,targetTrim_=1.;
};
}
