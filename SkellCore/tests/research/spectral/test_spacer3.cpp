#include "Spacer3.h"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <limits>
using skellcore::experimental::spectral::Spacer3;
int main(){
    constexpr double pi=3.141592653589793;
    for(double fs:{44100.,48000.,96000.,192000.}){
        for(bool ms:{false,true}){
            Spacer3 p;Spacer3::Settings s;s.depth=0;s.midSide=ms;p.prepare(fs,s);
            double error=0;
            for(int i=0;i<10000;++i){float l=float(.2*std::sin(i*.07)),r=float(.3*std::cos(i*.13));auto v=p.tick(l,r,.9f,-.8f);error=std::max({error,std::abs(double(v.l-l)),std::abs(double(v.r-r))});}
            assert(error<1e-6);p.reset();auto impulse=p.tick(.5f,0,0,0);assert(impulse.l==.5f&&impulse.r==0);
        }
    }
    Spacer3 p;Spacer3::Settings s;s.attackMs=5;s.releaseMs=50;s.depth=.85;p.prepare(48000,s);
    auto measure=[&](double f,double detector){p.reset();double in=0,out=0;for(int i=0;i<48000;++i){float x=float(.3*std::sin(2*pi*f*i/48000)),y=float(.5*std::sin(2*pi*detector*i/48000));auto v=p.tick(x,x,y,y);if(i>=24000){in+=x*x;out+=v.l*v.l;}}return 10*std::log10(out/in);};
    double low=measure(100,100),high=measure(9000,100),silent=measure(100,0);std::printf("duck_100Hz_db %.6f duck_9000Hz_db %.6f silent_db %.6f\n",low,high,silent);assert(low<high-3&&std::abs(silent)<1e-5);
    p.prepare(48000,s);auto v=p.tick(std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),0,0);assert(std::isfinite(v.l)&&std::isfinite(v.r));
    p.reset();double first=0,last=0;for(int i=0;i<48000;++i){auto z=p.tick(.1f,0,i<12000?.8f:0,0);if(i==11999)first=z.l;if(i==47999)last=z.l;assert(z.r==0);}assert(first<.08&&last>.099);
    p.prepare(48000,s);volatile float sink=0;auto a=std::chrono::steady_clock::now();for(int i=0;i<480000;++i){auto z=p.tick(float(i%97)*.001f,float(i%53)*.001f,float(i%127)*.003f,float(i%41)*.001f);sink+=z.l;}auto b=std::chrono::steady_clock::now();std::printf("us_per_48_stereo_frames %.6f checksum %.4f\n",std::chrono::duration<double,std::micro>(b-a).count()/10000.,double(sink));
}
