#include <SkellCore/MatchToneFinish.h>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <vector>
using T=skellcore::dsp::MatchToneFinish;
#define CHECK(b) do { if(!(b)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#b);std::abort();} }while(false)
int main(){
 for(double sr:{44100.,48000.,96000.,192000.}){
  T t;T::Parameters p;CHECK(t.prepare(sr,p));CHECK(t.latencySamples()==0);
  for(int i=0;i<4096;++i){float l=.2f*std::sin(float(i)*.05f),r=.1f*std::cos(float(i)*.1f);auto a=t.process({l,r});CHECK(std::abs(a.left-l)<1.e-7f&&std::abs(a.right-r)<1.e-7f);}
  p.bandDb={-2.f,1.f,3.f,-1.f};p.gainDb=-1.f;p.sideDb=4.f;p.slope=-.2f;CHECK(t.prepare(sr,p));
  auto render=[&](std::size_t b){t.reset();std::vector<T::Frame> out(10000);for(std::size_t start=0;start<out.size();start+=b)for(std::size_t i=start;i<std::min(start+b,out.size());++i){float l=.2f*std::sin(float(i)*.04f);out[i]=t.process({l,l});CHECK(std::isfinite(out[i].left)&&out[i].left==out[i].right);}return out;};
  auto a=render(1);for(auto b:{64u,256u,511u,1024u}){auto o=render(b);for(std::size_t i=0;i<a.size();++i)CHECK(o[i].left==a[i].left&&o[i].right==a[i].right);}
  p.slope=1.f;CHECK(!t.prepare(sr,p));t.reset();auto bad=t.process({std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()});CHECK(bad.left==0.f&&bad.right==0.f);
 }std::puts("MatchToneFinish PASS: 4 rates, 5 schedules, unity, mono preservation, bounds, nonfinite");
}
