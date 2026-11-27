#include <SkellCore/backends/vocal/AlignedSpectralPitchFormant.h>
#include <SkellCore/backends/vocal/SkellAir.h>
#include <SkellCore/backends/vocal/ClassicDeEsser.h>
#include <SkellCore/backends/vocal/OversampledSaturation.h>
#include <iostream>
#include <cstdlib>
#include <cmath>
using namespace skell::research;
void require(bool x){if(!x)std::abort();}
void checkAlignedFormant(){int n=0;for(int sr:{8000,44100,48000,96000,192000}){
 for(float f:{0.f,.01f,-6.f,6.f}){
  AlignedSpectralPitchFormant p;p.prepare(sr);p.set(.5f,.5f+f/24.f,1,1);const int onset=std::max(8192,sr/5);float peak=0;int peakIndex=-1;
  for(int i=0;i<onset+4096;++i){auto y=p.process(i==onset?.5f:0.f,i==onset?-.25f:0.f);require(std::isfinite(y[0])&&std::isfinite(y[1]));require(std::abs(y[0]+2*y[1])<1e-5f);if(std::abs(y[0])>peak){peak=std::abs(y[0]);peakIndex=i;}if(f==0)require(y[0]==(i==onset+1024?.5f:0.f));}
  require(peakIndex==onset+AlignedSpectralPitchFormant::latencySamples);++n;
 }
 for(float pitch:{0.f,12.f}){AlignedSpectralPitchFormant p;p.prepare(sr);p.set(.5f+pitch/24.f,.25f,1,1);int crosses=0;float last=0;for(int i=0;i<sr*2;++i){auto y=p.process(.2f*std::sin(6.283185307179586*220*i/sr),0);if(i>=sr){if(last<=0&&y[0]>0)++crosses;}last=y[0];require(std::isfinite(y[0]));}require(std::abs(crosses-(pitch==0?220:440))<=3);++n;}
 }std::cout<<n<<" aligned formant checks passed\n";
}
int main(){checkAlignedFormant();int n=0;for(int sr:{8000,44100,48000,96000,192000,384000}){
 ClassicDeEsser a,b;a.prepare(sr);b.prepare(sr);a.set(6500,-30,0,65,false,false);for(int i=0;i<1000;++i){float x=.3f*std::sin(i*.7f);auto y=a.process(x,-x);require(y[0]==x&&y[1]==-x);}++n;
 a.set(6500,-36,12,65,false,false);b.set(6500,-36,12,65,false,false);a.reset();b.reset();double in=0,out=0;
 for(int i=0;i<sr/2;++i){float x=.2f*std::sin(i*6.283185307179586*std::min(7500.,sr*.40)/sr);auto y=a.process(x,-x);if(i%127==0)b.set(6500,-36,12,65,false,false);auto z=b.process(x,-x);require(y==z&&y[0]==-y[1]&&std::isfinite(y[0]));if(i>sr/4){in+=x*x;out+=y[0]*y[0];}}require(out<in*.90);++n;
 a.set(NAN,NAN,NAN,NAN,true,true);a.reset();auto y=a.process(NAN,INFINITY);require(y[0]==0&&y[1]==0);++n;
 SkellAir air;air.prepare(sr);for(int i=0;i<500;++i){float x=.1f*std::sin(i*.5f);auto z=air.process(x,-x);require(z[0]==x&&z[1]==-x);}++n;
 OversampledSaturation s;s.set(.5f,0,.3f);s.prepare(sr);for(int i=0;i<64;++i){auto z=s.process(i==0?.3f:0,i==0?-.2f:0);require(z[0]==(i==24?.3f:0)&&z[1]==(i==24?-.2f:0));}++n;
 }std::cout<<n<<" shared vocal DSP checks passed\n";}
