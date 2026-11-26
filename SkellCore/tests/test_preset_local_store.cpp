#include <SkellCore/PresetLocalStore.h>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace skellcore::presetownership;
void require(bool b){if(!b)throw std::runtime_error("preset disk store regression");}
int main(){
    namespace fs=std::filesystem;
    const auto root=fs::temp_directory_path()/("skell-ownership-test-"+std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    struct Cleanup {fs::path p;~Cleanup(){std::error_code e;fs::remove_all(p,e);}} cleanup{root};
    DiskStore first(root),second(root);Validator check=[](auto p){return p=="original"||p=="modified";};
    Frozen f{"Free",{}, {},"original"};auto io=first.io();require(first.load()==std::optional<Bytes>{Bytes{}});
    require(edit(io,check,[&](auto& s){return savePersonal(s,"mine",f,check)&&setDefault(s,f,check);}));
    auto before=second.load();require(before.has_value());auto s=decode(*before,check);require(s.has_value()&&s->startup->payload=="original");
    auto a=*s,b=*s;f.payload="modified";require(savePersonal(a,"mine",f,check,true));b.preferences.automatic=false;
    auto aa=encode(a,check),bb=encode(b,check);require(aa.has_value()&&bb.has_value());bool x=false,y=false;
    std::thread t1([&]{x=first.compareAndSwap(*before,*aa);}),t2([&]{y=second.compareAndSwap(*before,*bb);});t1.join();t2.join();require(x!=y);
    auto latest=first.load();require(latest.has_value()&&*latest==(x?*aa:*bb));require(!second.compareAndSwap(*before,*aa));
    require(decode(*latest,check)->startup->payload=="original");
    // A crash before rename must leave the committed bytes untouched.
    {std::ofstream tmp(root/"ownership.new",std::ios::binary);tmp<<"interrupted";}
    require(second.load()==latest);require(second.compareAndSwap(*latest,*before));require(!fs::exists(root/"ownership.new"));
    {std::ofstream out(root/"ownership.bin",std::ios::binary|std::ios::trunc);out<<"corrupt";}
    require(!edit(io,check,[&](auto&){return true;})&&first.load()==std::optional<Bytes>{"corrupt"});
    {std::ofstream out(root/"ownership.bin",std::ios::binary|std::ios::trunc);}
    require(!first.load());require(!first.compareAndSwap({},*before));
    fs::remove(root/"ownership.bin");require(first.compareAndSwap({},*before));require(first.load()==before);
    std::cout<<"preset disk store: restart, concurrent CAS, interrupted write, corruption and independent default passed\n";
}
