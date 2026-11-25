#include <SkellCore/MatchModulation.h>
#include <cstdio>
#include <cstdlib>
#include <limits>
using M=skellcore::dsp::MatchModulation;
#define CHECK(b) do { if(!(b)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#b);std::abort();} }while(false)
int main(){for(double sr:{44100.,48000.,96000.,192000.}){
 M m;M::Parameters p;p.centerMs=10.;p.depthMs=0.;p.wet=.5f;p.dry=1.f;CHECK(m.prepare(sr,p));CHECK(m.latencySamples()==0);auto o=m.process({1.f,0.f});CHECK(o.left==1.f&&o.right==0.f);const auto n=static_cast<unsigned>(sr*.01);for(unsigned i=1;i<n;++i){o=m.process({});CHECK(o.left==0.f&&o.right==0.f);}o=m.process({});CHECK(std::abs(o.left-.5f)<1.e-6f&&o.right==0.f);
 p.depthMs=3.;p.rateHz=.8;p.feedback=.3f;CHECK(m.prepare(sr,p));auto render=[&](std::size_t block){m.reset();std::vector<M::Frame> a(static_cast<std::size_t>(sr*.2));for(std::size_t s=0;s<a.size();s+=block)for(std::size_t i=s;i<std::min(s+block,a.size());++i){float x=.2f*std::sin(float(i)*.03f);a[i]=m.process({x,x});CHECK(std::isfinite(a[i].left)&&std::isfinite(a[i].right));}return a;};auto a=render(1);double side=0.;for(auto v:a)side+=std::abs(v.left-v.right);CHECK(side>1.);for(auto b:{64u,256u,511u,1024u}){auto c=render(b);for(std::size_t i=0;i<a.size();++i)CHECK(a[i].left==c[i].left&&a[i].right==c[i].right);}p.depthMs=20.;CHECK(!m.prepare(sr,p));m.reset();o=m.process({std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()});CHECK(o.left==0.f&&o.right==0.f);
 }std::puts("MatchModulation PASS: four rates, five schedules, impulse, moving stereo, bounds and reset");}
