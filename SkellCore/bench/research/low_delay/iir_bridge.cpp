#include "LowDelayColor.h"
using E=skellcore::experimental::spectral::LowDelayColor;
extern "C" int color_iir(const double* x,double* y,unsigned n,double fs,double drive,double trim){E e;if(!e.prepare(fs,drive,trim))return 0;for(unsigned i=0;i<n;++i){auto v=e.process({x[2*i],x[2*i+1]});y[2*i]=v[0];y[2*i+1]=v[1];}return 1;}
