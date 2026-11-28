#include "EvidenceEngines.h"
#include <cstdio>
#include <cstdlib>
#include <random>
using namespace skell_milestone;
static void check(bool x,const char* m){if(!x){std::fprintf(stderr,"FAIL %s\n",m);std::exit(1);}}
int main(){for(double fs:{44100.,48000.,96000.,192000.}){
    BoundedPeakGuard g;g.prepare(fs);std::array<Frame,34> ring{};int pos=0;std::mt19937 rng(80);std::uniform_real_distribution<double> u(-2,2);double worst=0;
    for(int i=0;i<150000;++i){Frame x{u(rng),u(rng)};if(i%10000<3000)x={1.5*std::sin(i*.99),1.5*std::cos(i*.77)};if(i%10000>9000)x={0,0};if(i==5000)x={NAN,INFINITY};auto y=g.process(x);ring[pos]=ring[pos+17]=y;worst=std::max({worst,std::abs(y[0]),std::abs(y[1])});for(const auto& h:g.phases()){Frame z{};for(int k=0;k<17;++k){z[0]+=h[k]*ring[pos+17-k][0];z[1]+=h[k]*ring[pos+17-k][1];}worst=std::max({worst,std::abs(z[0]),std::abs(z[1])});}if(++pos==17)pos=0;}
    check(worst<=db(-1)+1e-12,"finite-kernel ceiling");g.reset();for(int i=0;i<80;++i)check(g.process({i==0?.1:0.,0})==(i==16?Frame{.1,0}:Frame{}),"quiet impulse delay");
    TimedGate a,b;a.prepare(fs);b.prepare(fs);for(int i=0;i<100;++i)check(a.next(.001,.001)==1.,"default exact bypass");a.setParameters(1,-45,4,2,60,1,1);b.setParameters(1,-45,4,2,20,.25,1);double slow=1,quick=1;for(int i=0;i<int(fs*.1);++i){check(a.next(.2,-.2)>.99&&b.next(-.2,.2)>.99,"open");}for(int i=0;i<int(fs*.25);++i){slow=a.next(.001,-.001);quick=b.next(-.001,.001);}check(quick<slow*.1,"faster detector recovery");
    a.setParameters(0,-45,4,2,60);for(int i=0;i<int(fs*.02);++i)slow=a.next(.001,.001);check(slow==1.,"ramped exact bypass");a.setParameters(1,-45,0,2,60);a.reset();for(int i=0;i<100;++i)check(a.next(0,0)==1.,"neutral ratio");a.setParameters(1,NAN,INFINITY,NAN,-1,NAN,INFINITY);check(std::isfinite(a.next(NAN,INFINITY)),"nonfinite");
    std::printf("%.0f Hz finite-kernel peak %.9f, fast gate gain after 250ms %.9f, guard delay 16 PASS\n",fs,worst,quick);
}}
