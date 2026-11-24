#include <SkellCore/MatchConvolution.h>
#include <cstdio>
#include <cstdlib>
#include <limits>
using Conv=skellcore::dsp::MatchConvolution;
static void check(bool b) { if(!b) std::abort(); }
int main() {
    for(double rate:{44100.,48000.,96000.,192000.}) for(auto p:{64u,256u,1024u}) {
        std::vector<Conv::Frame> ir(2*p+17),x(4*p+31);
        for(std::size_t i=0;i<ir.size();++i) ir[i]={.02f*std::sin(float(i)*.05f),.015f*std::cos(float(i)*.07f)};
        for(std::size_t i=0;i<x.size();++i) x[i]={.2f*std::cos(float(i)*.13f),.17f*std::sin(float(i)*.11f)};
        for(auto route:{Conv::Routing::DualMono,Conv::Routing::MonoToStereo}) {
            Conv c; check(c.prepare(rate,rate,ir,p,route,.4f)); check(c.latencySamples()==p);
            std::vector<Conv::Frame> first(x.size());
            for(std::size_t i=0;i<x.size();++i) {
                first[i]=c.process(x[i]); double l=0,r=0;
                if(i>=p) {
                    auto t=i-p; l=x[t].left; r=x[t].right;
                    for(std::size_t k=0;k<ir.size()&&k<=t;++k) {
                        auto a=x[t-k]; const double m=.5*a.left+.5*a.right;
                        l+=.4*ir[k].left*(route==Conv::Routing::DualMono?a.left:m);
                        r+=.4*ir[k].right*(route==Conv::Routing::DualMono?a.right:m);
                    }
                }
                check(std::abs(first[i].left-l)<2.e-5 && std::abs(first[i].right-r)<2.e-5);
            }
            for(std::size_t b:{1u,63u,256u,511u,1024u}) {
                c.reset(); for(std::size_t start=0;start<x.size();start+=b) for(std::size_t i=start;i<std::min(start+b,x.size());++i) {
                    auto o=c.process(x[i]);check(o.left==first[i].left&&o.right==first[i].right);
                }
            }
            check(!c.prepare(rate,rate+1,ir,p,route,.4f)); check(!c.setWet(-1));
            c.reset(); auto bad=c.process({std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}); check(bad.left==0&&bad.right==0);
        }
    }
    std::puts("MatchConvolution PASS: direct FIR parity, four rates, three partitions, two routings, five block schedules");
}
