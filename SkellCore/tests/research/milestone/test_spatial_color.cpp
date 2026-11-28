#include "SpatialColor.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
using namespace skell_milestone;
static void check(bool ok,const char* msg){if(!ok){std::fprintf(stderr,"FAIL %s\n",msg);std::exit(1);}}
static double sideGain(double fs,double f){StereoSpace s;s.prepare(fs,3,6,200);double a=0,b=0;for(int i=0;i<int(fs);++i){double x=.1*std::sin(2*pi*f*i/fs);auto y=s.process({x,-x});if(i>=fs/2){a+=x*x;b+=y[0]*y[0];}}return 10*std::log10(b/a);}
int main(){for(double fs:{44100.,48000.,96000.,192000.}){
    StereoSpace w,n;w.prepare(fs,6,3,200);n.prepare(fs,0,0,200);ToneShape eq;eq.prepare(fs,100,5000,0,0);VocalDouble d,dry;d.prepare(fs,.65);dry.prepare(fs,0);DistancePan p,q,center;center.prepare(fs,0,3);p.prepare(fs,-.8,1);q.prepare(fs,.8,1);Clip4x c;c.prepare(9,-1,0);
    double err=0,side=0,near=0,far=0;int farFirst=-1;for(int i=0;i<int(fs*.3);++i){const Frame x={.2*std::sin(i*.12),.13*std::cos(i*.17)};auto y=w.process(x);err=std::max(err,std::abs((x[0]+x[1])-(y[0]+y[1])));check(n.process(x)==x&&eq.process(x)==x&&dry.process(x)==x,"neutral exact");auto z=d.process(x);side+=std::abs(z[0]-z[1]);auto a=p.process(i==0?1.:0.),b=q.process(i==0?1.:0.);check(a[0]==b[1]&&a[1]==b[0],"pan mirror");auto mid=center.process(x[0]);check(mid[0]==mid[1],"centered panning must remain centered");near+=a[0]*a[0];far+=a[1]*a[1];if(farFirst<0&&std::abs(a[1])>1e-8)farFirst=i;auto cl=c.process(x);check(std::isfinite(cl[0])&&std::isfinite(cl[1]),"clip finite");}
    check(err<1e-14&&side>1.&&near>far&&farFirst>0,"mono/side/ITD");double low=sideGain(fs,30),high=sideGain(fs,6000);check(low>8.5&&high>2.9&&high<3.4,"frequency-selective width");
    VocalDouble wet;wet.prepare(fs,1,18,0);int firstL=-1,firstR=-1;for(int i=0;i<int(fs*.05);++i){auto y=wet.process({i==0?1.:0.,i==0?1.:0.});if(firstL<0&&y[0]>1e-9)firstL=i;if(firstR<0&&y[1]>1e-9)firstR=i;}check(std::abs(firstL-fs*.018)<2&&std::abs(firstR-fs*.018*1.37)<2,"independent delay taps");
    ToneShape tone;tone.prepare(fs,150,5000,6,-6);double lowIn=0,lowOut=0,highIn=0,highOut=0;for(int i=0;i<int(fs);++i){double x=.1*std::sin(2*pi*30*i/fs),z=.1*std::sin(2*pi*10000*i/fs);auto y=tone.process({x,z});if(i>=fs/2){lowIn+=x*x;lowOut+=y[0]*y[0];highIn+=z*z;highOut+=y[1]*y[1];}}check(10*std::log10(lowOut/lowIn)>4.5&&10*std::log10(highOut/highIn)<-2.,"active shelves");
    DistancePan close,distant;close.prepare(fs,0,1);distant.prepare(fs,0,4);double energyClose=0,energyFar=0;for(int i=0;i<int(fs*.2);++i){double x=.1*std::sin(2*pi*500*i/fs);auto y=close.process(x),z=distant.process(x);energyClose+=y[0]*y[0];energyFar+=z[0]*z[0];}check(10*std::log10(energyFar/energyClose)<-11.5,"distance attenuation");
    Clip4x soft;soft.prepare(0,-1,.25);double prior=-1;for(int i=0;i<=2000;++i){double x=i*.001;double y=soft.direct({x,-x})[0];check(y>=prior-1e-12&&y<=db(-1)+1e-12,"soft knee monotone bounded");prior=y;}
    Clip4x impulse;impulse.prepare(0,0,0);int peakAt=-1;double peak=0;for(int i=0;i<100;++i){auto a=impulse.process({i==0?.1:0.,0});if(std::abs(a[0])>peak){peak=std::abs(a[0]);peakAt=i;}}check(peakAt==40,"clip impulse delay");
    StereoSpace invalid;invalid.prepare(NAN,NAN,INFINITY,NAN);check(invalid.process({NAN,INFINITY})==Frame{0,0},"invalid settings and input");
    std::printf("%.0f Hz: mid sum error %.3g, low/high side gain %.4f/%.4f dB, far-ear onset %d samples, clip delay %d PASS\n",fs,err,low,high,farFirst,peakAt);
    }
    Clip4x c;c.prepare(12,-1,0);double a=0,b=0,pk=0;const int fs=48000;double ar=0,ai=0,br=0,bi=0;for(int i=0;i<2*fs;++i){const double v=.8*std::sin(2*pi*11000*i/fs);auto y=c.process({v,v}),z=c.direct({v,v});pk=std::max(pk,std::abs(y[0]));if(i>=fs){const double t=2*pi*15000*i/fs;ar+=z[0]*std::cos(t);ai+=z[0]*std::sin(t);br+=y[0]*std::cos(t);bi+=y[0]*std::sin(t);}}a=std::hypot(ar,ai);b=std::hypot(br,bi);check(a>1&&b<a*.1,"selected alias suppression");std::printf("11k hard clip: 15k fold reduction %.4f dB; output sample peak %.6f (not guaranteed ceiling)\n",20*std::log10(b/a),pk);
}
