#pragma once
// Allocation-free shared UI math extracted from Skell-Necromancer.
#include <algorithm>
#include <cmath>
#include <cstddef>
namespace skellcore::ui::runes {
struct Position {float x=0,y=0;};
inline float phaseStep(std::size_t band,float dt,float live) noexcept {
 constexpr float speed[3]{.48f,-.37f,.29f};
 return speed[band%3]*std::clamp(dt,0.f,.1f)*(.9f+.3f*std::clamp(live,0.f,1.f));
}
inline Position position(float angle,float radius,float phase,float cx=220.f,float cy=220.f) noexcept {
 const auto a=angle+phase;return {cx+std::cos(a)*radius,cy+std::sin(a)*radius};
}
inline float headingFade(float a) noexcept {
 const float t=std::clamp((-std::sin(a)-.68f)/.27f,0.f,1.f);
 const float m=std::clamp((.82f-std::abs(std::cos(a)))/.42f,0.f,1.f);
 return 1.f-.64f*t*t*(3.f-2.f*t)*m*m*(3.f-2.f*m);
}
inline float quietFade(float a) noexcept {
 const float s=std::clamp((std::sin(a)-.45f)/.5f,0.f,1.f);
 return 1.f-.85f*s*s*(3.f-2.f*s);
}
class FramePacer {
 public:
 bool shouldRender(double dt,bool forced=false) noexcept {
  constexpr double frame=1.0/30.0;
  if(forced){budget=0;return true;}
  budget+=std::clamp(dt,0.,.1);
  if(budget+1e-9<frame)return false;
  budget=std::clamp(budget-frame,0.,frame*.95);
  return true;
 }
 private:double budget=1.0/30.0;
};
}
