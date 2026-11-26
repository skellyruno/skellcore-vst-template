#include "MotionField.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
using namespace skell_milestone;
using M=MotionField;
static void check(bool x,const char* m){if(!x){std::fprintf(stderr,"FAIL %s\n",m);std::exit(1);}}
int main(){for(double fs:{44100.,48000.,96000.,192000.}){
    M m;m.prepare(fs);M::Settings p;p.mode=M::Mode::Manual;p.start=0;p.depth=0;p.level=1;m.set(0,p);p.start=1;m.set(1,p);m.setMix(1);m.reset();Frame y{};
    for(int i=0;i<int(fs*.05);++i){y=m.process({.4,-.2});}check(std::abs(y[0]-.2)<1e-9&&std::abs(y[1]+.1)<1e-9,"independent hard pan");
    m.setWetTrim(6);for(int i=0;i<int(fs*.05);++i)y=m.process({.4,-.2});check(std::abs(y[0]-.2*db(6))<1e-9,"wet trim");m.setWetTrim(0);
    m.setMix(0);for(int i=0;i<int(fs*.02);++i)y=m.process({.4,-.2});check(y==Frame{.4,-.2},"exact dry");
    p={};p.mode=M::Mode::Tempo;p.bpm=120;p.beats=2;p.wave=M::Wave::Ramp;p.depth=0;m.set(0,p);m.reset();for(int i=0;i<int(fs*.25);++i)m.process({.2,0});check(std::abs(m.cycle(0)-.25)<1e-8,"tempo quarter cycle");p.bpm=60;m.set(0,p);for(int i=0;i<int(fs*.5);++i)m.process({.2,0});check(std::abs(m.cycle(0)-.5)<1e-8,"tempo change continuous");
    p={};p.seconds=.1;p.external=true;p.trigger=M::Trigger::Gate;p.wave=M::Wave::Ramp;m.set(0,p);m.reset();for(int i=0;i<int(fs*.02);++i)m.process({.8,0});check(m.cycle(0)==0,"external no key");for(int i=0;i<int(fs*.02);++i)m.process({0,0},{.8,0});check(m.triggered(0)&&m.cycle(0)>.1,"external gate");for(int i=0;i<int(fs*.1);++i)m.process({0,0});const auto frozen=m.cycle(0);for(int i=0;i<int(fs*.03);++i)m.process({0,0});check(m.cycle(0)==frozen,"gate freezes");
    p.trigger=M::Trigger::Cycle;m.set(0,p);m.reset();for(int i=0;i<int(fs*.01);++i)m.process({0,0},{.8,0});for(int i=0;i<int(fs*.25);++i)m.process({0,0});check(m.cycle(0)==0,"one cycle stops after quiet");
    p.trigger=M::Trigger::Return;m.set(0,p);m.reset();for(int i=0;i<int(fs*.02);++i)m.process({0,0},{.8,0});for(int i=0;i<int(fs*.15);++i)m.process({0,0});check(m.cycle(0)==0,"return to start");
    p.trigger=M::Trigger::Latch;m.set(0,p);m.reset();for(int i=0;i<int(fs*.01);++i)m.process({0,0},{.8,0});for(int i=0;i<int(fs*.08);++i)m.process({0,0});check(m.cycle(0)>.7,"latch continues");
    p.trigger=M::Trigger::Toggle;p.holdMs=10;m.set(0,p);m.reset();for(int i=0;i<int(fs*.005);++i)m.process({0,0},{.8,0});check(m.position(0)<-.9,"hold delays toggle");for(int i=0;i<int(fs*.02);++i)m.process({0,0},{.8,0});check(m.position(0)>.9,"toggle endpoint");
    p={};p.mode=M::Mode::Manual;p.start=0;p.depth=0;p.predelayMs=10;m.set(0,p);p.level=0;m.set(1,p);m.setMix(1);m.reset();int onset=-1;for(int i=0;i<int(fs*.03);++i){y=m.process({i==0?.5:0.,0});if(onset<0&&std::abs(y[0])>1e-7)onset=i;}check(onset==int(fs*.01),"fixed predelay");
    p.level=1;p.predelayMs=0;p.path=M::Path::PhaseOrbit;p.start=.5;p.depth=0;m.set(0,p);m.reset();for(int i=0;i<int(fs*.05);++i)y=m.process({.4,0});check(y[0]>.13&&y[1]<-.13,"phase orbit");
    p.path=M::Path::Line;p.start=0;p.farDb=-12;p.shelfHz=500;m.set(0,p);m.reset();double e=0;for(int i=0;i<int(fs*.1);++i){y=m.process({.2*std::sin(2*pi*6000*i/fs),0});if(i>=fs*.05)e+=y[0]*y[0];}check(e/(fs*.05)<.001,"motion shelf attenuation");
    p.farDb=0;p.dynamics=M::Dynamics::Wet;p.dirt=1;p.driveDb=12;m.set(0,p);m.reset();for(int i=0;i<int(fs*.05);++i)y=m.process({.9,0});check(std::abs(y[0])<.07,"wet clip morph");
    p={};p.mode=M::Mode::Input;p.inputSensitivity=2;p.depth=0;m.set(0,p);m.reset();for(int i=0;i<int(fs*.05);++i)m.process({.4,0});check(m.position(0)>.5,"input movement");
    p.mode=M::Mode::Free;p.path=M::Path::Pulse;p.seconds=.1;m.set(0,p);m.reset();double peak=0;for(int i=0;i<int(fs*.2);++i){y=m.process({.4,0});check(std::isfinite(y[0])&&std::isfinite(y[1]),"finite pulse");peak=std::max({peak,std::abs(y[0]),std::abs(y[1])});}check(peak>.15&&peak<=.2,"pulse bounded");
    p.seconds=NAN;p.shelfHz=INFINITY;p.rangeA=NAN;p.rangeB=-1;m.set(0,p);y=m.process({NAN,INFINITY});check(std::isfinite(y[0])&&std::isfinite(y[1]),"bad input");
    std::printf("%.0f Hz: independent lanes, tempo, six triggers, phase orbit, shelf, clip, dry, predelay %d PASS\n",fs,onset);
}}
