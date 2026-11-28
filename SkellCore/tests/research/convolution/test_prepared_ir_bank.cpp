#include "PreparedIRBank.h"
#include <cassert>
#include <cstdlib>
#include <new>
#include <random>
#include <limits>
#include <iostream>
using namespace skellcore::experimental::convolution;
static std::size_t allocations=0;
#if defined(__GNUC__) || defined(__clang__)
#define SKELL_TEST_NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#define SKELL_TEST_NOINLINE __declspec(noinline)
#else
#define SKELL_TEST_NOINLINE
#endif
SKELL_TEST_NOINLINE void* operator new(std::size_t n){++allocations;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
SKELL_TEST_NOINLINE void operator delete(void* p) noexcept {std::free(p);}
SKELL_TEST_NOINLINE void operator delete(void* p,std::size_t) noexcept {std::free(p);}
int main(){
    static_assert(PreparedIRBank::bufferingLatency==0);
    PreparedIRBank unprepared;assert((unprepared.tick({1,1})==IRFrame{}));assert(!unprepared.request(0));
    Impulse impulse(4801);impulse[0]={.2,.03,.1,.25};impulse[4800]={.2,.03,.1,.25};assert(shapeIR(impulse,48000)==impulse);
    IRShape shape;shape.earlyMs=10;shape.transitionMs=5;shape.earlyGain=.5;shape.lateGain=.25;shape.extraDecayDbPerSecond=20;auto shaped=shapeIR(impulse,48000,shape);assert(std::abs(shaped[0][0]-.1)<1e-15);assert(std::abs(shaped[4800][0]-.05*std::pow(10.,-.1))<1e-15);
    shape.width=0.;auto mono=shapeIR(impulse,48000,shape);for(auto t:mono)assert(t[0]==t[2]&&t[1]==t[3]);
    shape={};shape.lateLowpassHz=2000;shape.earlyMs=0.;shape.transitionMs=1.;Impulse lp(400);lp[100]={.5,0,0,.5};auto filtered=shapeIR(lp,48000,shape);assert(filtered[100][0]<.5&&filtered[101][0]>0);
    try{shapeIR(impulse,std::numeric_limits<double>::quiet_NaN());assert(false);}catch(const std::invalid_argument&){}
    std::vector<Impulse> irs{Impulse(3001),Impulse(4129),Impulse(127)};
    std::mt19937 rng(775);std::uniform_real_distribution<double> u(-.4,.4);
    for(unsigned k=0;k<irs.size();++k){for(auto j:{0u,7u,63u,64u,126u,1023u,1024u,2047u,2048u,3000u,4128u})if(j<irs[k].size())for(auto& v:irs[k][j])v=u(rng)*.2;}
    double worst=0.;std::size_t calls=0;
    for(double fs:{8000.,44100.,48000.,96000.,192000.}){
        PreparedIRBank a,b;a.prepare(irs,fs,5);b.prepare(irs,fs,5);const auto f=a.fadeFrames();
        const std::size_t t1=100,t2=t1+f+irs[0].size()+PreparedIRBank::drainGuard,t3=t2+f+irs[1].size()+PreparedIRBank::drainGuard;
        const std::size_t n=t3+f+irs[2].size()+PreparedIRBank::drainGuard+1000;
        std::vector<IRFrame> x(n),y(n),z(n),expected(n);for(std::size_t i=0;i<n-500;++i)x[i]={u(rng),u(rng)};
        // Independent sparse direct FIR oracle with explicit input-weight schedule.
        for(std::size_t i=0;i<n;++i){std::array<double,3>w{};
            if(i<t1)w[0]=1.;else if(i<t1+f){w[1]=double(i-t1+1)/f;w[0]=1.-w[1];}
            else if(i<t2)w[1]=1.;else if(i<t2+f){w[2]=double(i-t2+1)/f;w[1]=1.-w[2];}
            else if(i<t3)w[2]=1.;else if(i<t3+f){w[0]=double(i-t3+1)/f;w[2]=1.-w[0];}else w[0]=1.;
            for(unsigned k=0;k<3;++k)if(w[k]!=0.)for(std::size_t j=0;j<irs[k].size()&&i+j<n;++j){const auto& h=irs[k][j];if(h==IRTap{})continue;auto& out=expected[i+j];out[0]+=w[k]*(x[i][0]*h[0]+x[i][1]*h[1]);out[1]+=w[k]*(x[i][0]*h[2]+x[i][1]*h[3]);}
        }
        const std::array<std::size_t,4> events{t1,t1+f/2,t2+f/2,t2+f};const std::array<unsigned,4> indices{1,2,1,0};
        // Last request wins: request 1 then 0 during the second transition/drain.
        const auto before=allocations;
        for(std::size_t i=0;i<n;++i){for(unsigned j=0;j<events.size();++j)if(i==events[j])assert(a.request(indices[j]));y[i]=a.tick(x[i]);++calls;}
        for(std::size_t pos=0;pos<n;){for(unsigned j=0;j<events.size();++j)if(pos==events[j])assert(b.request(indices[j]));std::size_t end=std::min(n,pos+127);for(auto e:events)if(e>pos)end=std::min(end,e);b.process(x.data()+pos,z.data()+pos,end-pos);pos=end;}
        assert(allocations==before);assert(a.active()==0&&b.active()==0&&!a.busy()&&!b.busy());
        for(std::size_t i=0;i<n;++i){assert(y[i]==z[i]);for(unsigned c=0;c<2;++c){worst=std::max(worst,std::abs(y[i][c]-expected[i][c]));assert(std::abs(y[i][c]-expected[i][c])<2e-12);}}
        assert(!a.request(3));auto v=a.tick({std::numeric_limits<double>::quiet_NaN(),0});assert(std::isfinite(v[0]));
        auto bad=irs;bad[0][0][0]=std::numeric_limits<double>::infinity();try{a.prepare(bad,fs);assert(false);}catch(const std::invalid_argument&){}assert(a.active()==0);
        std::cout<<fs<<" frames "<<n<<'\n';
    }
    std::cout<<"frames "<<calls<<" max error "<<worst<<" zero observed scalar new in processing\n";
}
