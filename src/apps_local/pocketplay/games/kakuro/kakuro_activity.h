#pragma once

#include <memory>

class Activity;
class GfxRenderer;
class MappedInputManager;

namespace pocketplay {
std::unique_ptr<Activity> createKakuro(GfxRenderer& renderer, MappedInputManager& mappedInput);
}
