#pragma once
// Control/worker-thread only. Local state never participates in remote sync.
#include <SkellCore/PresetFeed.h>
#include <map>
#include <utility>

namespace skellcore::presetownership {
namespace feed = presetfeed;
using Bytes = std::string;
using Validator = std::function<bool(std::string_view)>;
struct Preferences { bool automatic=true, notify=true, history=true; };
inline bool shouldCheck(const Preferences& p,bool online,bool manual) { return online&&(manual||p.automatic); }
struct Frozen {
    std::string title,sourceId,sourceSha;
    Bytes payload; // Complete native package/state, not a pointer to a mutable catalog.
};
inline bool valid(const Frozen& f,const Validator& check) {
    return feed::label(f.title,288)&&((f.sourceId.empty()&&f.sourceSha.empty())||(feed::id(f.sourceId)&&feed::hash(f.sourceSha)))&&
        !f.payload.empty()&&f.payload.size()<=feed::presetLimit&&check&&check(f.payload);
}
struct LocalState {
    Preferences preferences;
    std::map<std::string,Frozen> personal,pinned;
    std::optional<Frozen> startup;
};
inline bool savePersonal(LocalState& s,const std::string& id,Frozen f,const Validator& check,bool replace=false) {
    if(!feed::id(id)||!valid(f,check)||(!replace&&s.personal.count(id))||(!s.personal.count(id)&&s.personal.size()>=feed::entryLimit))return false;
    s.personal[id]=std::move(f);return true;
}
inline bool pin(LocalState& s,const std::string& localId,Frozen f,const Validator& check) {
    if(!feed::id(localId)||f.sourceId.empty()||!valid(f,check)||s.pinned.count(localId)||s.pinned.size()>=feed::entryLimit)return false;
    s.pinned.emplace(localId,std::move(f));return true;
}
inline bool setDefault(LocalState& s,Frozen f,const Validator& check) {
    if(!valid(f,check))return false;
    s.startup=std::move(f);return true;
}
inline std::optional<Frozen> startup(const LocalState& s,const Frozen& factory,const Validator& check) {
    if(s.startup&&valid(*s.startup,check))return s.startup;
    if(valid(factory,check))return factory;
    return {};
}
// A restored project is never resolved by a preset ID or the current default.
// Invalid embedded state is an error for the host to surface, not permission to substitute.
inline std::optional<Bytes> restoreProject(std::string_view embedded,const Validator& check) {
    if(embedded.empty()||embedded.size()>feed::presetLimit||!check||!check(embedded))return {};
    return Bytes(embedded);
}
struct OfflineIO {
    std::function<bool(std::string_view,std::string_view)> authenticate;
    std::function<std::string(std::string_view)> digest;
    std::function<std::optional<Bytes>(const std::string&,std::size_t)> read;
    Validator validate;
};
inline std::optional<feed::Manifest> verified(const OfflineIO& io,const feed::Snapshot& s,std::string_view product,std::string_view channel) {
    if(!io.authenticate||s.manifest.empty()||s.manifest.size()>feed::manifestLimit||s.signature.empty()||s.signature.size()>feed::signatureLimit||!io.authenticate(s.manifest,s.signature))return {};
    auto m=feed::parse(s.manifest);
    if(!m||m->product!=product||m->channel!=channel)return {};
    return m;
}
inline std::optional<Frozen> cached(const OfflineIO& io,const feed::Entry& e,std::uint64_t abi) {
    if(!feed::id(e.id)||!feed::hash(e.sha)||e.bytes==0||e.bytes>feed::presetLimit||e.minAbi==0||e.minAbi>abi||!io.read||!io.digest||!io.validate)return {};
    auto b=io.read("objects/"+e.sha+".skellpreset",static_cast<std::size_t>(e.bytes));
    if(!b||b->size()!=e.bytes||io.digest(*b)!=e.sha)return {};
    Frozen f{e.title,e.id,e.sha,std::move(*b)};
    if(!valid(f,io.validate))return {};
    return f;
}
// No network callback exists here. Only the current authenticated snapshot is
// eligible automatically; older revisions require explicit selection/pinning.
inline std::optional<Frozen> official(const OfflineIO& io,const feed::Snapshot& s,std::string_view product,std::string_view channel,std::uint64_t abi,const std::string& id,const std::map<std::string,Frozen>& bundled) {
    if(auto m=verified(io,s,product,channel))for(const auto& e:m->entries)if(e.id==id){if(auto f=cached(io,e,abi))return f;break;}
    auto i=bundled.find(id);if(i!=bundled.end()&&valid(i->second,io.validate))return i->second;
    return {};
}
inline std::optional<Frozen> revision(const OfflineIO& io,const std::vector<feed::Snapshot>& history,std::string_view product,std::string_view channel,std::uint64_t abi,const std::string& id,const std::string& sha) {
    if(!feed::id(id)||!feed::hash(sha))return {};
    for(const auto& s:history)if(auto m=verified(io,s,product,channel))for(const auto& e:m->entries)if(e.id==id&&e.sha==sha)if(auto f=cached(io,e,abi))return f;
    return {};
}
// Persistence codec: bounded binary envelope. Adapter stores in a per-product
// local-only location with atomic CAS; corrupt data must not be reset silently.
constexpr std::size_t stateLimit=64*1024*1024;
inline void u32(Bytes& b,std::uint32_t n) { for(unsigned i=0;i<4;++i)b.push_back(static_cast<char>((n>>(8*i))&255)); }
inline bool field(Bytes& b,std::string_view s) {
    if(s.size()>stateLimit||b.size()>stateLimit-4||s.size()>stateLimit-b.size()-4)return false;
    u32(b,static_cast<std::uint32_t>(s.size()));b.append(s);return true;
}
inline bool get32(std::string_view& r,std::uint32_t& n) {
    if(r.size()<4)return false;
    n=0;for(unsigned i=0;i<4;++i)n|=static_cast<std::uint32_t>(static_cast<unsigned char>(r[i]))<<(8*i);
    r.remove_prefix(4);return true;
}
inline bool get(std::string_view& r,std::string& v,std::size_t limit) {
    std::uint32_t n;if(!get32(r,n)||n>limit||n>r.size())return false;
    v.assign(r.substr(0,n));r.remove_prefix(n);return true;
}
inline std::optional<Bytes> encode(const LocalState& s,const Validator& check) {
    Bytes b="SKELLOCAL1";
    b.push_back(static_cast<char>((s.preferences.automatic?1:0)|(s.preferences.notify?2:0)|(s.preferences.history?4:0)));
    auto record=[&](const Frozen& f){return valid(f,check)&&field(b,f.title)&&field(b,f.sourceId)&&field(b,f.sourceSha)&&field(b,f.payload);};
    auto group=[&](const auto& g){if(g.size()>feed::entryLimit||b.size()>stateLimit-4)return false;u32(b,static_cast<std::uint32_t>(g.size()));for(const auto& kv:g)if(!feed::id(kv.first)||!field(b,kv.first)||!record(kv.second))return false;return true;};
    if(!group(s.personal)||!group(s.pinned)||b.size()>=stateLimit)return {};
    for(const auto& kv:s.pinned)if(kv.second.sourceId.empty())return {};
    b.push_back(s.startup?1:0);if(s.startup&&!record(*s.startup))return {};
    return b;
}
inline std::optional<LocalState> decode(std::string_view r,const Validator& check) {
    if(r.size()<11||r.size()>stateLimit||r.substr(0,10)!="SKELLOCAL1")return {};
    r.remove_prefix(10);unsigned flags=static_cast<unsigned char>(r.front());r.remove_prefix(1);if(flags>7)return {};
    LocalState s;s.preferences={bool(flags&1),bool(flags&2),bool(flags&4)};
    auto record=[&](Frozen& f){return get(r,f.title,288)&&get(r,f.sourceId,80)&&get(r,f.sourceSha,64)&&get(r,f.payload,feed::presetLimit)&&valid(f,check);};
    auto group=[&](auto& g){std::uint32_t n;if(!get32(r,n)||n>feed::entryLimit)return false;for(std::uint32_t i=0;i<n;++i){std::string k;Frozen f;if(!get(r,k,80)||!feed::id(k)||!record(f)||!g.emplace(k,std::move(f)).second)return false;}return true;};
    if(!group(s.personal)||!group(s.pinned)||r.empty()||static_cast<unsigned char>(r.front())>1)return {};
    for(const auto& kv:s.pinned)if(kv.second.sourceId.empty())return {};
    bool has=r.front()!=0;r.remove_prefix(1);if(has){Frozen f;if(!record(f))return {};s.startup=std::move(f);}
    if(!r.empty())return {};
    return s;
}
struct LocalIO {
    // nullopt means I/O error; empty bytes means no local store yet.
    std::function<std::optional<Bytes>()> load;
    std::function<bool(std::string_view,std::string_view)> compareAndSwap;
};
inline bool edit(const LocalIO& io,const Validator& check,const std::function<bool(LocalState&)>& change) {
    if(!io.load||!io.compareAndSwap||!change)return false;
    auto raw=io.load();if(!raw)return false;
    auto s=raw->empty()?std::optional<LocalState>{LocalState{}}:decode(*raw,check);
    if(!s||!change(*s))return false;
    auto next=encode(*s,check);return next&&io.compareAndSwap(*raw,*next);
}
} // namespace skellcore::presetownership
