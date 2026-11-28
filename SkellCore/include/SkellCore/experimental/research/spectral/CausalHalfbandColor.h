#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace skellcore::experimental::spectral {
// Fixed 4x research engine. Causal, wet-only, no live coefficient changes.
// Supply symmetric odd polyphase coefficients from the verified design.
class CausalHalfbandColor {
public:
    using Frame=std::array<double,2>;
    using First=std::array<double,320>;using Second=std::array<double,32>;
    static constexpr unsigned latency=336;
    bool prepare(const First& first,const Second& second,double drive=4.) noexcept {
        if(!valid(first)||!valid(second)||!std::isfinite(drive)||drive<.1||drive>20)return false;
        a_=first;b_=second;drive_=drive;ready_=true;reset();return true;
    }
    void reset() noexcept {channels_={};}
    Frame process(Frame x) noexcept {
        Frame y{};if(!ready_)return y;
        for(unsigned c=0;c<2;++c){
            auto& s=channels_[c];const double v=std::isfinite(x[c])?std::clamp(x[c],-4.,4.):0.;
            const auto p=s.a.up(v,a_);Frame q{};
            for(unsigned j=0;j<2;++j){const auto z=s.b.up(p[j],b_);q[j]=s.b.down(std::tanh(drive_*z[0]),std::tanh(drive_*z[1]),b_);}
            y[c]=s.a.down(q[0],q[1],a_);
        }
        return y;
    }
private:
    template<std::size_t N> static bool valid(const std::array<double,N>& h) noexcept {
        double sum=0.;for(std::size_t k=0;k<N;++k){if(!std::isfinite(h[k])||std::abs(h[k])>1||h[k]!=h[N-1-k])return false;sum+=std::abs(h[k]);}return sum<=4.;
    }
    template<std::size_t N> struct History {
        std::array<double,2*N> data{};std::size_t head=0;
        void push(double v) noexcept {head=head?head-1:N-1;data[head]=data[head+N]=v;}
        const double* samples() const noexcept {return data.data()+head;}
    };
    template<std::size_t N> struct Halfband {
        History<N> upHistory,evenHistory,oddHistory;
        static double side(const double* x,const std::array<double,N>& h) noexcept {double sum=0.;for(std::size_t k=0;k<N/2;++k)sum+=h[k]*(x[k]+x[N-1-k]);return sum;}
        Frame up(double x,const std::array<double,N>& h) noexcept {upHistory.push(x);const auto* p=upHistory.samples();return {p[N/2],2*side(p,h)};}
        double down(double a,double b,const std::array<double,N>& h) noexcept {evenHistory.push(a);const double y=.5*evenHistory.samples()[N/2]+side(oddHistory.samples(),h);oddHistory.push(b);return y;}
    };
    struct Channel {Halfband<320> a;Halfband<32> b;};
    std::array<Channel,2> channels_{};First a_{};Second b_{};double drive_=4.;bool ready_=false;
};
}
