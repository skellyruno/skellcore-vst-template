#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace skellcore::experimental::spectral {
// Original recording-mode candidate. No lookahead or integer delay buffer.
// ADAA averages the nonlinear transfer over adjacent samples: it has frequency-
// dependent attenuation and approximately half-sample small-signal group delay.
class LiveColor {
public:
    using Frame=std::array<double,2>;
    enum class Mode { Direct, Antiderivative };
    static constexpr unsigned bufferingLatency=0;
    bool prepare(double fs,double drive=4.,double mix=1.,Mode mode=Mode::Antiderivative) noexcept {
        if(!std::isfinite(fs)||fs<8000||fs>192000||(mode!=Mode::Direct&&mode!=Mode::Antiderivative)||!valid(drive,mix))return false;
        alpha_=1.-std::exp(-1./(.01*fs));mode_=mode;targetDrive_=drive;targetMix_=mix;reset();return true;
    }
    bool setTarget(double drive,double mix) noexcept {
        if(!valid(drive,mix))return false;
        targetDrive_=drive;targetMix_=mix;return true;
    }
    void reset() noexcept {previous_={};drive_=targetDrive_;mix_=targetMix_;}
    Frame process(Frame x) noexcept {
        drive_+=alpha_*(targetDrive_-drive_);mix_+=alpha_*(targetMix_-mix_);
        for(unsigned c=0;c<2;++c){const double v=clean(x[c]);const double a=drive_*previous_[c],b=drive_*v;
            const double wet=mode_==Mode::Direct?std::tanh(b):average(a,b);
            previous_[c]=v;x[c]=mix_==0.?v:(1.-mix_)*v+mix_*wet;
        }return x;
    }
private:
    static bool valid(double drive,double mix) noexcept {return std::isfinite(drive)&&drive>=.1&&drive<=20&&std::isfinite(mix)&&mix>=0&&mix<=1;}
    static double clean(double x) noexcept {return std::isfinite(x)&&std::abs(x)>=1e-25?std::clamp(x,-4.,4.):0.;}
    static double primitive(double x) noexcept {const double a=std::abs(x);return a+std::log1p(std::exp(-2*a));}
    static double average(double a,double b) noexcept {
        // Midpoint limit avoids cancellation. Current drive applies to both endpoints.
        const double d=b-a;
        if(std::abs(d)<1e-4)return std::tanh((a+b)*.5);
        return std::clamp((primitive(b)-primitive(a))/d,-1.,1.);
    }
    Frame previous_{};double alpha_=1.-std::exp(-1./480.),drive_=4.,mix_=1.,targetDrive_=4.,targetMix_=1.;Mode mode_=Mode::Antiderivative;
};
}
