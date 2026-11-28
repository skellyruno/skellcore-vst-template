#include "StackedFlanger.h"
#include <cassert>
#include <iostream>
#include <limits>
#include <chrono>
int main() {
    for (double fs:{44100.,48000.,96000.,192000.}) {
        skellcore::experimental::modulation::StackedFlanger f; assert(f.prepare(fs));
        skellcore::experimental::modulation::FlangeSettings p; p.order=8;p.delayMs=10;p.depthMs=0;p.spread=0;p.dry=0;p.wet=1;
        for (double cross:{0.,1.,-1.}) {
            p.cross=cross;f.setParameters(p);f.reset();
            const int arrival=static_cast<int>(fs*.01);
            for(int i=0;i<arrival+3;++i) {float l=i==0?1.f:0.f,r=0;f.process(l,r);
                assert(std::abs(l-(i==arrival && cross==0?1.:0.))<1e-5);
                assert(std::abs(r-(i==arrival?cross:0.))<1e-5);
            }
        }
        p.dry=-1;p.wet=0;f.setParameters(p);f.reset();float l=.25f,r=-.5f;f.process(l,r);assert(l==-.25f&&r==.5f);
        p.order=16;p.dry=1;p.wet=1;p.cross=-.5;p.spread=1;p.depthMs=20;p.delayMs=40;p.rateHz=5;
        f.setParameters(p);f.reset();
        const auto start=std::chrono::steady_clock::now();
        for(int i=0;i<static_cast<int>(fs);++i) {l=static_cast<float>(.8*std::sin(i*.051));r=static_cast<float>(.8*std::cos(i*.053));if(i==2000){p.order=1;f.setParameters(p);}if(i==4000){p.order=16;f.setParameters(p);}f.process(l,r);assert(std::isfinite(l)&&std::isfinite(r)&&std::abs(l)<=1.601&&std::abs(r)<=1.601);}
        const auto ns=std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-start).count()/fs;
        l=std::numeric_limits<float>::quiet_NaN();r=std::numeric_limits<float>::infinity();f.process(l,r);assert(std::isfinite(l)&&std::isfinite(r));
        std::cout<<"SKELL_STACKED_PASS fs="<<fs<<" ns_per_frame="<<ns<<'\n';
    }
    skellcore::experimental::modulation::StackedFlanger f;assert(!f.prepare(0));
}
