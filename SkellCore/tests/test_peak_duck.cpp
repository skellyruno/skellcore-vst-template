#include <SkellCore/ResearchWetDuck.h>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
void require(bool ok){if(!ok)throw std::runtime_error("peak duck regression");}
int main(){
    for(double fs:{44100.,48000.,96000.,192000.}){
        using skellcore::research::WetDuck;
        WetDuck flat(fs), explicitZero(fs,9.,14.,260.,0.), peak(fs,9.,14.,260.,1.), swap(fs,9.,14.,260.,1.);
        double sumA=0.,sumB=0.;
        for(int i=0;i<int(fs);++i){
            const double x=.45*std::sin(6.283185307179586*113.*i/fs);
            const double a=flat.process(x,0.),b=peak.process(x,0.);
            require(a==explicitZero.process(x,0.));require(b==swap.process(0.,x));
            require(b<=a+1e-12&&b>=std::pow(10.,-9./20.));sumA+=a;sumB+=b;
        }
        require(sumB<sumA);
        const double start=peak.gain();int t90=0;
        for(int i=1;i<int(2*fs);++i){double prev=peak.gain(),g=peak.process(0.,0.);require(g<=1.&&std::isfinite(g));if(i>int(.02*fs))require(g>=prev);if(!t90&&g>=1.-.1*(1.-start))t90=i;}
        require(t90>0&&t90/fs<.65);
        peak.reset();require(peak.process(0.,0.)==1.);require(peak.process(std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity())==1.);
        std::cout<<fs<<" Hz: t90="<<1000.*t90/fs<<" ms\n";
    }
    for(double v:{-1.,101.,std::numeric_limits<double>::quiet_NaN()}){bool rejected=false;try{skellcore::research::WetDuck d(48000.,9.,14.,260.,v);}catch(const std::invalid_argument&){rejected=true;}require(rejected);}
}
