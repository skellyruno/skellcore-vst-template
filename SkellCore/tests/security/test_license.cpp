#include "LicenseVerifier.h"
#include <cstdio>
#include <cstdlib>
using namespace skellcore::experimental::security;
static void check(bool ok,const char* msg){if(!ok){std::fprintf(stderr,"FAIL %s\n",msg);std::exit(1);}}
static void put(std::uint8_t* p,std::uint64_t v,int n){while(n){p[--n]=std::uint8_t(v);v>>=8;}}
int main(){
    EVP_PKEY_CTX* g=EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519,nullptr);EVP_PKEY* k=nullptr;
    check(g&&EVP_PKEY_keygen_init(g)==1&&EVP_PKEY_keygen(g,&k)==1,"ephemeral test key");EVP_PKEY_CTX_free(g);
    TrustKey key{7,{}};std::size_t n=key.publicKey.size();check(EVP_PKEY_get_raw_public_key(k,key.publicKey.data(),&n)==1&&n==32,"public key");
    std::array<std::uint8_t,tokenBytes> t{};const char magic[]="SKLLIC01";for(int i=0;i<8;++i)t[i]=magic[i];put(t.data()+8,7,4);
    Policy p{};p.product[0]=42;p.binding[0]=17;p.now=120;p.minimumSequence=3;p.allowedFeatures=3;t[12]=42;t[28]=99;t[44]=17;
    put(t.data()+76,1,8);put(t.data()+84,100,8);put(t.data()+92,110,8);put(t.data()+100,150,8);put(t.data()+108,3,8);
    auto sign=[&](){auto* c=EVP_MD_CTX_new();std::size_t len=64;check(c&&EVP_DigestSignInit(c,nullptr,nullptr,nullptr,k)==1&&EVP_DigestSign(c,t.data()+payloadBytes,&len,t.data(),payloadBytes)==1&&len==64,"test sign");EVP_MD_CTX_free(c);};sign();
    auto run=[&](Policy v){return verify(t.data(),t.size(),&key,1,v);};
    auto r=run(p);check(r.status==Status::valid&&r.features==1&&r.expires==150&&r.sequence==3,"valid");
    for(std::size_t i=12;i<t.size();++i){t[i]^=1;check(run(p).status==Status::invalidSignature,"tamper every signed field and signature byte");t[i]^=1;}
    for(std::size_t i=0;i<t.size();++i)check(verify(t.data(),i,&key,1,p).status==Status::malformed,"truncation");
    check(verify(nullptr,tokenBytes,&key,1,p).status==Status::malformed,"null");
    auto q=p;q.product[0]^=1;check(run(q).status==Status::wrongProduct,"product");q=p;q.binding[0]^=1;check(run(q).status==Status::wrongBinding,"binding");
    q=p;q.now=109;check(run(q).status==Status::invalidTime,"not before");q.now=110;check(run(q).status==Status::valid,"start boundary");q.now=150;check(run(q).status==Status::expired,"expiry boundary");
    q=p;q.minimumSequence=4;check(run(q).status==Status::rollback,"sequence");q=p;q.allowedFeatures=0;check(run(q).status==Status::unsupportedFeatures,"feature mask");
    TrustKey duplicate[]{key,key};check(verify(t.data(),t.size(),duplicate,2,p).status==Status::malformed,"duplicate key ID");
    key.id=8;check(run(p).status==Status::unknownKey,"unknown key");key.id=7;key.publicKey[0]^=1;check(run(p).status==Status::invalidSignature,"wrong key");key.publicKey[0]^=1;
    put(t.data()+100,110,8);sign();check(run(p).status==Status::invalidTime,"signed malformed interval");
    EVP_PKEY_free(k);std::puts("Ed25519 entitlement verification: 168 tampered bytes, 180 truncations, policy boundaries PASS");
}
