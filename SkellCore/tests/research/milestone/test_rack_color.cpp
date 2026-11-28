#include "RackColor.h"
#include <cstdio>
#include <cstdlib>
#include <random>
#include <chrono>
using namespace skell_milestone;
static void check(bool b,const char* msg){if(!b){std::fprintf(stderr,"FAIL %s\n",msg);std::exit(1);}}
int main(){
    SlidingPeak<33> peak;std::array<double,33> hist{};std::mt19937 rng(44);std::uniform_real_distribution<double> u(-2,2);
    for(int i=0;i<100000;++i){double x=i%100<40?double(i%40):std::abs(u(rng));hist[i%33]=x;check(peak.push(x)==*std::max_element(hist.begin(),hist.end()),"queue parity");}
    for(double fs:{44100.,48000.,96000.,192000.}){
        ParallelAlign a;a.prepare(fs,{0,17,64,24},{.25,.25,.25,.25});check(a.latency()==64,"rack latency");
        for(int i=0;i<150;++i){std::array<Frame,4> x{};for(std::size_t j=0;j<4;++j){const int n=std::array<int,4>{0,17,64,24}[j];if(i==n)x[j]={.5,-.25};}const auto y=a.process(x);check(y==(i==64?Frame{.5,-.25}:Frame{}),"parallel impulse aligned");}
        a.reset();a.setMix({0,0,1,0});for(int i=0;i<int(fs*.1);++i){auto y=a.process({Frame{.5,.5},Frame{.5,.5},Frame{.5,.5},Frame{.5,.5}});check(y[0]<=.5+1e-12&&y[0]>=0.&&y[0]==y[1],"mix transition headroom");if(i>int(fs*.02))check(std::abs(y[0]-.5)<1e-12,"unity after transition");}
        ReconstructionGuard old;FastReconstructionGuard fast;old.prepare(fs);fast.prepare(fs);
        for(int i=0;i<150000;++i){Frame x={u(rng),u(rng)};if(i%10000<5000)x={0,0};if(i==5000)x={NAN,INFINITY};check(old.process(x)==fast.process(x),"exact guard parity");}
        SoftColor s;s.prepare(8);double pk=0.;for(int i=0;i<10000;++i){auto y=s.process({u(rng),u(rng)});pk=std::max({pk,std::abs(y[0]),std::abs(y[1])});check(std::isfinite(y[0])&&std::isfinite(y[1])&&pk<=2./3.+1e-10,"color bounds");}
        std::printf("%.0f Hz parallel aligned 64 samples, 150000 guard frames exact, soft bound %.9g PASS\n",fs,pk);
    }
    SoftColor saturated;saturated.prepare(24);saturated.process({63.,-63.});for(int i=0;i<1000;++i){const auto y=saturated.process({63.+i*1e-6,-63.-i*1e-6});check(y==Frame{2./3.,-2./3.},"near-equal saturated ADAA precision");}
    ParallelAlign zero;zero.prepare(48000,{0,0,0,0},{.5,.5,0,0});check(zero.latency()==0&&zero.process({Frame{1.,-.5},Frame{-1.,.5},Frame{},Frame{}})==Frame{},"zero-latency phase cancellation");
    SoftColor s;s.prepare(9);double ar=0,ai=0,br=0,bi=0,fr=0,fi=0,gr=0,gi=0;for(int i=0;i<96000;++i){double x=.65*std::sin(2*pi*11000*i/48000.);auto y=s.process({x,x}),z=s.direct({x,x});if(i>=48000){double t=2*pi*15000*i/48000.,f=2*pi*11000*i/48000.;ar+=z[0]*std::cos(t);ai+=z[0]*std::sin(t);br+=y[0]*std::cos(t);bi+=y[0]*std::sin(t);fr+=z[0]*std::cos(f);fi+=z[0]*std::sin(f);gr+=y[0]*std::cos(f);gi+=y[0]*std::sin(f);}}
    const double reduction=20*std::log10(std::hypot(br,bi)/std::hypot(ar,ai));const double matched=reduction-20*std::log10(std::hypot(gr,gi)/std::hypot(fr,fi));check(matched< -1.,"selected matched fold reduction");std::printf("selected 15k fold %.4f dB, fundamental-matched %.4f dB\n",reduction,matched);
    bool rejected=false;try{ParallelAlign a;a.prepare(48000,{0,0,-1,0},{1,0,0,0});}catch(const std::invalid_argument&){rejected=true;}check(rejected,"negative latency rejected");
}
