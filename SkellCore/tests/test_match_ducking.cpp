#include <SkellCore/MatchStereoDelay.h>
#include <cstdlib>
#include <cstdio>
using Delay=skellcore::dsp::MatchStereoDelay;
static void check(bool b) { if(!b) std::abort(); }
int main() {
    for(double sr:{44100.,48000.,96000.,192000.}) {
        Delay dry,duck,trim; Delay::Parameters p; p.delayMs=50.; p.feedback=.65f; p.wet=.5f;
        check(dry.prepare(sr,p)); p.duckDepth=.8f; p.duckThreshold=.05f; check(duck.prepare(sr,p));
        p.outputGain=.5f; check(trim.prepare(sr,p));
        double loudWet=0., duckWet=0., tail=0.;
        for(int i=0;i<int(sr*.8);++i) {
            float in=i<int(sr*.4)?.25f:0.f; auto a=dry.process({in,in}),b=duck.process({in,in}),c=trim.process({in,in});
            check(std::abs(c.left-b.left*.5f)<1.e-7f);
            if(i>int(sr*.15)&&i<int(sr*.4)) { loudWet+=a.left-in; duckWet+=b.left-in; }
            if(i>int(sr*.4)) tail+=std::abs(b.left);
        }
        check(duckWet<loudWet*.65 && tail>0.);
        duck.reset(); check(duck.process({.2f,.2f}).left==.2f);
        p.duckDepth=1.1f;check(!duck.prepare(sr,p));
    }
    std::puts("MatchStereoDelay ducking PASS: wet-only reduction, repeat tail, explicit trim, four rates");
}
