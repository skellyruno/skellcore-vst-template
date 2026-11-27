# Opt-in exact native Skellay reuse. No copied engine or private state.
include(FetchContent)
set(SKELLCORE_MATCH_DELAY2_REVISION "8a0904b99dba671195f139907e09ac86a09ad5f4")
set(SKELLCORE_MATCH_DELAY2_SOURCE "" CACHE PATH "Verified local Skellay source root")
if(NOT TARGET skellcore_match_delay2)
    if(SKELLCORE_MATCH_DELAY2_SOURCE)
        set(_match_delay2_root "${SKELLCORE_MATCH_DELAY2_SOURCE}")
    else()
        FetchContent_Declare(SkellCoreMatchDelay2Source
            GIT_REPOSITORY https://github.com/skellyruno/skellay.git
            GIT_TAG "${SKELLCORE_MATCH_DELAY2_REVISION}"
            GIT_SHALLOW FALSE SOURCE_SUBDIR _native_sources_only)
        FetchContent_MakeAvailable(SkellCoreMatchDelay2Source)
        set(_match_delay2_root "${skellcorematchdelay2source_SOURCE_DIR}")
    endif()
    # Source integrity even with a local cache; never silently pick a changed engine.
    file(SHA256 "${_match_delay2_root}/native/Delay.cpp" _delay_cpp_hash)
    file(SHA256 "${_match_delay2_root}/native/Delay.h" _delay_h_hash)
    file(SHA256 "${_match_delay2_root}/native/codec.h" _codec_hash)
    if(NOT _delay_cpp_hash STREQUAL "fd4bb4f3a376fb9d93c732af5ec3d1110449c69520bf0f6b5f598874d6874b80" OR
       NOT _delay_h_hash STREQUAL "c9684a7f6022eaffc6268ee612ede736df5d6c577c12d939c144177166afedb6" OR
       NOT _codec_hash STREQUAL "90f9dc5057c056fe9c028616a94b4e5a019bea1b3eac6d150108e172070c8e94")
        message(FATAL_ERROR "Unverified Skellay source; restore pinned revision")
    endif()
    add_library(skellcore_match_delay2 STATIC "${_match_delay2_root}/native/Delay.cpp")
    target_include_directories(skellcore_match_delay2 PUBLIC "${_match_delay2_root}/native")
    target_compile_features(skellcore_match_delay2 PUBLIC cxx_std_17)
    set_target_properties(skellcore_match_delay2 PROPERTIES POSITION_INDEPENDENT_CODE ON)
    if(MSVC)
        target_compile_options(skellcore_match_delay2 PRIVATE /W4 /fp:strict /utf-8)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(skellcore_match_delay2 PRIVATE -Wall -Wextra -Werror -ffp-contract=off)
    endif()
endif()
if(NOT TARGET SkellCore::MatchDelay2)
    add_library(SkellCore::MatchDelay2 ALIAS skellcore_match_delay2)
endif()
