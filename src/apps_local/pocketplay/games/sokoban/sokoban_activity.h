#pragma once

#include <memory>

class Activity;
class GfxRenderer;
class MappedInputManager;

namespace pocketplay {
std::unique_ptr<Activity> createSokoban(GfxRenderer& renderer, MappedInputManager& mappedInput);
}
