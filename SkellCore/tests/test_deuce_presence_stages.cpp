#include <SkellCore/DeucePresenceStages.h>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <vector>
using D=skellcore::dsp::DeucePresenceStages;
#define CHECK(b) do{if(!(b)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#b);std::abort();}}while(false)
int main(){
 // Golden samples from the independent scipy Butterworth + RBJ reference.
 constexpr int indices[5]={1,53,257,4095,11999};
 constexpr double golden[5][5]={
 {0.037828108868384232,0.14985742472399335,-0.08439959577810896,-0.094130217171824432,-0.0044640970937552728},
 {0.035431390680830163,-0.097232282857411229,0.088931726217374094,0.012868465281783793,0.026727444609207091},
 {0.033992552871243253,-0.12494285962710951,-0.16412846600995132,-0.13478704053387944,-0.039344380432745421},
 {0.020112458885354286,0.10543796933956304,-0.094068492355491001,-0.085929169982158729,-0.014254295056820573},
 {0.010422955719544554,0.14339020770558469,-0.081643577482331753,-0.078194143799384427,-0.13642140978464273}};
 int rateIndex=0;for(double sr:{32000.,44100.,48000.,96000.,192000.}){
 D d;CHECK(d.prepare(sr));CHECK(d.latencySamples()==0);auto render=[&](std::size_t b){d.reset();std::vector<D::Frame> o(12000);for(std::size_t s=0;s<o.size();s+=b)for(std::size_t i=s;i<std::min(s+b,o.size());++i){double x=.15*std::sin(2.*3.141592653589793*700.*i/sr)+.02*std::sin(2.*3.141592653589793*11000.*i/sr);o[i]=d.process({x,x});CHECK(std::isfinite(o[i].left)&&o[i].left==o[i].right);}return o;};auto a=render(1);for(int i=0;i<5;++i)CHECK(std::abs(a[indices[i]].left-golden[rateIndex][i])<1.e-10);++rateIndex;for(auto b:{64u,256u,511u,1024u}){auto c=render(b);for(std::size_t i=0;i<a.size();++i)CHECK(a[i].left==c[i].left&&a[i].right==c[i].right);}CHECK(!d.prepare(22050.));d.reset();auto v=d.process({std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()});CHECK(v.left==0.&&v.right==0.);
 }std::puts("DeucePresenceStages PASS: five rates, five schedules, finite, reset, mono and rate validation");}
