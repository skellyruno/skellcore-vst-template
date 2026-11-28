#include "OriginalSpace.h"
#include <cstdio>
#include <cstdlib>
using namespace skellcore::experimental::dynamics;
int main(){double base=0.;for(double fs:{44100.,48000.,96000.,192000.}){OriginalSpace s(fs,1.8,4200,0);double early=0,late=0,tail=0;for(int i=0;i<int(fs*4);++i){const double t=i/fs;double a=0.;if(t<.25){const double w=std::sin(3.14159265358979323846*t/.25);a=.1*w*w*(std::sin(2*3.14159265358979323846*223*t)+std::sin(2*3.14159265358979323846*809*t)+std::sin(2*3.14159265358979323846*2399*t));}const auto y=s.wet({a,0});const double e=y[0]*y[0]+y[1]*y[1];if(!std::isfinite(e))return 1;if(t>=.35&&t<.65)early+=e/fs;if(t>=1.35&&t<1.65)late+=e/fs;if(t>=2.35&&t<2.65)tail+=e/fs;}if(!(early>late&&late>tail&&tail>0))return 2;const double v=10*std::log10(late/early);if(fs==44100)base=v;std::printf("%.0f Hz matched smooth multitone burst: late/early %.5f dB, tail/early %.5f dB, delta from 44.1k %.5f dB\n",fs,v,10*std::log10(tail/early),v-base);}}
