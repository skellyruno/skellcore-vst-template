#include "SpacerExtended.h"
#include "SpacerStereoSIMD.h"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <vector>
#include <cstdint>
#include <cstring>
#include <limits>
using namespace skellcore::experimental::spectral;
struct Input{float l,r,sl,sr;};
static uint32_t seed=12345;
float rnd(){seed=1664525u*seed+1013904223u;return float(double(seed)/2147483648.-1.);}
volatile double sink;
template<class T> double bench(const std::vector<Input>& in){T p;p.prepare(48000,{});double sum=0;auto start=std::chrono::steady_clock::now();for(auto x:in){auto v=p.tick(x.l,x.r,x.sl,x.sr);sum+=v[0]+v[1];}sink=sum;return std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count()*48/in.size();}
template<class T> double measure(int fs,int main,int sc){T p;p.prepare(fs,{});double a=0,b=0;for(int i=0;i<fs;++i){double t=double(i)/fs;float x=float(.25*std::sin(6.283185307179586*main*t)),s=float(.5*std::sin(6.283185307179586*sc*t));auto v=p.tick(x,x,s,0);assert(v[1]==x);if(i>=fs/2){a+=double(x)*x;b+=double(v[0])*v[0];}}return 10*std::log10(b/a);}
int main(int argc,char**argv){bool quick=argc>1&&!std::strcmp(argv[1],"quick");std::vector<Input> in(quick?12000:120000);for(auto&v:in)v={rnd(),rnd(),rnd(),rnd()};
 for(int fs:{44100,48000,96000,192000}){for(bool ms:{false,true}){SpacerExtended<false> a;SpacerExtended<true>b;SpacerExtended<>::Settings s;s.midSide=ms;a.prepare(fs,s);b.prepare(fs,s);double e=0;for(auto x:in){auto y=a.tick(x.l,x.r,x.sl,x.sr),z=b.tick(x.l,x.r,x.sl,x.sr);for(int k=0;k<2;++k)e=std::max(e,std::abs(double(y[k])-z[k]));}assert(e<1e-6);a.reset();b.reset();for(int i=0;i<fs;++i){float v=i==0?.8f:0.f;auto y=a.tick(v,-v,v,-v),z=b.tick(v,-v,v,-v);assert(std::isfinite(z[0])&&std::isfinite(z[1]));assert(std::abs(double(y[0])-z[0])<1e-6&&std::abs(double(y[1])-z[1])<1e-6);}b.reset();auto zero=b.tick(std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),0,0);assert(zero[0]==0&&zero[1]==0);s.depth=0;b.prepare(fs,s);for(auto x:in){auto z=b.tick(x.l,x.r,x.sl,x.sr);assert(z[0]==x.l&&z[1]==x.r);}printf("parity %d %d %.12g\n",fs,ms,e);}
 if(!quick)for(int f:{20,25,30,40,45,63,100,1000,9000,16000}){double a=measure<SpacerStereoSIMD>(fs,f,f),b=measure<SpacerExtended<>>(fs,f,f);printf("sweep %d %d %.8f %.8f\n",fs,f,a,b);assert(b<-3.5);}
 }
 if(!quick){for(int j=0;j<3;++j){double a=bench<SpacerStereoSIMD>(in),b=bench<SpacerExtended<false>>(in),c=bench<SpacerExtended<true>>(in);printf("bench %.6f %.6f %.6f\n",a,b,c);}for(int d:{20,30,1000,9000})for(int f:{20,30,100,1000,9000})printf("matrix %d %d %.8f\n",d,f,measure<SpacerExtended<>>(48000,f,d));}
}
