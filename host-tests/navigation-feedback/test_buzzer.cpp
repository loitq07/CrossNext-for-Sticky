#include "HalBuzzer.h"
#include <cassert>
#include <cstdio>

int main() {
  for (int volume = 0; volume < 3; ++volume) {
    const int before = pwmtest::tones;
    HalBuzzer::click(volume);
    HalBuzzer::beep(2500, 30, volume);
    HalBuzzer::click(volume);
    assert(pwmtest::tones == before + 3);
    assert(pwmtest::attaches == volume * 2 + 1);
    HalBuzzer::prepareForDeepSleep();
    assert(!pwmtest::attached);
    HalBuzzer::beep(2500, 30, volume);
    HalBuzzer::click(volume);
    assert(pwmtest::tones == before + 5);
    HalBuzzer::prepareForDeepSleep();
    assert(!pwmtest::attached);
  }
  const int before = pwmtest::tones;
  BoardConfig::ACTIVE.audio.buzzer = BoardConfig::PIN_UNASSIGNED;
  HalBuzzer::click();
  HalBuzzer::beep();
  HalBuzzer::prepareForDeepSleep();
  assert(pwmtest::tones == before);
  assert(pwmtest::attaches == 6 && pwmtest::detaches == 6);
  std::puts("PASS real buzzer driver: mixed click/beep, all volumes, sleep/restart and absent hardware");
}
