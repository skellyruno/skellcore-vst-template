#include "ColorSplitSymmetric.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <random>
using namespace skellcore::experimental::spectral;
int main(){std::mt19937 rng(122);std::uniform_real_distribution<double> u(-2,2);unsigned long long checks=0;
for(double fs:{8000.,44100.,48000.,96000.,192000.,384000.})for(double wet:{0.,.5,1.}){
 ColorSplitSymmetric<4,40,450,475> a,b;a.prepare(fs,12,.12,wet,0);b.prepare(fs,12,.12,wet,0);Frame dry[40]{};
 for(int i=0;i<8192;++i){if(i==4096){a.reset();b.reset();for(auto& d:dry)d={};}Frame x{u(rng),u(rng)};auto y=a.process(x),z=b.process({x.r,x.l});assert(std::isfinite(y.l)&&std::isfinite(y.r)&&y.l==z.r&&y.r==z.l);if(wet==0)assert(y.l==dry[i%40].l&&y.r==dry[i%40].r);dry[i%40]=x;checks+=2;}
}static_assert(ColorSplitSymmetric<4,40,450,475>::latency==40);std::printf("%llu finite/stereo channel checks; dry latency and reset passed\n",checks);}
