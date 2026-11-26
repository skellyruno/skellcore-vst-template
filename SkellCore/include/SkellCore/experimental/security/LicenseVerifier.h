#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
#include <openssl/evp.h>

namespace skellcore::experimental::security {
// Offline verifier only. Invoke outside the audio thread. No private key in client.
using Id = std::array<std::uint8_t,16>;
using Binding = std::array<std::uint8_t,32>;
struct TrustKey { std::uint32_t id; std::array<std::uint8_t,32> publicKey; };
struct Policy { Id product; Binding binding; std::uint64_t now, minimumSequence, allowedFeatures; };
enum class Status { valid, malformed, unknownKey, invalidSignature, wrongProduct, wrongBinding, invalidTime, expired, rollback, unsupportedFeatures };
struct Result { Status status; std::uint64_t features=0, expires=0, sequence=0; };
inline constexpr std::size_t payloadBytes=116, tokenBytes=180;
inline std::uint64_t readBE(const std::uint8_t* p,std::size_t n) noexcept {
    std::uint64_t v=0; for(std::size_t i=0;i<n;++i)v=(v<<8)|p[i]; return v;
}
inline Result verify(const std::uint8_t* token,std::size_t size,const TrustKey* keys,std::size_t count,const Policy& policy) noexcept {
    constexpr std::array<std::uint8_t,8> magic{'S','K','L','L','I','C','0','1'};
    if(!token||size!=tokenBytes||!keys||count==0||count>16)return {Status::malformed};
    for(std::size_t i=0;i<magic.size();++i)if(token[i]!=magic[i])return {Status::malformed};
    const auto keyId=readBE(token+8,4);const TrustKey* key=nullptr;
    for(std::size_t i=0;i<count;++i)if(keys[i].id==keyId){if(key)return {Status::malformed};key=&keys[i];}
    if(!key)return {Status::unknownKey};
    EVP_PKEY* p=EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519,nullptr,key->publicKey.data(),key->publicKey.size());
    EVP_MD_CTX* c=EVP_MD_CTX_new();
    const bool authentic=p&&c&&EVP_DigestVerifyInit(c,nullptr,nullptr,nullptr,p)==1&&EVP_DigestVerify(c,token+payloadBytes,64,token,payloadBytes)==1;
    EVP_MD_CTX_free(c);EVP_PKEY_free(p);
    if(!authentic)return {Status::invalidSignature};
    for(std::size_t i=0;i<16;++i)if(token[12+i]!=policy.product[i])return {Status::wrongProduct};
    for(std::size_t i=0;i<32;++i)if(token[44+i]!=policy.binding[i])return {Status::wrongBinding};
    const auto features=readBE(token+76,8), issued=readBE(token+84,8), start=readBE(token+92,8), expiry=readBE(token+100,8), seq=readBE(token+108,8);
    if(!issued||issued>start||start>=expiry||policy.now<start)return {Status::invalidTime};
    if(policy.now>=expiry)return {Status::expired};
    if(!seq||seq<policy.minimumSequence)return {Status::rollback};
    if(features&~policy.allowedFeatures)return {Status::unsupportedFeatures};
    return {Status::valid,features,expiry,seq};
}
}
