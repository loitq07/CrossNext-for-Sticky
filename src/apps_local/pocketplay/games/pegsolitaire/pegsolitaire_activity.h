#pragma once

#include <memory>

class Activity;
class GfxRenderer;
class MappedInputManager;

namespace pocketplay {
std::unique_ptr<Activity> createPegSolitaire(GfxRenderer& renderer, MappedInputManager& mappedInput);
}
