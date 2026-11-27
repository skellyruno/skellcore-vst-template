#include <SkellCore/ClubGate.h>
#include <array>
#include <chrono>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <limits>
using namespace skellcore::dsp;
// Native-endian IEEE float32 stereo transport; no WAV conversion in DSP.
int main(int argc,char** argv){
 try{
  static_assert(sizeof(float)==4 && std::numeric_limits<float>::is_iec559);
  if(argc==2 && std::string(argv[1])=="--version"){std::cout<<"club-gate-v1\n";return 0;}
  if(argc!=13)throw std::runtime_error("usage: render input.f32 output.f32 rate threshold attack hold release hysteresis finish(0/1) inputDb outputDb block");
  auto parse=[&](int i,double lo,double hi){size_t n=0;std::string s=argv[i];double v=std::stod(s,&n);if(n!=s.size()||!std::isfinite(v)||v<lo||v>hi)throw std::runtime_error("invalid parameter");return v;};
  const double sr=parse(3,8000,192000),th=parse(4,-75,-15),at=parse(5,.5,25),hold=parse(6,0,250),rel=parse(7,20,600),hy=parse(8,0,12),fi=parse(9,0,1),in=parse(10,-24,24),out=parse(11,-24,24),bl=parse(12,1,8192);
  if(fi!=std::floor(fi)||bl!=std::floor(bl)||sr!=std::floor(sr))throw std::runtime_error("integer parameter required");
  if(std::filesystem::exists(argv[2]) || std::string(argv[1])==argv[2])throw std::runtime_error("output already exists or input/output collision");
  std::ifstream src(argv[1],std::ios::binary|std::ios::ate);if(!src)throw std::runtime_error("input open");
  auto bytes=src.tellg();if(bytes<=0||bytes%8||bytes>std::streamoff(192000)*2*4*600)throw std::runtime_error("bad input length");
  std::vector<float> x(static_cast<size_t>(bytes)/4);src.seekg(0);src.read(reinterpret_cast<char*>(x.data()),bytes);if(!src)throw std::runtime_error("read failed");
  for(float v:x)if(!std::isfinite(v))throw std::runtime_error("nonfinite input");
  MicNoiseGate gate;ClubGateFinish finish;gate.prepare(sr);finish.prepare(sr);gate.set(float(th),float(at),float(hold),float(rel),float(hy));finish.setThresholdDb(float(th)-11.4f);
  const float ig=std::pow(10.f,float(in)/20.f),og=std::pow(10.f,float(out)/20.f);const auto start=std::chrono::steady_clock::now();
  for(size_t b=0;b<x.size()/2;b+=size_t(bl))for(size_t i=b;i<std::min(x.size()/2,b+size_t(bl));++i){float l=x[2*i]*ig,r=x[2*i+1]*ig;float g=clubGateGain(gate,finish,fi==1,l,r);x[2*i]=l*g*og;x[2*i+1]=r*g*og;}
  auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
  for(float v:x)if(!std::isfinite(v))throw std::runtime_error("nonfinite output");
  std::ofstream dst(argv[2],std::ios::binary|std::ios::trunc);if(!dst)throw std::runtime_error("output open");dst.write(reinterpret_cast<const char*>(x.data()),bytes);if(!dst)throw std::runtime_error("write failed");
  std::cout<<"{\"frames\":"<<x.size()/2<<",\"processing_seconds\":"<<seconds<<",\"latency_samples\":0}\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
