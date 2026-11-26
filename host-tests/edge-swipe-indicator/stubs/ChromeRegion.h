#pragma once
namespace chrome {
inline int headerHeight = 0;
inline bool lightHeader = false;
inline bool framebufferLightHeader = false;
inline bool invertHeaderForOutput(bool invertedDisplay) { return invertedDisplay != framebufferLightHeader; }
}  // namespace chrome
