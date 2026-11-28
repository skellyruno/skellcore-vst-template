#include "CausalHalfbandColor.h"
#include "CausalHalfbandCoefficients.h"
#include <cassert>
#include <cmath>
#include <limits>
using namespace skellcore::experimental::spectral;
int main(){
    CausalHalfbandColor a,b;assert(a.prepare(halfband641,halfband65));assert(b.prepare(halfband641,halfband65));
    unsigned peak=0;double max=0;
    for(unsigned i=0;i<2048;++i){auto y=a.process({i==0?.5:0.,0.});assert(y[1]==0.);if(std::abs(y[0])>max){max=std::abs(y[0]);peak=i;}}assert(peak==CausalHalfbandColor::latency);
    a.reset();for(unsigned i=0;i<48000;++i){double l=.8*std::sin(i*.13),r=.2*std::cos(i*.2);auto y=a.process({l,r}),z=b.process({r,l});assert(y[0]==z[1]&&y[1]==z[0]);}
    auto bad=halfband641;bad[0]=std::numeric_limits<double>::quiet_NaN();assert(!a.prepare(bad,halfband65));assert(!a.prepare(halfband641,halfband65,0));
    a.reset();auto y=a.process({std::numeric_limits<double>::infinity(),0.});assert((y==CausalHalfbandColor::Frame{}));
    // Fixed-state processing, reproducible reset, no hidden coefficient lifetimes.
    a.reset();b.reset();for(unsigned i=0;i<8192;++i){double x=std::sin(i*.17);assert(a.process({x,x})==b.process({x,x}));}
}
