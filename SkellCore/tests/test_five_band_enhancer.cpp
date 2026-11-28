#include <SkellCore/experimental/FiveBandEnhancer.h>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using skellcore::experimental::FiveBandEnhancer;
static void check(bool b,const char* s){if(!b)throw std::runtime_error(s);}
int main(){
    for(double sr:{44100.,48000.,88200.,96000.,176400.,192000.,352800.,384000.,768000.}) {
        FiveBandEnhancer bypass,mono,anti,reset; bypass.prepare(sr);mono.prepare(sr);anti.prepare(sr);reset.prepare(sr);
        mono.setAmounts(0.,1.,0.);anti.setAmounts(0.,1.,0.);reset.setAmounts(1.,1.,1.);
        for(int i=0;i<int(sr*.1);++i){
            double l=.3*std::sin(i*.17),r=.25*std::cos(i*.23),a=l,b=r;
            bypass.process(l,r);check(l==a&&r==b,"neutral exact");
            r=l;mono.process(l,r);check(l==a&&r==a,"mono width exact");
            l=a;r=-a;anti.process(l,r);check(l==a&&r==-a,"anti-phase width exact");
            l=a;r=b;reset.process(l,r);check(std::isfinite(l)&&std::isfinite(r),"finite");
        }
        reset.reset();double l=.2,r=-.1;reset.process(l,r);check(l==.2&&r==-.1,"reset bypass exact");
        for(double f:{60.,150.,500.,1000.,2000.,7000.,12000.}){
            FiveBandEnhancer e;e.prepare(sr);e.setAmounts(0.,1.,0.);double in=0.,out=0.;
            for(int i=0;i<int(sr*.4);++i){
                const double m=.25*std::sin(6.283185307179586*f*i/sr),s=.05*std::cos(6.283185307179586*f*i/sr);
                double a=m+s,b=m-s;e.process(a,b);check(std::abs(.5*(a+b)-m)<1.e-12,"width center invariant");
                if(i>sr*.2){in+=s*s;out+=.25*(a-b)*(a-b);}
            }
            const double db=10.*std::log10(out/in);check(f<=150.?std::abs(db)<.9:db<10.,"width bounds");
            if(sr==176400.)std::cout<<f<<"Hz "<<db<<"dB\n";
        }
        FiveBandEnhancer e;e.prepare(sr);e.setAmounts(1.,1.,1.);
        for(int i=0;i<int(sr*.1);++i){double a=.5*std::sin(i*.11),b=.5*std::cos(i*.12);e.process(a,b);}
        e.setAmounts(0.,0.,0.);
        for(int i=0;i<int(sr*.8);++i){double a=.3,b=.2;e.process(a,b);}
        double a=.3,b=.2;e.process(a,b);check(a==.3&&b==.2,"fade to exact bypass");
        e.setAmounts(1.,1.,1.);a=std::numeric_limits<double>::infinity();b=std::numeric_limits<double>::quiet_NaN();e.process(a,b);check(std::isfinite(a)&&std::isfinite(b),"invalid inputs");
        // Sauce and saturation must each produce real non-linear coloration,
        // while preserving mono symmetry and suppressing steady DC offset.
        for(bool sauce:{true,false}) {
            FiveBandEnhancer harmonic;harmonic.prepare(sr);
            harmonic.setAmounts(sauce?1.:0.,0.,sauce?0.:1.);
            double h2c=0.,h2s=0.,h3c=0.,h3s=0.,mean=0.;int count=0;
            for(int i=0;i<int(sr*.5);++i) {
                const double w=6.283185307179586*1000.*i/sr;
                double x=.5*std::sin(w),y=x;harmonic.process(x,y);
                check(x==y&&std::isfinite(x),"linked mono color");
                if(i>=int(sr*.25)){h2c+=x*std::cos(2*w);h2s+=x*std::sin(2*w);h3c+=x*std::cos(3*w);h3s+=x*std::sin(3*w);mean+=x;++count;}
            }
            check(2.*std::hypot(h2c,h2s)/count>1.e-4,"even harmonic response");
            check(2.*std::hypot(h3c,h3s)/count>1.e-4,"odd harmonic response");
            check(std::abs(mean/count)<1.e-5,"DC rejection");
            if(sr==176400.)std::cout<<(sauce?"Sauce":"Saturation")<<" H2/H3 linear "<<2.*std::hypot(h2c,h2s)/count<<" "<<2.*std::hypot(h3c,h3s)/count<<"\n";
        }
        std::cout<<"PASS "<<sr<<"\n";
    }
}
