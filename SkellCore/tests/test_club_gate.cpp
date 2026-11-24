#include <SkellCore/ClubGate.h>
#include <iostream>
using namespace skellcore::dsp;
int main(){for(double sr:{44100.,48000.,96000.,192000.}){
 MicNoiseGate a,b;ClubGateFinish fa,fb;a.prepare(sr);b.prepare(sr);fa.prepare(sr);fb.prepare(sr);fa.setThresholdDb(-57.4f);fb.setThresholdDb(-57.4f);
 for(int i=0;i<int(sr);++i){float x=i>sr*.1&&i<sr*.5?.12f:0.f;float u=clubGateGain(a,fa,true,x,-x),v=clubGateGain(b,fb,true,x,-x);if(u!=v||!std::isfinite(u)||u<0||u>1)return 1;if(i==int(sr*.4)&&u<.98f)return 2;}
 if(a.isOpen()||b.isOpen())return 3;
 a.prepare(sr);fa.prepare(sr);if(!std::isfinite(clubGateGain(a,fa,true,NAN,INFINITY)))return 4;
 }std::cout<<"4 sample rates, deterministic, opening/release and finite checks passed\n";return 0;}
