// Platform socket options are isolated in this translation unit so Winsock 2
// is included before any JUCE/Windows headers in each consuming plugin build.
#if defined(_WIN32)
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
 #endif
 #include <winsock2.h>
#else
 #include <netinet/in.h>
 #include <netinet/tcp.h>
 #include <sys/socket.h>
#endif

#include <SkellCore/WorldLink.h>

namespace skellcore::world::detail {
void enableTcpNoDelay(juce::StreamingSocket& socket) noexcept {
    const auto handle = socket.getRawSocketHandle();
    if (handle < 0) return;
    const int enabled = 1;
#if defined(_WIN32)
    (void)::setsockopt(static_cast<SOCKET>(handle), IPPROTO_TCP, TCP_NODELAY,
                       reinterpret_cast<const char*>(&enabled),
                       static_cast<int>(sizeof(enabled)));
#else
    (void)::setsockopt(handle, IPPROTO_TCP, TCP_NODELAY, &enabled,
                       static_cast<socklen_t>(sizeof(enabled)));
#endif
}
} // namespace skellcore::world::detail
