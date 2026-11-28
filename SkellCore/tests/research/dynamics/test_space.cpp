#include "OriginalSpace.h"
#include <cassert>
#include <chrono>
#include <iostream>
int main(){
  for(double fs:{44100.,48000.,96000.,192000.}){
    skellcore::experimental::dynamics::OriginalSpace a(fs,1.8,4200.,0.),b(fs,1.8,4200.,9.);
    double early=0.,late=0.;
    for(int n=0;n<int(5*fs);++n){auto x=a.wet(n==0?skellcore::experimental::dynamics::Frame{1.,0.}:skellcore::experimental::dynamics::Frame{});assert(std::isfinite(x[0])&&std::isfinite(x[1]));double e=x[0]*x[0]+x[1]*x[1];if(n<fs)early+=e;if(n>4*fs)late+=e;}
    assert(early>1e-6&&late<early*1e-6);
    a.reset();b.reset();
    for(int n=0;n<int(fs);++n){auto x=a.wet({.25,.2}),y=b.wet({.25,.2});assert(std::abs(y[0])<=std::abs(x[0])+1e-12);assert(std::abs(y[1])<=std::abs(x[1])+1e-12);}
    assert(b.wetGain()<.7);b.reset();assert(b.wetGain()==1.);assert(b.wet({})==skellcore::experimental::dynamics::Frame{});
    a.reset();auto x=a.wet({NAN,INFINITY});assert(std::isfinite(x[0])&&std::isfinite(x[1]));
  }
  skellcore::experimental::dynamics::OriginalSpace r(48000);double sink=0.;auto t=std::chrono::steady_clock::now();
  for(int n=0;n<480000;++n)sink+=r.wet({.1*std::sin(n*.01),.1*std::sin(n*.013)})[0];
  auto ns=std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-t).count();
  std::cout<<"4 sample rates passed; benchmark ns/frame="<<ns/480000<<" sink="<<sink<<"\n";
}
