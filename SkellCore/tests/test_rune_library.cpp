#include <SkellCore/RuneGlyphCatalog.h>
#include <SkellCore/RuneOrbit.h>
#include <cmath>
#include <iostream>
int main(){
 using namespace skellcore::ui::runes;
 static_assert(totalCount==64 && originalCount==40 && egyptianCount==24);
 int a=0,b=0;
 for(int i=0;i<totalCount;++i){
  if(isEgyptian(i)){++a;if(motif(i)!=i-40)return 1;}
  else{++b;if(motif(i)!=-1)return 2;}
 }
 if(a!=24||b!=40||validIndex(64)||!validIndex(63))return 3;
 for(int hz:{30,60,75,120,144}){
  FramePacer pacer;int frames=0;
  for(int n=0;n<2*hz;++n)frames+=pacer.shouldRender(1.0/hz);
  if(frames<58||frames>62)return 4;
 }
 for(int band=0;band<3;++band){
  float phase=0.f;const auto first=position(.3f,175.f,0,123,456);
  for(int i=0;i<30;++i)phase+=phaseStep(band,1.f/30.f,0.f);
  const auto last=position(.3f,175.f,phase,123,456);
  if(std::hypot(last.x-first.x,last.y-first.y)<40.f)return 5;
 }
 if(std::abs(headingFade(-1.5707963f)-.36f)>.01f)return 6;
 if(std::abs(quietFade(1.5707963f)-.15f)>.01f)return 7;
 std::cout<<"PASS: 40 legacy + 24 new signs and moving 30 Hz geometry\n";
}
