#include "WetReturn.h"
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace skellcore::vocal;
static void check(bool x){if(!x)std::abort();}
int main(){int count=0;for(double sr:{8000.,32000.,44100.,48000.,96000.,192000.}){
 WetReturn e;WetReturnSettings s;check(e.prepare(sr,s));++count;
 auto v=e.process(.25f,-.25f,1,1);check(v[0]==.25f&&v[1]==-.25f);++count;
 s.duck=true;s.thresholdDb=-24;s.ratio=4;s.attackMs=10;s.releaseMs=100;s.rangeDb=12;check(e.prepare(sr,s));++count;
 for(int i=0;i<int(sr*.2);++i)v=e.process(.25f,-.25f,1,-1);
 check(std::abs(e.reductionDb()-12)<.001&&std::abs(v[0]-.25*std::pow(10.,-.6))<1e-5&&v[0]==-v[1]);++count;
 double before=e.reductionDb();for(int i=0;i<int(sr*.1);++i)v=e.process(.25f,-.25f,0,0);
 check(std::abs(e.reductionDb()-before/std::exp(1.))<.003);++count;
 e.reset();check(e.reductionDb()==0);++count;
 auto nan=std::numeric_limits<float>::quiet_NaN();v=e.process(nan,.1f,nan,0);check(v[0]==0&&v[1]==.1f);++count;
 s.ratio=0;check(!e.prepare(sr,s)&&!e.prepare(NAN,WetReturnSettings{}));++count;
 s.ratio=1;check(e.prepare(sr,s));v=e.process(.25f,-.25f,1,1);check(v[0]==.25f&&v[1]==-.25f);++count;
 }std::cout<<count<<" wet-return checks passed\n";}
