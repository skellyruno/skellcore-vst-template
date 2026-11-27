#include "CalibratedVocalBands.h"
#include <cstdlib>
#include <iostream>
using namespace skellcore::vocal;
static void check(bool b){if(!b)std::abort();}
int main(){int n=0;for(double sr:{32000.,44100.,48000.,96000.,192000.}){CalibratedAirBands a,b;CalibratedDeEssBand d,e;AirBandSettings as;DeEssBandSettings ds;ds.rangeDb=0;check(a.prepare(sr,as)&&d.prepare(sr,ds));++n;
 for(int i=0;i<4000;++i){float v=.5f*std::sin(i*.6f);auto x=a.process(v,-v),y=d.process(v,-v);check(x[0]==v&&x[1]==-v&&y[0]==v&&y[1]==-v);}++n;
 as.mid=.3;as.high=.5;ds.rangeDb=12;check(a.prepare(sr,as)&&b.prepare(sr,as)&&d.prepare(sr,ds)&&e.prepare(sr,ds));++n;
 for(int i=0;i<int(sr*.2);++i){float v=.4f*std::sin(i*.6f);auto x=a.process(v,-v),y=b.process(v,-v),u=d.process(v,-v),z=e.process(v,-v);check(x==y&&u==z&&std::isfinite(x[0])&&std::isfinite(u[0])&&x[0]==-x[1]&&u[0]==-u[1]);}++n;
 a.reset();b.reset();d.reset();e.reset();check(a.process(.1f,-.1f)==b.process(.1f,-.1f)&&d.process(.1f,-.1f)==e.process(.1f,-.1f));++n;
 auto x=a.process(NAN,0),y=d.process(0,NAN);check(std::isfinite(x[0])&&std::isfinite(y[1]));++n;
 as.ratio=NAN;ds.hz=20000;check(!a.prepare(sr,as)&&!d.prepare(sr,ds));++n;
 }std::cout<<n<<" checks passed\n";}
