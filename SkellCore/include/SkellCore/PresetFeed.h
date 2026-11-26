#pragma once
// Worker-thread-only, transport/storage independent preset distribution.
// Authentication MUST verify the exact manifest bytes with a pinned public key.
#include <cstdint>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace skellcore::presetfeed {
constexpr std::size_t manifestLimit=1024*1024, presetLimit=4*1024*1024,
    coverLimit=8*1024*1024, signatureLimit=4096, entryLimit=1000;
struct Entry {
    std::string id,artistId,artist,title,category,sha,coverSha;
    std::uint64_t minAbi=0,bytes=0,coverBytes=0;
};
struct Manifest { std::string product,channel; std::uint64_t generation=0; std::vector<Entry> entries; };
inline bool id(std::string_view s) {
    if(s.empty()||s.size()>80)return false;
    for(char c:s)if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'||c=='-'))return false;
    return (s.front()>='a'&&s.front()<='z')||(s.front()>='0'&&s.front()<='9');
}
inline bool hash(std::string_view s) {
    if(s.size()!=64)return false;
    for(char c:s)if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;
    return true;
}
inline bool label(std::string_view s,std::size_t limit) {
    if(s.empty()||s.size()>limit)return false;
    bool nonspace=false;
    for(unsigned char c:s){if(c<32||c==127)return false;if(c!=' ')nonspace=true;}
    return nonspace;
}
inline bool integer(std::string_view s,std::uint64_t& v,std::uint64_t limit) {
    v=0;if(s.empty()||s.size()>16||(s.size()>1&&s.front()=='0'))return false;
    for(char c:s){if(c<'0'||c>'9')return false;const auto d=static_cast<unsigned>(c-'0');if(d>limit||v>(limit-d)/10)return false;v=v*10+d;}
    return true;
}
inline std::vector<std::string> fields(std::string_view s) {
    std::vector<std::string> out;
    for(;;){const auto p=s.find('\t');out.emplace_back(s.substr(0,p));if(p==s.npos)break;s.remove_prefix(p+1);if(out.size()>10)break;}
    return out;
}
inline std::optional<Manifest> parse(std::string_view raw) {
    if(raw.empty()||raw.size()>manifestLimit||raw.back()!='\n')return {};
    const auto p=raw.find('\n');auto h=fields(raw.substr(0,p));raw.remove_prefix(p+1);
    Manifest m;
    if(h.size()!=4||h[0]!="SKELLFEED1"||!id(h[1])||(h[2]!="release"&&h[2]!="trial")||!integer(h[3],m.generation,9007199254740991ULL)||m.generation==0)return {};
    m.product=h[1];m.channel=h[2];std::set<std::string> ids;
    while(!raw.empty()){
        const auto n=raw.find('\n');if(n==raw.npos||m.entries.size()>=entryLimit)return {};
        auto f=fields(raw.substr(0,n));raw.remove_prefix(n+1);Entry e;
        if(f.size()!=10||!id(f[0])||!id(f[1])||!label(f[2],240)||!label(f[3],288)||(f[4]!="main"&&f[4]!="adlib")||!integer(f[5],e.minAbi,1000000)||e.minAbi==0||!hash(f[6])||!integer(f[7],e.bytes,presetLimit)||e.bytes==0||!integer(f[9],e.coverBytes,coverLimit))return {};
        if((f[8]=="-"&&e.coverBytes!=0)||(f[8]!="-"&&(!hash(f[8])||e.coverBytes==0))||!ids.insert(f[0]).second)return {};
        e.id=f[0];e.artistId=f[1];e.artist=f[2];e.title=f[3];e.category=f[4];e.sha=f[6];e.coverSha=f[8];m.entries.push_back(std::move(e));
    }
    return m;
}
// All callbacks are supplied by the host's worker, not the audio thread.
// Fetch accepts fixed relative paths only; adapter must enforce trusted HTTPS
// origin, no redirects, timeout and max-byte limits. A cache is per product/channel.
// Store writes immutable content objects atomically; commit publishes manifest
// and signature together atomically, with a cross-process lock and CAS against
// the old snapshot. Never overwrite a user preset or activate a rack here.
struct Snapshot {std::string manifest,signature;};
struct IO {
    std::function<std::optional<std::string>(const std::string&,std::size_t)> fetch,read;
    std::function<bool(std::string_view,std::string_view)> authenticate;
    std::function<std::string(std::string_view)> digest;
    std::function<bool(std::string_view,bool)> validate; // native package / PNG
    std::function<bool(const std::string&,std::string_view)> store;
    std::function<bool(const Snapshot&,const Snapshot&)> commit; // old,new
};
enum class Status { updated,current,offline,invalid,rollback,incompatible,storageFailed };
struct Result {Status status;std::size_t downloaded=0,reused=0;};
inline Result sync(const IO& io,const Snapshot& previous,const std::string& product,const std::string& channel,std::uint64_t abi) {
    if(!id(product)||(channel!="release"&&channel!="trial")||!io.fetch||!io.read||!io.authenticate||!io.digest||!io.validate||!io.store||!io.commit)return {Status::invalid};
    auto raw=io.fetch("manifest.tsv",manifestLimit),sig=io.fetch("manifest.sig",signatureLimit);
    if(!raw||!sig)return {Status::offline};
    if(raw->size()>manifestLimit||sig->empty()||sig->size()>signatureLimit||!io.authenticate(*raw,*sig))return {Status::invalid};
    auto m=parse(*raw);if(!m||m->product!=product||m->channel!=channel)return {Status::invalid};
    if(!previous.manifest.empty()){
        auto old=parse(previous.manifest);
        if(!old||old->product!=product||old->channel!=channel||previous.signature.empty()||previous.signature.size()>signatureLimit||!io.authenticate(previous.manifest,previous.signature))return {Status::invalid};
        if(m->generation<old->generation||(m->generation==old->generation&&*raw!=previous.manifest))return {Status::rollback};
    }
    for(const auto& e:m->entries)if(e.minAbi>abi)return {Status::incompatible};
    Result r{Status::updated};std::uint64_t budget=128ULL*1024*1024;
    auto object=[&](const std::string& sha,std::uint64_t size,bool image){
        const std::string path="objects/"+sha+(image?".png":".skellpreset");
        const auto limit=static_cast<std::size_t>(size);
        auto valid=[&](const std::optional<std::string>& b){return b&&b->size()==limit&&io.digest(*b)==sha&&io.validate(*b,image);};
        if(valid(io.read(path,limit))){++r.reused;return true;}
        if(size>budget){r.status=Status::invalid;return false;}budget-=size;
        auto data=io.fetch(path,limit);
        if(!data){r.status=Status::offline;return false;}
        if(!valid(data)){r.status=Status::invalid;return false;}
        if(!io.store(path,*data)){r.status=Status::storageFailed;return false;}
        ++r.downloaded;return true;
    };
    for(const auto& e:m->entries){if(!object(e.sha,e.bytes,false))return r;if(e.coverBytes&&!object(e.coverSha,e.coverBytes,true))return r;}
    if(previous.manifest==*raw){r.status=Status::current;return r;}
    if(!io.commit(previous,Snapshot{*raw,*sig}))r.status=Status::storageFailed;
    return r;
}
} // namespace skellcore::presetfeed
