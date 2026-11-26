#include <SkellCore/DemoTrace.h>
#include <filesystem>
#include <iostream>
#include <limits>
#include <fstream>
namespace fs=std::filesystem;
int main(){
    using namespace skellcore::demo;
    float a[]={.5f,-.5f},b[]={.25f,-.25f};std::array<float,10> cuts{};cuts.fill(6.f);
    auto levels=measure(a,2);if(levels.rms!=.5||levels.peak!=.5)return 1;
    float bad[]={std::numeric_limits<float>::infinity()};
    try{measure(bad,1);return 2;}catch(const std::runtime_error&){}
    // Unique temp folder, only its own files are cleaned up.
    auto dir=fs::temp_directory_path()/"skell-demo-trace-test";
    if(!fs::create_directory(dir)){std::cerr<<"Test folder exists\n";return 3;}
    int result=0;
    try{
        Trace trace((dir/"events.csv").string(),(dir/"meters.csv").string(),4);
        trace.parameter(0,"amount",0.5);
        try{trace.parameter(0,"bad,id",.5);result=4;}catch(const std::invalid_argument&){}
        try{trace.parameter(0,"amount",1.5);result=5;}catch(const std::invalid_argument&){}
        trace.block(0,2,a,b,cuts);
        try{trace.block(3,2,a,b,cuts);result=6;}catch(const std::invalid_argument&){}
        try{trace.finish();result=7;}catch(const std::runtime_error&){}
        trace.parameter(2,"amount",1.0);trace.block(2,2,a,b,cuts);trace.finish();
    }catch(...){result=8;}
    fs::remove_all(dir);return result;
}
