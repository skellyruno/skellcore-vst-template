#include <SkellCore/PresetFeed.h>
#include <map>
#include <iostream>
#include <stdexcept>
using namespace skellcore::presetfeed;
void check(bool ok){if(!ok)throw std::runtime_error("preset feed regression");}
int main(){
    const std::string a(64,'a'),b(64,'b'),c(64,'c');
    const auto row=[&](const std::string& sha){return "free_main\tskellyruno\tskellyruno\tFree Main\tmain\t2\t"+sha+"\t1\t"+c+"\t1\n";};
    const auto manifest=[&](int n,const std::string& sha){return "SKELLFEED1\tskell-preset\trelease\t"+std::to_string(n)+"\n"+row(sha);};
    std::map<std::string,std::string> remote,cache;Snapshot saved;int commits=0;bool storage=true,auth=true,semantic=true;
    auto read=[&](const auto& map,const std::string& path,std::size_t lim)->std::optional<std::string>{auto i=map.find(path);if(i==map.end()||i->second.size()>lim)return {};return i->second;};
    IO io;
    io.fetch=[&](const auto& p,auto lim){return read(remote,p,lim);};io.read=[&](const auto& p,auto lim){return read(cache,p,lim);};
    io.authenticate=[&](auto raw,auto sig){return auth&&sig==std::string("sig:")+std::string(raw);};
    io.digest=[](auto v){return std::string(64,v.empty()?'x':v.front());};
    io.validate=[&](auto v,bool){return semantic&&v.size()==1;};
    io.store=[&](const auto& p,auto v){if(!storage)return false;cache[p]=v;return true;};
    io.commit=[&](const auto& old,const auto& next){if(!storage||old.manifest!=saved.manifest)return false;saved=next;++commits;return true;};
    auto publish=[&](auto raw){remote["manifest.tsv"]=raw;remote["manifest.sig"]="sig:"+raw;};
    auto run=[&](unsigned abi=2){const auto old=saved;return sync(io,old,"skell-preset","release",abi);};
    remote["objects/"+a+".skellpreset"]="a";remote["objects/"+b+".skellpreset"]="b";remote["objects/"+c+".png"]="c";
    publish(manifest(1,a));auto r=run();check(r.status==Status::updated&&r.downloaded==2&&commits==1);
    r=run();check(r.status==Status::current&&r.downloaded==0&&r.reused==2&&commits==1);
    publish(manifest(2,b));r=run();check(r.status==Status::updated&&r.downloaded==1&&r.reused==1&&commits==2&&cache.size()==3);
    publish(manifest(1,a));check(run().status==Status::rollback&&commits==2);
    publish(manifest(2,a));check(run().status==Status::rollback);
    publish(manifest(3,a));auth=false;check(run().status==Status::invalid);auth=true;
    check(run(1).status==Status::incompatible);
    publish(manifest(3,a)+row(a));check(run().status==Status::invalid);
    publish(manifest(3,a));cache.erase("objects/"+a+".skellpreset");remote["objects/"+a+".skellpreset"]="z";check(run().status==Status::invalid&&commits==2);
    remote.erase("objects/"+a+".skellpreset");check(run().status==Status::offline&&commits==2);
    remote["objects/"+a+".skellpreset"]="a";storage=false;check(run().status==Status::storageFailed);storage=true;
    semantic=false;check(run().status==Status::invalid);semantic=true;
    check(run().status==Status::updated&&commits==3);
    cache.erase("objects/"+c+".png");r=run();check(r.status==Status::current&&r.downloaded==1&&commits==3);
    publish("SKELLFEED1\tskell-preset\ttrial\t4\n"+row(a));check(run().status==Status::invalid);
    check(!parse("SKELLFEED1\tskell-preset\trelease\t4\n../x\ta\ta\ta\tmain\t2\t"+a+"\t1\t-\t0\n"));
    check(!parse("SKELLFEED1\tskell-preset\trelease\t04\n"));
    std::cout<<"preset feed: 16 regression scenarios passed\n";
}
