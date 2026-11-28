#pragma once
#include "SpatialColor.h"
#include "WetBalance.h"
namespace skell_milestone {
// Independent motion processor, based on general panning/filter concepts.
// Control updates on the processing thread only. Prepare/reset off-thread.
class MotionField {
public:
    enum class Mode { Free, Tempo, Input, Manual };
    enum class Path { Line, Orbit, PhaseOrbit, Pulse };
    enum class Wave { Sine, Triangle, Ramp, Square };
    enum class Trigger { Continuous, Latch, Cycle, Gate, Return, Toggle };
    enum class Dynamics { Off, Wet, Dry, Output };
    struct Settings {
        Mode mode=Mode::Free; Path path=Path::Line; Wave wave=Wave::Sine;
        Trigger trigger=Trigger::Continuous; Dynamics dynamics=Dynamics::Off;
        double seconds=1,beats=2,bpm=120,start=0,rangeA=0,rangeB=1;
        double width=1,depth=1,thresholdDb=-30,holdMs=0,inputSensitivity=1;
        double predelayMs=0,travelDelayMs=0,shelfHz=1000,closeDb=0,farDb=0;
        double level=1,pulseWidth=.6,driveDb=0,dirt=0,triggerHP=0;
        bool reverse=false,external=false;
    };
private:
    struct Lane {
        Settings p{}; std::vector<double> buffer; std::size_t pos=0;
        double progress=0,envelope=0,hold=0,step=0,threshold=0;
        double l=0,r=0,delay=0,low=0,colour=0,keyLow=0,keyCoeff=0;
        double drive=1,comp=1,close=1,farShelf=1,level=1,shelf=1;bool latched=false,active=false,high=false;
        double actualPan=0,far=0;
    };
    std::array<Lane,2> lanes{};double fs=48000,slew=0,detector=0,attack=0,release=0,wet=0,targetWet=0,wetTrim=1,targetTrim=1;
    skell_balance::WetBalance balance;bool adaptiveWet=false;
    static double wrap(double p) noexcept {return p-std::floor(p);}
    static double wave(Wave w,double t) noexcept {
        switch(w){case Wave::Triangle:return 1.-std::abs(2.*t-1.);case Wave::Ramp:return t;case Wave::Square:return t<.5?0.:1.;default:return .5-.5*std::cos(2*pi*t);}
    }
    double dynamics(Lane& a,double peak) noexcept {
        const double driven=peak*a.drive;
        const double want=driven>.125?std::pow(.125/driven,.75):1.;
        a.comp+=(want<a.comp?attack:release)*(want-a.comp);
        return (1.-a.p.dirt)*a.comp+a.p.dirt*(driven>.5?.5/driven:1.);
    }
    void resetLane(Lane& a) noexcept {
        std::fill(a.buffer.begin(),a.buffer.end(),0.);a.pos=0;a.progress=a.envelope=a.hold=0.;
        a.l=a.r=a.low=a.keyLow=0.;a.delay=a.p.predelayMs*.001*fs;a.comp=1.;a.level=a.p.level;a.shelf=a.close;
        a.latched=a.active=a.high=false;a.actualPan=a.far=0.;
    }
    double read(Lane& a,double d) noexcept {
        const auto n=a.buffer.size();d=std::clamp(d,0.,double(n-2));const auto i=std::size_t(d);const double f=d-i;
        return a.buffer[(a.pos+n-i)%n]*(1.-f)+a.buffer[(a.pos+n-i-1)%n]*f;
    }
    Frame lane(Lane& a,double x,double key) noexcept {
        const auto& p=a.p;a.keyLow+=a.keyCoeff*(key-a.keyLow);
        const double k=std::abs(key-(p.triggerHP>0?a.keyLow:0.));
        a.envelope+=detector*(k-a.envelope);if(a.envelope<1e-24)a.envelope=0.;
        const bool over=a.envelope>=(a.high?a.threshold*.8:a.threshold);
        const double holdFrames=p.holdMs*.001*fs;
        if(over!=a.high){if(++a.hold>=holdFrames){a.high=over;a.hold=0;}}else a.hold=0;
        double q=0.;
        if(p.mode==Mode::Manual)q=p.start;
        else if(p.mode==Mode::Input)q=std::clamp(a.envelope*p.inputSensitivity,0.,1.);
        else if(p.trigger==Trigger::Toggle)q=(a.high!=p.reverse)?1.:0.;
        else {
            if(a.high){a.latched=true;a.active=true;}
            bool move=false,back=false;
            switch(p.trigger){case Trigger::Continuous:move=true;break;case Trigger::Latch:move=a.latched;break;case Trigger::Cycle:move=a.active;break;case Trigger::Gate:move=a.high;break;case Trigger::Return:move=a.high;back=!a.high;break;default:break;}
            q=wrap(p.start+(p.reverse?-a.progress:a.progress));
            if(move){a.progress+=a.step;if(a.progress>=1.){a.progress=wrap(a.progress);if(p.trigger==Trigger::Cycle&&!a.high){a.active=false;a.progress=0.;}}}
            else if(back)a.progress=std::max(0.,a.progress-a.step);
        }
        const bool endpoint=p.mode==Mode::Manual||p.mode==Mode::Input||p.trigger==Trigger::Toggle;
        const double t=p.rangeA+(p.rangeB-p.rangeA)*(endpoint?q:wave(p.path==Path::Pulse?Wave::Ramp:p.wave,q));
        double pan=2.*t-1.,far=0.,pulse=1.,polarity=1.;
        if(p.path==Path::Orbit||p.path==Path::PhaseOrbit){pan=std::sin(2*pi*t);far=.5+.5*std::cos(2*pi*t);if(p.path==Path::PhaseOrbit)polarity=std::cos(2*pi*t)<0.?-1.:1.;}
        else if(p.path==Path::Pulse){pan=t<.5?-1.:1.;const double d=std::abs(wrap(2*t)-.5);pulse=std::clamp((p.pulseWidth*.5-d)*20.,0.,1.);}
        else far=std::abs(pan);
        pan=std::clamp(pan*p.width,-1.,1.);a.actualPan=pan;a.far=far;
        const double amp=1.-.5*p.depth*far;
        const double targetL=std::cos((pan+1.)*pi*.25)*amp*pulse;
        const double targetR=std::sin((pan+1.)*pi*.25)*amp*pulse*polarity;
        a.l+=slew*(targetL-a.l);a.r+=slew*(targetR-a.r);
        const double wanted=(p.predelayMs+p.travelDelayMs*far)*.001*fs;
        // Bounded read-head velocity avoids instantaneous delay jumps.
        a.delay+=std::clamp(wanted-a.delay,-.25,.25);
        a.buffer[a.pos]=x;double y=read(a,a.delay);if(++a.pos==a.buffer.size())a.pos=0;
        a.low+=a.colour*(y-a.low);if(std::abs(a.low)<1e-24)a.low=0.;
        const double shelf=a.close+(a.farShelf-a.close)*far;
        a.shelf+=slew*(shelf-a.shelf);a.level+=slew*(p.level-a.level);
        y+=(a.shelf-1.)*(y-a.low);
        if(p.dynamics==Dynamics::Wet)y*=dynamics(a,std::abs(y));
        return {y*a.l*a.level,y*a.r*a.level};
    }
public:
    void prepare(double sampleRate){fs=rate(sampleRate);balance.prepare(fs);slew=-std::expm1(-1./(.002*fs));detector=-std::expm1(-1./(.005*fs));attack=-std::expm1(-1./(.005*fs));release=-std::expm1(-1./(.100*fs));for(int i=0;i<2;++i){lanes[i].buffer.assign(std::size_t(std::ceil(1.2*fs))+3,0.);set(i,lanes[i].p);}reset();}
    void set(std::size_t i,Settings p) noexcept {
        if(i>=lanes.size())return;
        auto& a=lanes[i];p.seconds=bound(p.seconds,.05,125.,1.);p.beats=bound(p.beats,.0625,128.,2.);p.bpm=bound(p.bpm,30.,240.,120.);
        p.start=bound(p.start,0.,1.,0.);p.rangeA=bound(p.rangeA,0.,1.,0.);p.rangeB=bound(p.rangeB,0.,1.,1.);if(p.rangeA>p.rangeB)std::swap(p.rangeA,p.rangeB);
        p.width=bound(p.width,0.,1.,1.);p.depth=bound(p.depth,0.,1.,1.);p.thresholdDb=bound(p.thresholdDb,-60.,0.,-30.);p.holdMs=bound(p.holdMs,0.,1000.,0.);p.inputSensitivity=bound(p.inputSensitivity,0.,100.,1.);
        p.predelayMs=bound(p.predelayMs,0.,200.,0.);p.travelDelayMs=bound(p.travelDelayMs,0.,1000.,0.);p.shelfHz=bound(p.shelfHz,100.,std::min(18000.,fs*.45),1000.);p.closeDb=bound(p.closeDb,-24.,24.,0.);p.farDb=bound(p.farDb,-24.,24.,0.);
        p.level=bound(p.level,0.,1.,1.);p.pulseWidth=bound(p.pulseWidth,.05,1.,.6);p.driveDb=bound(p.driveDb,0.,24.,0.);p.dirt=bound(p.dirt,0.,1.,0.);p.triggerHP=bound(p.triggerHP,0.,2000.,0.);
        if(a.p.mode!=p.mode||a.p.trigger!=p.trigger){a.progress=a.hold=0.;a.latched=a.active=a.high=false;}
        a.p=p;a.step=1./(fs*(p.mode==Mode::Tempo?p.beats*60./p.bpm:p.seconds));a.threshold=db(p.thresholdDb);a.drive=db(p.driveDb);a.close=db(p.closeDb);a.farShelf=db(p.farDb);a.colour=-std::expm1(-2*pi*p.shelfHz/fs);a.keyCoeff=-std::expm1(-2*pi*p.triggerHP/fs);
    }
    void setMix(double amount) noexcept {targetWet=bound(amount,0.,1.,0.);balance.setMix(targetWet);}
    // Configure off-thread before reset; enabling mid-stream is not a smoothed toggle.
    void setAdaptiveWetBalance(bool on) noexcept {adaptiveWet=on;balance.setEnabled(on);}
    bool dryHeadroomExceeded() const noexcept {return adaptiveWet&&balance.dryExceeded();}
    void setWetTrim(double gainDb) noexcept {targetTrim=db(bound(gainDb,-18.,12.,0.));}
    void reset() noexcept {for(auto& a:lanes)resetLane(a);wet=targetWet;wetTrim=targetTrim;balance.reset();}
    double position(std::size_t i)const noexcept{return i<2?lanes[i].actualPan:0.;}
    double cycle(std::size_t i)const noexcept{return i<2?lanes[i].progress:0.;}
    bool triggered(std::size_t i)const noexcept{return i<2&&lanes[i].high;}
    Frame process(Frame x,Frame sidechain={0.,0.}) noexcept {
        x={clean(x[0]),clean(x[1])};if(lanes[0].buffer.empty())return x;
        const auto a=lane(lanes[0],x[0],lanes[0].p.external?clean(sidechain[0]):x[0]);
        const auto b=lane(lanes[1],x[1],lanes[1].p.external?clean(sidechain[1]):x[1]);
        const double s=100./fs;wet=targetWet>wet?std::min(targetWet,wet+s):std::max(targetWet,wet-s);
        wetTrim+=slew*(targetTrim-wetTrim);
        Frame dry=x;for(auto& l:lanes)if(l.p.dynamics==Dynamics::Dry){const double g=dynamics(l,std::max(std::abs(dry[0]),std::abs(dry[1])));dry={dry[0]*g,dry[1]*g};}
        Frame y{(1.-wet)*dry[0]+wet*wetTrim*.5*(a[0]+b[0]),(1.-wet)*dry[1]+wet*wetTrim*.5*(a[1]+b[1])};
        if(adaptiveWet)y=balance.process(dry,{wetTrim*.5*(a[0]+b[0]),wetTrim*.5*(a[1]+b[1])});
        for(auto& l:lanes)if(l.p.dynamics==Dynamics::Output){const double g=dynamics(l,std::max(std::abs(y[0]),std::abs(y[1])));y={y[0]*g,y[1]*g};}
        return y;
    }
};
}
