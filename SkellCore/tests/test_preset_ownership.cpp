#include <SkellCore/PresetOwnership.h>
#include <iostream>
#include <stdexcept>
using namespace skellcore::presetownership;
void require(bool b){if(!b)throw std::runtime_error("preset ownership regression");}
int main(){
    const std::string a(64,'a'),b(64,'b');
    Validator check=[](std::string_view p){return p=="a"||p=="b"||p=="custom"||p=="factory";};
    std::map<std::string,Bytes> cache{{"objects/"+a+".skellpreset","a"},{"objects/"+b+".skellpreset","b"}};
    OfflineIO offline;
    offline.authenticate=[](auto raw,auto sig){return sig=="sig:"+std::string(raw);};
    offline.digest=[](auto p){return std::string(64,p.empty()?'x':p.front());};
    offline.read=[&](const auto& path,auto limit)->std::optional<Bytes>{auto i=cache.find(path);if(i==cache.end()||i->second.size()>limit)return {};return i->second;};offline.validate=check;
    auto snapshot=[&](int generation,const std::string& sha){auto m="SKELLFEED1\tskell-preset\trelease\t"+std::to_string(generation)+"\nfree\tskellyruno\tskellyruno\tFree\tmain\t2\t"+sha+"\t1\t-\t0\n";return feed::Snapshot{m,"sig:"+m};};
    auto v1=snapshot(1,a),v2=snapshot(2,b);Frozen factory{"Factory",{}, {},"factory"};std::map<std::string,Frozen> bundled{{"free",factory}};
    auto resolve=[&](const feed::Snapshot& s,unsigned abi=2){return official(offline,s,"skell-preset","release",abi,"free",bundled);};
    require(resolve({})->payload=="factory");
    require(resolve(v1)->payload=="a");
    LocalState local;auto original=*resolve(v1);auto custom=original;custom.title="My Free";custom.payload="custom";
    require(savePersonal(local,"my_free",custom,check));require(!savePersonal(local,"my_free",original,check));
    require(pin(local,"free_v1",original,check));require(!pin(local,"free_v1",original,check));require(setDefault(local,custom,check));
    auto before=encode(local,check);require(before.has_value());
    require(resolve(v2)->payload=="b");require(encode(local,check)==before);
    require(local.personal.at("my_free").payload=="custom"&&local.pinned.at("free_v1").payload=="a"&&startup(local,factory,check)->payload=="custom");
    require(savePersonal(local,"my_free",original,check,true));require(startup(local,factory,check)->payload=="custom");
    require(revision(offline,{v2,v1},"skell-preset","release",2,"free",a)->payload=="a");
    require(!revision(offline,{v2},"skell-preset","release",2,"free",a));
    cache.erase("objects/"+a+".skellpreset");require(!revision(offline,{v1},"skell-preset","release",2,"free",a));require(local.pinned.at("free_v1").payload=="a");
    cache["objects/"+b+".skellpreset"]="a";require(resolve(v2)->payload=="factory");cache["objects/"+b+".skellpreset"]="b";
    auto bad=v2;bad.signature="invalid";require(resolve(bad)->payload=="factory");require(resolve(v2,1)->payload=="factory");
    require(restoreProject("a",check)==std::optional<Bytes>{"a"});require(!restoreProject("wrong",check));
    local.preferences={false,false,false};require(!shouldCheck(local.preferences,true,false)&&shouldCheck(local.preferences,true,true)&&!shouldCheck(local.preferences,false,true));
    auto encoded=encode(local,check);auto reopened=decode(*encoded,check);require(reopened.has_value()&&encode(*reopened,check)==encoded);require(!reopened->preferences.automatic&&reopened->startup->payload=="custom");
    require(!decode(encoded->substr(0,encoded->size()-1),check));require(!decode(*encoded+"x",check));
    auto wrong=*encoded;wrong[10]=8;require(!decode(wrong,check));
    Bytes store=*encoded;bool write=true;LocalIO io{[&](){return std::optional<Bytes>{store};},[&](auto old,auto next){if(!write||old!=store)return false;store=next;return true;}};
    auto change=[&](LocalState& s){return savePersonal(s,"another",custom,check);};write=false;require(!edit(io,check,change)&&store==*encoded);write=true;require(edit(io,check,change)&&decode(store,check)->personal.count("another")==1);
    store="corrupt";require(!edit(io,check,change)&&store=="corrupt");store.clear();require(edit(io,check,change));
    require(!savePersonal(local,"../bad",custom,check));require(!pin(local,"not_official",factory,check));
    std::cout<<"preset ownership, offline fallback, persistence and project recall passed\n";
}
