#include "WetBalance.h"
#include <cstdio>
#include <cstdlib>
#include <random>
using namespace skell_balance;
static void check(bool ok,const char* name){if(!ok){std::fprintf(stderr,"FAIL %s\n",name);std::exit(1);}}
int main(){for(double fs:{44100.,48000.,96000.,192000.}){
    WetBalance b;b.prepare(fs);b.setMix(.65);b.reset();const Frame dry{.2,-.3},wet{.04,.09};
    auto y=b.process(dry,wet);check(y==Frame{(1.-.65)*.2+.65*.04,(1.-.65)*(-.3)+.65*.09},"disabled parity");
    b.setEnabled(true);b.setMix(1);b.reset();for(int i=0;i<int(fs*2);++i)y=b.process({.2,.2},{.05,.05});check(b.gain()>2.80&&b.gain()<2.82,"bounded boost");
    b.setMix(.65);b.reset();std::mt19937 rng(7);std::uniform_real_distribution<double> u(-2,2);double peak=0.;
    for(int i=0;i<150000;++i){Frame d{u(rng),u(rng)},w{u(rng),u(rng)};if(i==100)d={NAN,INFINITY};y=b.process(d,w);check(std::isfinite(y[0])&&std::isfinite(y[1]),"finite");peak=std::max({peak,std::abs(y[0]),std::abs(y[1])});check(peak<=1.+1e-12,"sample headroom");check(!b.dryExceeded(),"dry feasible");}
    b.setMix(.1);b.reset();y=b.process({2.,-2.},{.3,.4});check(b.dryExceeded()&&b.gain()==0.&&y==Frame{1.8,-1.8},"dry overload not hidden");
    b.setMix(0);b.reset();check(b.process(dry,wet)==dry,"exact dry");b.setMix(1);b.reset();for(int i=0;i<int(fs*.4);++i)y=b.process({0.,0.},{0.,0.});check(y==Frame{}&&b.gain()==1.,"silence");
    std::printf("%.0f Hz peak %.12f, 150000 stress frames, PASS\n",fs,peak);
}}
