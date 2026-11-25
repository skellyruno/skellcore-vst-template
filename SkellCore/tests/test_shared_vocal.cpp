#include <SkellCore/backends/vocal/SkellAir.h>
#include <SkellCore/backends/vocal/ClassicDeEsser.h>
#include <SkellCore/backends/vocal/OversampledSaturation.h>
#include <iostream>
#include <cstdlib>
#include <cmath>
using namespace skell::research;
void require(bool x){if(!x)std::abort();}
int main(){int n=0;for(int sr:{8000,44100,48000,96000,192000,384000}){
 ClassicDeEsser a,b;a.prepare(sr);b.prepare(sr);a.set(6500,-30,0,65,false,false);for(int i=0;i<1000;++i){float x=.3f*std::sin(i*.7f);auto y=a.process(x,-x);require(y[0]==x&&y[1]==-x);}++n;
 a.set(6500,-36,12,65,false,false);b.set(6500,-36,12,65,false,false);a.reset();b.reset();double in=0,out=0;
 for(int i=0;i<sr/2;++i){float x=.2f*std::sin(i*6.283185307179586*std::min(7500.,sr*.40)/sr);auto y=a.process(x,-x);if(i%127==0)b.set(6500,-36,12,65,false,false);auto z=b.process(x,-x);require(y==z&&y[0]==-y[1]&&std::isfinite(y[0]));if(i>sr/4){in+=x*x;out+=y[0]*y[0];}}require(out<in*.90);++n;
 a.set(NAN,NAN,NAN,NAN,true,true);a.reset();auto y=a.process(NAN,INFINITY);require(y[0]==0&&y[1]==0);++n;
 SkellAir air;air.prepare(sr);for(int i=0;i<500;++i){float x=.1f*std::sin(i*.5f);auto z=air.process(x,-x);require(z[0]==x&&z[1]==-x);}++n;
 OversampledSaturation s;s.set(.5f,0,.3f);s.prepare(sr);for(int i=0;i<64;++i){auto z=s.process(i==0?.3f:0,i==0?-.2f:0);require(z[0]==(i==24?.3f:0)&&z[1]==(i==24?-.2f:0));}++n;
 }std::cout<<n<<" shared vocal DSP checks passed\n";}
