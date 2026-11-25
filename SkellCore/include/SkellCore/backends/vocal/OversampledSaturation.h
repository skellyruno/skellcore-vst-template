#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace skell::research {
// Original 4x polyphase nonlinear residual shaper, 24 samples fixed latency.
struct OversampledSaturation {
    static constexpr int latency=24,taps=97;
    inline static constexpr std::array<float,taps> coeff{{
        -3.634317735e-20f, -2.981577341e-05f, -6.507523809e-05f, -6.678904976e-05f, 6.438340332e-19f, 0.0001252385374f,
        0.0002329090761f, 0.0002122787007f, -4.660355756e-19f, -0.000336288492f, -0.0005870959611f, -0.0005069869586f,
        9.559169453e-19f, 0.0007355256926f, 0.001237993278f, 0.001034623517f, -1.683537687e-18f, -0.001418783015f,
        -0.002330432838f, -0.001904650135f, 2.657024199e-18f, 0.002511612528f, 0.004055320763f, 0.003262789874f,
        -3.845975251e-18f, -0.004186670369f, -0.006681284322f, -0.005319833665f, 5.178083656e-18f, 0.006711561686f,
        0.01064213753f, 0.008431881803f, -6.543994039e-18f, -0.01058622172f, -0.01679413974f, -0.01334375503f,
        7.81044627e-18f, 0.01699679994f, 0.02731535673f, 0.02209948774f, -8.839765016e-18f, -0.02984411897f,
        -0.050228112f, -0.04334055864f, 9.512087577e-18f, 0.07401257848f, 0.1581996571f, 0.2247476704f,
        0.2500103772f, 0.2247476704f, 0.1581996571f, 0.07401257848f, 9.512087577e-18f, -0.04334055864f,
        -0.050228112f, -0.02984411897f, -8.839765016e-18f, 0.02209948774f, 0.02731535673f, 0.01699679994f,
        7.81044627e-18f, -0.01334375503f, -0.01679413974f, -0.01058622172f, -6.543994039e-18f, 0.008431881803f,
        0.01064213753f, 0.006711561686f, 5.178083656e-18f, -0.005319833665f, -0.006681284322f, -0.004186670369f,
        -3.845975251e-18f, 0.003262789874f, 0.004055320763f, 0.002511612528f, 2.657024199e-18f, -0.001904650135f,
        -0.002330432838f, -0.001418783015f, -1.683537687e-18f, 0.001034623517f, 0.001237993278f, 0.0007355256926f,
        9.559169453e-19f, -0.0005069869586f, -0.0005870959611f, -0.000336288492f, -4.660355756e-19f, 0.0002122787007f,
        0.0002329090761f, 0.0001252385374f, 6.438340332e-19f, -6.678904976e-05f, -6.507523809e-05f, -2.981577341e-05f,
        -3.634317735e-20f
    }};
    float drive=1,driveTarget=1,mix=0,mixTarget=0,asym=0,asymTarget=0,smooth=0,dc=0;
    std::array<std::array<float,25>,2> input{};
    std::array<std::array<float,128>,2> wet{};
    std::array<float,2> prevIn{},prevOut{};
    int iw=0,ww=0;
    static float clean(float x) noexcept {return std::isfinite(x)?std::clamp(x,-32.f,32.f):0.f;}
    static float unit(float x) noexcept {return std::clamp(clean(x),0.f,1.f);}
    static float shape(float x) noexcept {if(x>=1.f)return 2.f/3.f;if(x<=-1.f)return -2.f/3.f;return x-x*x*x/3.f;}
    static float transfer(float x,float d,float bias) noexcept {return (shape(d*x+bias)-shape(bias))/(d*(1.f-bias*bias));}
    void set(float d,float m,float a) noexcept {driveTarget=1.f+7.f*unit(d);mixTarget=unit(m);asymTarget=.22f*unit(a);}
    void reset() noexcept {input={};wet={};prevIn={};prevOut={};iw=ww=0;drive=driveTarget;mix=mixTarget;asym=asymTarget;}
    void prepare(float fs) noexcept {fs=std::clamp(std::isfinite(fs)?fs:48000.f,8000.f,384000.f);smooth=1.f-std::exp(-1.f/(.015f*fs));dc=std::exp(-6.28318530718f*12.f/fs);reset();}
    std::array<float,2> process(float l,float r) noexcept {
        input[0][iw]=clean(l);input[1][iw]=clean(r);
        drive+=smooth*(driveTarget-drive);mix+=smooth*(mixTarget-mix);asym+=smooth*(asymTarget-asym);
        std::array<float,2> y{};
        for(int p=0;p<4;++p){
            for(int c=0;c<2;++c){
                float x=0;int j=iw;
                for(int k=p;k<taps;k+=4){x+=coeff[k]*input[c][j];if(--j<0)j=24;}
                x*=4.f;wet[c][ww]=transfer(x,drive,asym)-x;
                if(p==0){float v=0;for(int k=0;k<taps;++k)v+=coeff[k]*wet[c][(ww-k)&127];
                    const float hp=v-prevIn[c]+dc*prevOut[c];prevIn[c]=v;prevOut[c]=std::abs(hp)<1.e-20f?0.f:hp;
                    y[c]=input[c][(iw+1)%25]+mix*hp;
                }
            }
            ww=(ww+1)&127;
        }
        if(++iw==25)iw=0;
        return y;
    }
};
}
