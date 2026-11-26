#include "SignedPeakGuard.h"
#include <cstdio>
#include <cstdlib>
#include <random>
using namespace skell_signed_peak;
static void check(bool x,const char* m){if(!x){std::fprintf(stderr,"FAIL %s\n",m);std::exit(1);}}
int main(){for(double fs:{44100.,48000.,96000.,192000.}){
    Guard<true> g;Guard<false> old;g.prepare(fs);old.prepare(fs);std::array<Frame,34> ring{};int pos=0;std::mt19937 rng(80);std::uniform_real_distribution<double> u(-2,2);double worst=0,energy=0,baseline=0;
    for(int i=0;i<200000;++i){Frame x{u(rng),u(rng)};if(i%10000<3000)x={1.5*std::sin(i*.99),1.5*std::cos(i*.77)};if(i%10000>9000)x={0,0};if(i==5000)x={NAN,INFINITY};auto y=g.process(x),b=old.process(x);ring[pos]=ring[pos+17]=y;for(int c=0;c<2;++c){check(std::abs(y[c])+1e-12>=std::abs(b[c]),"split bound cannot reduce output vs absolute bound");energy+=y[c]*y[c];baseline+=b[c]*b[c];}worst=std::max({worst,std::abs(y[0]),std::abs(y[1])});for(const auto& h:g.phases()){Frame z{};for(int k=0;k<17;++k){z[0]+=h[k]*ring[pos+17-k][0];z[1]+=h[k]*ring[pos+17-k][1];}worst=std::max({worst,std::abs(z[0]),std::abs(z[1])});}if(++pos==17)pos=0;}
    check(worst<=std::pow(10.,-1./20.)+1e-12,"finite kernel ceiling");g.reset();for(int i=0;i<80;++i)check(g.process({i==0?.1:0.,0})==(i==16?Frame{.1,0}:Frame{}),"quiet impulse delay");
    std::printf("%.0f Hz max %.12f RMS recovery %.6f dB, 200000 frames PASS\n",fs,worst,10*std::log10(energy/baseline));
}}
