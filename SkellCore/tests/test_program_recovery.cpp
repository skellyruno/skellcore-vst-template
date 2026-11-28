#include <SkellCore/experimental/ProgramRecovery.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
using skellcore::experimental::ProgramRecovery;
void require(bool ok, const char* reason) { if (!ok) throw std::runtime_error(reason); }
int main() {
    for (double sr : {44100., 48000., 96000., 192000., 352800., 768000.}) {
        ProgramRecovery shortEvent, longEvent;
        shortEvent.prepare(sr); longEvent.prepare(sr);
        require(shortEvent.process(1.) == 1., "unity exact");
        for (int i = 0; i < int(sr * .005); ++i) require(shortEvent.process(.3) <= .3, "fast attack");
        for (int i = 0; i < int(sr * .5); ++i) longEvent.process(.3);
        double a=0., b=0.;
        for (int i=0; i<int(sr*.1); ++i) { a=shortEvent.process(1.); b=longEvent.process(1.); }
        require(a>b+.15, "short event recovers faster than sustained event");
        std::mt19937 rng(719); std::uniform_real_distribution<double> dist(0.,1.);
        for (int i=0; i<100000; ++i) { const double x=dist(rng), g=longEvent.process(x); require(std::isfinite(g)&&g>=0.&&g<=x,"bounded gain"); }
        require(longEvent.process(std::numeric_limits<double>::quiet_NaN())==0.,"invalid target fail closed");
        longEvent.reset(); require(longEvent.process(1.)==1.,"deterministic reset");
        std::cout<<"PASS "<<sr<<" short/sustained after 100ms "<<a<<" "<<b<<"\n";
    }
}
