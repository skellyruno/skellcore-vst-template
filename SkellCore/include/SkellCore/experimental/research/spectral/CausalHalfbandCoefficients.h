#pragma once
#include <array>
#include <cstddef>
namespace skellcore::experimental::spectral {
// Original SciPy 1.17.0 remez halfband design, projected to exact symmetry and center .5.
template<std::size_t N> constexpr std::array<double,2*N> mirrorHalfband(const std::array<double,N>& h) noexcept {
    std::array<double,2*N> out{};for(std::size_t i=0;i<N;++i)out[i]=out[2*N-1-i]=h[i];return out;
}
inline constexpr auto halfband641=mirrorHalfband(std::array<double,160>{
    -0x1.b6a839b53b020p-19, 0x1.50b195e78331fp-20, -0x1.90d82d37e1d8ep-20, 0x1.d8cfc07aa623fp-20,
    -0x1.149b2fcc9643ep-19, 0x1.4163499077e9ap-19, -0x1.7317e354bf66ap-19, 0x1.aa1fd9e62c3b2p-19,
    -0x1.e6ddea28ed877p-19, 0x1.14e12819f5b75p-18, -0x1.399cf9c203d81p-18, 0x1.61dfe08813452p-18,
    -0x1.8de641f81fb35p-18, 0x1.bdf3855dd99c3p-18, -0x1.f24a7d83bef3ap-18, 0x1.159ab4108ef3ap-17,
    -0x1.347e8861f54abp-17, 0x1.55f93bba6b9b6p-17, -0x1.7a326437ee671p-17, 0x1.a1551303efa03p-17,
    -0x1.cb8c7ac3123f2p-17, 0x1.f90763f29dedap-17, -0x1.14fa429607206p-16, 0x1.2f4379c363120p-16,
    -0x1.4b78a42d6c3e5p-16, 0x1.69b543c6e4e7ap-16, -0x1.8a14a7521fe34p-16, 0x1.acb3e8026341fp-16,
    -0x1.d1b029d6f60dap-16, 0x1.f9287b63dce14p-16, -0x1.119e32e468ae2p-15, 0x1.2806b443f0d15p-15,
    -0x1.3fdea78cc6724p-15, 0x1.5937e6c306dd4p-15, -0x1.7424323bc7ae9p-15, 0x1.90b66ef28b5d1p-15,
    -0x1.af0179a0c4a96p-15, 0x1.cf197e5e92e87p-15, -0x1.f112de59a9246p-15, 0x1.0a8188c2e7929p-14,
    -0x1.1d7fbc56ad5b7p-14, 0x1.318f361ea91dfp-14, -0x1.46bb292ed6199p-14, 0x1.5d0f89693aa56p-14,
    -0x1.74986e2a79505p-14, 0x1.8d62829229c9bp-14, -0x1.a77a41652adaep-14, 0x1.c2ecbbdb6149ep-14,
    -0x1.dfc7257fd02bfp-14, 0x1.fe17bcf20f1f9p-14, -0x1.0ef652414d00dp-13, 0x1.1faa2768575c4p-13,
    -0x1.312e788ae0ae9p-13, 0x1.438b322b0fa3cp-13, -0x1.56c83dad7143fp-13, 0x1.6aed7dbf62e4ep-13,
    -0x1.80028ebc6af67p-13, 0x1.9611021572b6fp-13, -0x1.ad20577f30890p-13, 0x1.c539e6cc2e1acp-13,
    -0x1.de66b4e266e74p-13, 0x1.f8afbfecf7a2fp-13, -0x1.0a0f675810023p-12, 0x1.185eaa5f1baf5p-12,
    -0x1.274abb1c99eddp-12, 0x1.36d8a0bf937c0p-12, -0x1.470da617f6209p-12, 0x1.57ef1a587031cp-12,
    -0x1.69828afc85528p-12, 0x1.7bcd93c3cd946p-12, -0x1.8ed60a4665c58p-12, 0x1.a2a1da69cca5fp-12,
    -0x1.b7372d2fa5eb7p-12, 0x1.cc9c4422c3239p-12, -0x1.e2d7a25219f6ap-12, 0x1.f9efed2cd2caep-12,
    -0x1.08f603dda00ebp-11, 0x1.1569817721231p-11, -0x1.2256190442af4p-11, 0x1.2fbf8c2ffefe2p-11,
    -0x1.3da9c4bf08623p-11, 0x1.4c18c7313e392p-11, -0x1.5b10c288e4adap-11, 0x1.6a96086064e97p-11,
    -0x1.7aad15a3e0994p-11, 0x1.8b5a91cba4892p-11, -0x1.9ca35550f0facp-11, 0x1.ae8c698ab3aefp-11,
    -0x1.c11b11cb8d2e7p-11, 0x1.d454c5385b6d7p-11, -0x1.e83f43546f928p-11, 0x1.fce081d939020p-11,
    -0x1.091f67449a3abp-10, 0x1.14305cea01a4fp-10, -0x1.1fa697d93ad72p-10, 0x1.2b85bb623460bp-10,
    -0x1.37d19bcbc9c13p-10, 0x1.448e440d2a050p-10, -0x1.51bffba03355fp-10, 0x1.5f6b460068dd1p-10,
    -0x1.6d94ef005baaep-10, 0x1.7c420dcc47c60p-10, -0x1.8b780ca69641ep-10, 0x1.9b3caf0bd1d8bp-10,
    -0x1.ab961d079fe6ap-10, 0x1.bc8ae9e112c85p-10, -0x1.ce2220911076dp-10, 0x1.e0634ebcf6920p-10,
    -0x1.f356926bc5d91p-10, 0x1.03825414982a6p-9, -0x1.0dbb7e7a0f385p-9, 0x1.185bdf3c4e2f5p-9,
    -0x1.2368f8ffb4838p-9, 0x1.2ee8c689790e3p-9, -0x1.3ae1c5e32a4b3p-9, 0x1.475b0b0186f88p-9,
    -0x1.545c4f26d7687p-9, 0x1.61ee03aac2ae7p-9, -0x1.70196b6405e57p-9, 0x1.7ee8b21b0621bp-9,
    -0x1.8e670d31bf681p-9, 0x1.9ea0dbb7e056cp-9, -0x1.afa3d0ecba916p-9, 0x1.c17f2188f1359p-9,
    -0x1.d443bbb1b250ep-9, 0x1.e804867d351c3p-9, -0x1.fcd6ae8353843p-9, 0x1.0968ffafa0189p-8,
    -0x1.1508a7a33914dp-8, 0x1.21597f499e302p-8, -0x1.2e6cc8ec027cfp-8, 0x1.3c56365e48109p-8,
    -0x1.4b2c5abff6846p-8, 0x1.5b0935bf28915p-8, -0x1.6c0adfc636d12p-8, 0x1.7e546037d9c0bp-8,
    -0x1.920eba53e3b06p-8, 0x1.a76a42aad2a9ap-8, -0x1.bea052559ffe0p-8, 0x1.d7f57ab76cb72p-8,
    -0x1.f3bc65e88390bp-8, 0x1.092cd35c0e889p-7, -0x1.1a246b73bd50cp-7, 0x1.2d11c92741191p-7,
    -0x1.425592079a21bp-7, 0x1.5a6b1be7aff4ap-7, -0x1.75f24e4180ec5p-7, 0x1.95be4520f7065p-7,
    -0x1.baeb9227edc23p-7, 0x1.e7033a4cd8c02p-7, -0x1.0e19e427d2202p-6, 0x1.2ed955c8f1095p-6,
    -0x1.5835be1e709ddp-6, 0x1.8e28829cb52dfp-6, -0x1.d78effcf766c7p-6, 0x1.20b05c4a02482p-5,
    -0x1.73b37d08cd09cp-5, 0x1.04781c3acef09p-4, -0x1.b26cc49c941c9p-4, 0x1.45ef4f383d19fp-2
});
inline constexpr auto halfband65=mirrorHalfband(std::array<double,16>{
    -0x1.7c2bf67d0bd0cp-25, 0x1.1d8c60850bd0cp-21, -0x1.d0121496de85fp-19, 0x1.0ca0b16707628p-16,
    -0x1.ed59d7d00d2b6p-15, 0x1.7c701051c66a6p-13, -0x1.fe728db2c9a57p-12, 0x1.3143a350b66bdp-10,
    -0x1.4b819f567172cp-9, 0x1.4be3c72f662efp-8, -0x1.36bfc8acaa0c7p-7, 0x1.14772e6ff8255p-6,
    -0x1.ddac96396303bp-6, 0x1.a031538114eb8p-5, -0x1.90cf6e0f94454p-4, 0x1.4309449284695p-2
});
}
