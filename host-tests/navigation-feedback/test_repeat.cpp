#include "util/ButtonNavigator.h"
#include <cassert>
#include <cstdio>

unsigned long testNowMs = 1000;
int main() {
  MappedInputManager input;
  ButtonNavigator::setMappedInputManager(input);
  ButtonNavigator nav;
  int steps = 0, pages = 0;
  const auto poll = [&] {
    nav.onNextPress([&] { ++steps; });
    nav.onPreviousPress([&] { --steps; });
    nav.onNextContinuous([&] { ++pages; });
    nav.onPreviousContinuous([&] { --pages; });
  };
  input.frame = {0, 2, 0, 2};
  poll();
  assert(steps == 1 && pages == 0 && input.repeatFeedbacks == 0);
  testNowMs += 501;
  input.frame = {501, 0, 0, 2};
  poll();
  assert(steps == 1 && pages == 1 && input.repeatFeedbacks == 1);
  testNowMs += 100;
  poll();
  assert(pages == 1 && input.repeatFeedbacks == 1);
  testNowMs += 501;
  poll();
  assert(pages == 2 && input.repeatFeedbacks == 2);
  input.frame = {1102, 0, 2, 0};
  poll();
  assert(pages == 2 && steps == 1 && input.repeatFeedbacks == 2);
  input.frame = {0, 1, 0, 1};
  poll();
  assert(steps == 0 && pages == 2 && input.repeatFeedbacks == 2);
  testNowMs += 501;
  input.frame = {501, 0, 0, 1};
  poll();
  assert(pages == 1 && input.repeatFeedbacks == 3);
  std::puts("PASS production ButtonNavigator: one feedback per held repeat; no duplicate on press/release");
}
