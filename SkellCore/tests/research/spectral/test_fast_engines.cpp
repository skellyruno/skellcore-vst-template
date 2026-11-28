#include "SpacerStereoSIMD.h"
#include "ColorOversampled.h"
#include "ColorContiguous.h"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <vector>
#include <limits>
using namespace skellcore::experimental::spectral;
static uint32_t rng=1234567;
static double rnd(){rng=1664525u*rng+1013904223u;return double(rng)/2147483648.-1.;}
struct Input {float l,r,sl,sr;};
using Clock=std::chrono::steady_clock;
volatile double sink;
template<class T> double benchSpacer(const std::vector<Input>& in){T p;p.prepare(48000,{});double sum=0;auto t=Clock::now();for(auto a:in){auto v=p.tick(a.l,a.r,a.sl,a.sr);sum+=v[0]+v[1];}sink=sum;return std::chrono::duration<double,std::micro>(Clock::now()-t).count()*48/in.size();}
template<class T> double benchColor(const std::vector<Input>& in){T p;p.prepare(48000,12,.12,1,0);double sum=0;auto t=Clock::now();for(auto a:in){auto v=p.process({a.l,a.r});sum+=v.l+v.r;}sink=sum;return std::chrono::duration<double,std::micro>(Clock::now()-t).count()*48/in.size();}
template<class A,class B> void compareColor(double fs,const std::vector<Input>& in){A a;B b; a.prepare(fs,12,.12,1,0);b.prepare(fs,12,.12,1,0);double error=0;for(auto x:in){auto y=a.process({x.l,x.r}),z=b.process({x.l,x.r});error=std::max({error,std::abs(y.l-z.l),std::abs(y.r-z.r)});}assert(error<1e-12);std::printf("color %.0f %d error %.12g\n",fs,A::factor,error); a.prepare(fs,12,.12,0,0);b.prepare(fs,12,.12,0,0);for(int n=0;n<100;++n){auto y=a.process({n==0?1.:0.,0}),z=b.process({n==0?1.:0.,0});assert(y.l==z.l&&z.l==(n==B::latency?1.:0.)&&z.r==0);}}
int main(int argc,char**){const bool quick=argc>1;std::vector<Input> in(quick?12000:240000);for(auto& a:in)a={float(.8*rnd()),float(.8*rnd()),float(.7*rnd()),float(.7*rnd())};
 for(double fs:{44100.,48000.,96000.,192000.}){for(bool ms:{false,true}){SpacerSelective a;SpacerStereoSIMD b;SpacerSelective::Settings s;s.midSide=ms;a.prepare(fs,s);b.prepare(fs,s);double err=0;for(auto v:in){auto y=a.tick(v.l,v.r,v.sl,v.sr),z=b.tick(v.l,v.r,v.sl,v.sr);err=std::max({err,std::abs(double(y[0]-z[0])),std::abs(double(y[1]-z[1]))});}assert(err<1e-6);std::printf("spacer %.0f ms=%d error %.12g\n",fs,ms,err);b.reset();auto z=b.tick(.5f,0,0,0);assert(z[0]==.5f&&z[1]==0);z=b.tick(std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),0,0);assert(std::isfinite(z[0])&&std::isfinite(z[1]));}
 compareColor<ColorOversampled2x,ColorContiguous2x>(fs,in);compareColor<ColorOversampled,ColorContiguous4x>(fs,in);}
 if(!quick)for(int rep=0;rep<3;++rep){double a=benchSpacer<SpacerSelective>(in),b=benchSpacer<SpacerStereoSIMD>(in),c=benchColor<ColorOversampled>(in),d=benchColor<ColorContiguous4x>(in);std::printf("bench_us48 %.6f %.6f %.6f %.6f\n",a,b,c,d);}
}
