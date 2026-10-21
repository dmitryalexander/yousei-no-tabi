#pragma once
#include <atomic>
#include <string_view>

namespace Ship {
// Audio keeps its alternate resources when graphics revert, including loads on
// the audio/worker threads. Set only at the normal asset-switch boundary.
inline std::atomic_bool gAlternateGraphicsEnabled{true};
inline bool AllowAlternateResource(std::string_view path) {
    if (gAlternateGraphicsEnabled.load(std::memory_order_relaxed)) return true;
    if (path.starts_with("alt/")) path.remove_prefix(4);
    return !(path.starts_with("objects/") || path.starts_with("scenes/") ||
             path.starts_with("textures/") || path.starts_with("overlays/"));
}
}
