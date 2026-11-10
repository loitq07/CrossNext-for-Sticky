#include "SideButtonNavigation.h"
#include "../../lib/GfxRenderer/RevealedInteractions.h"
#include <cassert>
#include <cstdio>

int main() {
  namespace ui = freeink::ui;
  using Button = MappedInputManager::Button;
  paintclock::RevealedInteractions<4> controls;
  auto& raw = static_cast<ui::InteractionBuffer<4>&>(controls);
  raw.addInteraction({{10, 20, 180, 50}, 1, 10, ui::InputTouch});
  raw.addInteraction({{10, 80, 180, 50}, 1, 11, ui::InputTouch, ui::StateDisabled});
  raw.addInteraction({{10, 140, 180, 50}, 1, 12, ui::InputTouch});
  MappedInputManager input;
  Activity owner;
  int x = 0, y = 0;
  const auto route = [&] {
    assert(sidebuttons::poll(controls, input, x, y));
    ui::InputSnapshot snap;
    snap.touchReleased = true;
    snap.touchX = x;
    snap.touchY = y;
    return sidebuttons::route(controls, snap, input, owner);
  };
  // A restored logical selection must still repaint when keys first reveal it.
  controls.setFocusedIndex(0);
  input.key = true;
  route();
  assert(!sidebuttons::hasActivation(x, y));
  assert(controls.hasButtonFocus() && owner.repaints == 1);
  assert(controls.focusedIndex() == 0);
  route();
  assert(controls.focusedIndex() == 2 && owner.repaints == 2);
  input.released = Button::Confirm;
  const auto selected = route();
  assert(sidebuttons::hasActivation(x, y));
  assert(selected.action == 1 && selected.value == 12);
  input.key = false;
  input.touch = true;
  const auto tapped = route();
  assert(tapped.action == 1 && tapped.value == 10 && !controls.hasButtonFocus());
  input.touch = false;
  input.key = true;
  input.released = Button::NavPrevious;
  assert(!route() && controls.hasButtonFocus());
  // Touch wins over a simultaneous key and owns its following release.
  input.touch = input.pressed = true;
  const auto simultaneous = route();
  assert(simultaneous.value == 10 && !controls.hasButtonFocus() && input.suppressed > 0);
  std::puts("PASS production side-button adapter: focus repaint, disabled skip, Confirm and touch/key handoff");
}
