#include <SkellCore/MatchStereoDelay.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <vector>
using Delay = skellcore::dsp::MatchStereoDelay;
static void check(bool condition) { if (!condition) std::abort(); }
static bool near(float a, float b) { return std::abs(a-b) < 1.e-6f; }
int main() {
    for (double sr : {44100., 48000., 96000., 192000.}) {
        Delay d; Delay::Parameters p;
        p.delayMs = 20.; p.cutoffHz = 4000.; p.feedback = .6f; p.cross = 1.f; p.wet = .5f;
        check(d.prepare(sr,p)); const auto n = d.delaySamples();
        check(n == static_cast<std::size_t>(std::llround(sr*.02)) && d.latencySamples() == 0);
        const float alpha = static_cast<float>(1.-std::exp(-6.283185307179586*4000./sr));
        auto out = d.process({1.f,0.f}); check(near(out.left,1.f) && out.right==0.f);
        for (std::size_t i=1;i<n;++i) {out=d.process({});check(out.left==0.f && out.right==0.f);}
        out=d.process({}); check(near(out.left,.5f*alpha) && out.right==0.f);
        for (std::size_t i=n+1;i<2*n;++i) d.process({});
        out=d.process({}); check(near(out.right,.5f*.6f*alpha*alpha));
        d.reset(); auto bad=d.process({std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()});
        check(bad.left==0.f && bad.right==0.f);
        const auto render=[&](std::size_t block) {
            d.reset(); std::vector<Delay::Frame> v(3*n+17);
            for(std::size_t start=0;start<v.size();start+=block)
                for(std::size_t i=start;i<std::min(start+block,v.size());++i) {
                    if(i==n/2) check(d.setWet(.1f));
                    v[i]=d.process({.2f*std::sin(static_cast<float>(i)*.013f),-.1f});
                    check(std::isfinite(v[i].left)&&std::isfinite(v[i].right));
                }
            return v;
        };
        const auto base=render(1);
        for(std::size_t b:{64u,256u,511u,1024u}) {
            const auto other=render(b);
            for(std::size_t i=0;i<base.size();++i) check(base[i].left==other[i].left && base[i].right==other[i].right);
        }
        p.wet=0.f;p.midGain=.5f;p.sideGain=2.f;
        check(d.prepare(sr,p));out=d.process({.3f,-.1f});check(near(out.left,.45f)&&near(out.right,-.35f));
        p.feedback=1.f;check(!d.prepare(sr,p));check(!d.setWet(-1.f));
        d.reset();out=d.process({.3f,-.1f});check(near(out.left,.45f)&&near(out.right,-.35f));
    }
    std::puts("MatchStereoDelay PASS: four rates, causal cross-feedback, M/S, reset, block exact, finite and rejected settings");
}
