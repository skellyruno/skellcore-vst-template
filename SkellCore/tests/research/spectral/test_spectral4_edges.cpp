#include "Spectral4.h"
#include <cassert>
#include <cstdio>
#include <limits>
#include <random>
int main(){
    using namespace skellcore::experimental::spectral;
    const double nan=std::numeric_limits<double>::quiet_NaN();
    const double inf=std::numeric_limits<double>::infinity();
    for(double fs:{8000.,22050.,44100.,48000.,96000.,192000.,384000.}){
        double worst=0;
        for(auto edges:{std::array<double,3>{20000,20000,20000}, {nan,inf,-inf}, {7500,100,20}, {230,2400,7500}}){
            Spectral4 e;e.prepare(fs,edges);
            std::mt19937 rng(52);std::uniform_real_distribution<double> u(-1,1);
            for(int i=0;i<12000;++i){Frame x{u(rng),u(rng)};auto y=e.process(x);assert(std::isfinite(y.l)&&std::isfinite(y.r));worst=std::max({worst,std::abs(y.l-x.l),std::abs(y.r-x.r)});}
            assert(worst<1e-10);
            e.prepare(fs,edges,{6,-3,3,-6},-24,2.,1.,-6.);
            for(int i=0;i<12000;++i){auto y=e.process({u(rng),u(rng)});assert(std::isfinite(y.l)&&std::isfinite(y.r));}
            e.reset();Spectral4 fresh;fresh.prepare(fs,edges,{6,-3,3,-6},-24,2.,1.,-6.);auto a=e.process({.5,.2}),b=fresh.process({.5,.2});assert(a.l==b.l&&a.r==b.r);
        }
        printf("%.0f Hz: 4 invalid/normal edge sets, neutral max error %.12g, finite dynamic and reset PASS\n",fs,worst);
    }
}
