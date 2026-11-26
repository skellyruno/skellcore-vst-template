#include "SpatialRefinement.h"
#include <cstdio>
#include <cstdlib>
#include <random>
using namespace skell_milestone;
static void check(bool b,const char* msg){if(!b){std::fprintf(stderr,"FAIL: %s\n",msg);std::exit(1);}}
int main(){for(double fs:{44100.,48000.,96000.,192000.}){
    GuardedWidth w;w.prepare(fs,6,6,200,-1,0);MovingWobble zero,a,b;a.prepare(fs,.4,7,3,1.6,.35,1);b.prepare(fs,.4,7,3,1.6,.35,1);zero.prepare(fs,.4,7,3,1.6,.35,0);
    double mid=0,side=0,err=0,peak=0;std::mt19937 rng(46);std::uniform_real_distribution<double> u(-1,1);
    for(int i=0;i<int(fs*3);++i){Frame x{u(rng),u(rng)};auto y=w.process(x);err=std::max(err,std::abs((y[0]+y[1])-(x[0]+x[1])));check(std::abs(y[0])<=1.+1e-12&&std::abs(y[1])<=1.+1e-12,"width headroom");check(zero.process(x)==x,"wobble dry");double v=.2*std::sin(2*pi*440*i/fs);auto z=a.process({v,v});check(z==b.process({v,v}),"wobble deterministic reset");check(std::abs(z[0]+z[1]-2*v)<1e-12,"wobble mid unchanged");mid+=v*v;side+=.25*(z[0]-z[1])*(z[0]-z[1]);peak=std::max({peak,std::abs(z[0]),std::abs(z[1])});}
    check(err<1e-14&&side>mid*.3&&peak<1.,"moving stereo energy");
    MovingWobble moving;moving.prepare(fs,.4,7,3,1.6,.35,0);double jump=0,previous=0;
    for(int i=0;i<int(fs*2);++i){if(i==int(fs*.2))moving.setParameters(8,24,12,2,1,1);if(i==int(fs))moving.setParameters(.05,1,12,.3,0,0);double v=.02*std::sin(2*pi*200*i/fs);auto y=moving.process({v,v});check(std::isfinite(y[0])&&std::abs(y[0]+y[1]-2*v)<1e-12,"automation linked mid");jump=std::max(jump,std::abs(y[0]-previous));previous=y[0];if(i>int(fs*1.7))check(y==Frame{v,v},"automation settles exact bypass");}
    check(jump<.08,"bounded automation step");moving.reset();check(moving.process({.2,-.3})==Frame{.2,-.3},"reset current controls");
    ReconstructionGuard g;g.prepare(fs,-1,.65,60);for(int i=0;i<100;++i){auto y=g.process({i==0?.1:0.,0});check(y[0]==(i==24?.1:0.)&&y[1]==0.,"quiet impulse and latency");}
    Clip4x clip;clip.prepare(12,-1,0);g.prepare(fs,-1,.65,60);double pre=0,post=0;
    for(int i=0;i<int(fs);++i){const double v=.8*std::sin(2*pi*11000*i/fs);auto x=clip.process({v,-v});auto y=g.process(x);check(std::isfinite(y[0])&&y[1]==-y[0],"linked stereo");pre=std::max(pre,std::abs(x[0]));post=std::max(post,std::abs(y[0]));}check(post<=db(-1)+1e-12&&post>0.1,"guard ceiling sample");
    auto z=g.process({NAN,INFINITY});check(std::isfinite(z[0])&&std::isfinite(z[1]),"nonfinite input");
    std::printf("%.0f Hz: guarded width mid error %.3g, wobble side/mid %.4f dB, clip peak %.6f -> %.6f, total delay 64 PASS\n",fs,err,10*std::log10(side/mid),pre,post);
}}
