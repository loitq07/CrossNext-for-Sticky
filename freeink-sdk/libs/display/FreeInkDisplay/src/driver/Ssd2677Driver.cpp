#include "Ssd2677Driver.h"

#include <Arduino.h>
#include <string.h>

#include <BoardConfig.h>
#if defined(BOARD_HAS_PSRAM)
#include <esp_heap_caps.h>
#endif

namespace freeink {

namespace {

inline void pack_mono_byte_to_2bpp(uint8_t mono, uint8_t* out) {
  out[0] = 0;
  out[1] = 0;
  for (uint8_t i = 0; i < 8U; i++) {
    const uint8_t bit = static_cast<uint8_t>((mono >> (7U - i)) & 0x01U);
    const uint8_t pair = bit != 0U ? 0x03U : 0x00U;
    if (i < 4U) {
      out[0] |= static_cast<uint8_t>(pair << (6U - i * 2U));
    } else {
      out[1] |= static_cast<uint8_t>(pair << (14U - i * 2U));
    }
  }
}

inline void pack_interleave(uint8_t high_plane, uint8_t low_plane, uint8_t* out) {
  out[0] = 0;
  out[1] = 0;
  for (uint8_t i = 0; i < 8U; i++) {
    const uint8_t pair = static_cast<uint8_t>((((high_plane >> (7U - i)) & 0x01U) << 1U) |
                                              ((low_plane >> (7U - i)) & 0x01U));
    if (i < 4U) {
      out[0] |= static_cast<uint8_t>(pair << (6U - i * 2U));
    } else {
      out[1] |= static_cast<uint8_t>(pair << (14U - i * 2U));
    }
  }
}

uint8_t ssd2677_mono_full_waveform_for_temperature(uint8_t temp) {
  if (temp <= 10U) return 0xE8U;
  if (temp <= 20U) return 0xEBU;
  if (temp <= 30U) return 0xEEU;
  if (temp <= 127U) return 0xF1U;
  return 0xE8U;
}

uint8_t ssd2677_partial_waveform_for_temperature(uint8_t temp) {
  if (temp <= 10U) return 0x08U;
  if (temp <= 20U) return 0x0FU;
  if (temp <= 30U) return 0x19U;
  if (temp <= 127U) return 0x23U;
  return 0x08U;
}

uint8_t ssd2677_gray4_waveform_for_temperature(uint8_t temp) {
  if (temp <= 15U) return 0xF1U;
  if (temp <= 127U) return 0xFAU;
  return 0xF1U;
}

}  // namespace

const Ssd2677Config& ssd2677DefaultConfig() {
  static const Ssd2677Config cfg{};
  return cfg;
}

Ssd2677Driver::Ssd2677Driver(const Ssd2677Config& cfg)
    : _cfg(cfg),
      _w(800),
      _h(480),
      _wb(_w / 8),
      _bufferSize(static_cast<uint32_t>(_wb) * _h),
      _gray4BufferSize(static_cast<uint32_t>(_w / 4) * _h) {
  if (BoardConfig::ACTIVE.displayWidth && BoardConfig::ACTIVE.displayHeight) {
    _w = BoardConfig::ACTIVE.displayWidth;
    _h = BoardConfig::ACTIVE.displayHeight;
    _wb = _w / 8;
    _bufferSize = static_cast<uint32_t>(_wb) * _h;
    _gray4BufferSize = static_cast<uint32_t>(_w / 4) * _h;
  }
#if defined(BOARD_HAS_PSRAM)
  _prevFb = static_cast<uint8_t*>(heap_caps_malloc(_bufferSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
#else
  _prevFb = static_cast<uint8_t*>(malloc(_bufferSize));
#endif
  if (_prevFb) {
    memset(_prevFb, 0xFF, _bufferSize);
  }
}

Ssd2677Driver::~Ssd2677Driver() {
  if (_prevFb) {
    free(_prevFb);
    _prevFb = nullptr;
  }
  if (_gray4Fb) {
    free(_gray4Fb);
    _gray4Fb = nullptr;
  }
}

uint32_t Ssd2677Driver::spiHz() const {
  return BoardConfig::ACTIVE.displaySpiHz ? BoardConfig::ACTIVE.displaySpiHz : _cfg.spiHz;
}

PanelGeometry Ssd2677Driver::geometry() const {
  return {_w, _h, _wb, _bufferSize};
}

void Ssd2677Driver::initController(EpdBus& bus) {
  static const uint8_t psr[] = {0x2F, 0x0E};
  static const uint8_t booster[] = {0x0F, 0x8B, 0x93, 0xC1};
  static const uint8_t timing[] = {0x76, 0x76, 0x76, 0x5A, 0x9D, 0x8A, 0x76, 0x62};
  static const uint8_t resolution[] = {0x03, 0x20, 0x02, 0xA8};
  static const uint8_t gsst[] = {0x00, 0x00, 0x00, 0x00};

  bus.reset(10);
  bus.waitBusy("ssd2677_rst");

  bus.cmd(0x00);
  bus.data(psr, sizeof(psr));
  bus.waitBusy("ssd2677_psr");

  bus.cmd(0x06);
  bus.data(booster, sizeof(booster));

  bus.cmd(0xE7);
  bus.data(0xC1);

  bus.cmd(0x30);
  bus.data(0x08);

  bus.cmd(0x50);
  bus.data(0x77);

  bus.cmd(0x62);
  bus.data(timing, sizeof(timing));

  bus.cmd(0x61);
  bus.data(resolution, sizeof(resolution));

  bus.cmd(0xE0);
  bus.data(0x10);

  bus.cmd(0x65);
  bus.data(gsst, sizeof(gsst));

  bus.cmd(0xE9);
  bus.data(0x01);

  bus.waitBusy("ssd2677_init_ready");
}

uint8_t Ssd2677Driver::readTemperature(EpdBus& bus) {
  (void)bus;
  return _cfg.fallbackTemperature;
}

void Ssd2677Driver::latchWaveform(EpdBus& bus, RefreshMode mode, bool gray4) {
  const uint8_t temp = readTemperature(bus);
  uint8_t waveform = 0;
  if (gray4) {
    waveform = ssd2677_gray4_waveform_for_temperature(temp);
  } else if (mode == RefreshMode::Fast) {
    waveform = ssd2677_partial_waveform_for_temperature(temp);
  } else {
    waveform = ssd2677_mono_full_waveform_for_temperature(temp);
  }

  bus.cmd(0xE0);
  bus.data(0x12);
  bus.cmd(0xE6);
  bus.data(waveform);
  bus.cmd(0xA5);
  bus.waitBusy("ssd2677_latch");
  delay(10);
}

void Ssd2677Driver::streamFullMono(EpdBus& bus, const uint8_t* fb) {
  constexpr size_t IN_CHUNK = 512;
  uint8_t out[IN_CHUNK * 2];
  size_t offset = 0;

  bus.cmd(0x10);
  while (offset < _bufferSize) {
    const size_t chunk = (_bufferSize - offset) > IN_CHUNK ? IN_CHUNK : (_bufferSize - offset);
    for (size_t i = 0; i < chunk; i++) {
      pack_mono_byte_to_2bpp(fb[offset + i], &out[i * 2]);
    }
    bus.data(out, static_cast<uint16_t>(chunk * 2));
    offset += chunk;
  }
}

void Ssd2677Driver::streamPartialDiff(EpdBus& bus, const uint8_t* prev, const uint8_t* curr) {
  constexpr size_t IN_CHUNK = 512;
  uint8_t out[IN_CHUNK * 2];
  size_t offset = 0;

  bus.cmd(0x10);
  while (offset < _bufferSize) {
    const size_t chunk = (_bufferSize - offset) > IN_CHUNK ? IN_CHUNK : (_bufferSize - offset);
    for (size_t i = 0; i < chunk; i++) {
      pack_interleave(prev[offset + i], curr[offset + i], &out[i * 2]);
    }
    bus.data(out, static_cast<uint16_t>(chunk * 2));
    offset += chunk;
  }
}

void Ssd2677Driver::streamGray4(EpdBus& bus) {
  if (!_gray4Fb || !_gray4Valid) return;
  constexpr size_t CHUNK = 1024;
  size_t offset = 0;

  bus.cmd(0x10);
  while (offset < _gray4BufferSize) {
    const size_t chunk = (_gray4BufferSize - offset) > CHUNK ? CHUNK : (_gray4BufferSize - offset);
    bus.data(_gray4Fb + offset, static_cast<uint16_t>(chunk));
    offset += chunk;
  }
}

void Ssd2677Driver::begin(EpdBus& bus) {
  _refreshPowered = false;
  _gray4Valid = false;
  if (_prevFb) {
    memset(_prevFb, 0xFF, _bufferSize);
  }
  initController(bus);
}

void Ssd2677Driver::deepSleep(EpdBus& bus) {
  if (_refreshPowered) {
    const uint8_t zero = 0x00;
    bus.cmd(0x02);
    bus.data(zero);
    bus.waitBusy("ssd2677_sleep_pof");
    _refreshPowered = false;
  }
  const uint8_t code = 0xA5;
  bus.cmd(0x07);
  bus.data(code);
}

void Ssd2677Driver::display(EpdBus& bus, const uint8_t* fb, const uint8_t* prev, RefreshMode mode, bool turnOff) {
  displayStart(bus, fb, prev, mode, turnOff);
  displayFinish(bus, fb);
}

bool Ssd2677Driver::displayStart(EpdBus& bus, const uint8_t* fb, const uint8_t* prev, RefreshMode mode, bool turnOff) {
  (void)turnOff;
  _lastRefreshMode = mode;

  latchWaveform(bus, mode, false);

  const uint8_t* effectivePrev = prev ? prev : _prevFb;
  if (mode == RefreshMode::Fast && effectivePrev) {
    streamPartialDiff(bus, effectivePrev, fb);
  } else {
    streamFullMono(bus, fb);
  }

  if (!_refreshPowered) {
    bus.cmd(0x04);
    bus.waitBusy("ssd2677_pon");
    _refreshPowered = true;
  }

  const uint8_t zero = 0x00;
  bus.cmd(0x12);
  bus.data(zero);

  return true;
}

void Ssd2677Driver::displayFinish(EpdBus& bus, const uint8_t* fb) {
  bus.waitBusy("ssd2677_drf_done");

  if (_lastRefreshMode != RefreshMode::Fast) {
    const uint8_t zero = 0x00;
    bus.cmd(0x02);
    bus.data(zero);
    bus.waitBusy("ssd2677_pof");
    _refreshPowered = false;
  }

  if (_prevFb && fb) {
    memcpy(_prevFb, fb, _bufferSize);
  }
}

void Ssd2677Driver::displayWindow(EpdBus& bus, const uint8_t* fb, const uint8_t* prev, uint16_t x, uint16_t y,
                                  uint16_t w, uint16_t h, bool turnOff) {
  (void)x; (void)y; (void)w; (void)h;
  display(bus, fb, prev, RefreshMode::Fast, turnOff);
}

void Ssd2677Driver::seedPreviousFrame(EpdBus& bus, const uint8_t* buf) {
  (void)bus;
  if (_prevFb && buf) {
    memcpy(_prevFb, buf, _bufferSize);
  }
}

GrayscaleCapabilities Ssd2677Driver::grayscaleCapabilities(GrayscaleMode mode) const {
  (void)mode;
  GrayscaleCapabilities caps;
  caps.encoding = GrayscaleEncoding::AbsolutePlanes;
  caps.base = GrayscaleBase::Separate;
  caps.asyncBase = false;
  caps.stagingWhileBusy = false;
  return caps;
}

void Ssd2677Driver::copyGrayscaleLsb(EpdBus& bus, const uint8_t* lsb) {
  (void)bus;
  if (!lsb) return;
  if (!_gray4Fb) {
#if defined(BOARD_HAS_PSRAM)
    _gray4Fb = static_cast<uint8_t*>(heap_caps_malloc(_gray4BufferSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
#else
    _gray4Fb = static_cast<uint8_t*>(malloc(_gray4BufferSize));
#endif
  }
  if (!_gray4Fb) return;

  for (size_t i = 0; i < _bufferSize; i++) {
    const uint8_t byteLsb = lsb[i];
    const size_t outIdx = i * 2;
    for (uint8_t b = 0; b < 4; b++) {
      const uint8_t bit = (byteLsb >> (7 - b)) & 1;
      _gray4Fb[outIdx] = (_gray4Fb[outIdx] & ~(1 << (b * 2))) | (bit << (b * 2));
    }
    for (uint8_t b = 0; b < 4; b++) {
      const uint8_t bit = (byteLsb >> (3 - b)) & 1;
      _gray4Fb[outIdx + 1] = (_gray4Fb[outIdx + 1] & ~(1 << (b * 2))) | (bit << (b * 2));
    }
  }
}

void Ssd2677Driver::copyGrayscaleMsb(EpdBus& bus, const uint8_t* msb) {
  (void)bus;
  if (!msb) return;
  if (!_gray4Fb) {
#if defined(BOARD_HAS_PSRAM)
    _gray4Fb = static_cast<uint8_t*>(heap_caps_malloc(_gray4BufferSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
#else
    _gray4Fb = static_cast<uint8_t*>(malloc(_gray4BufferSize));
#endif
  }
  if (!_gray4Fb) return;

  for (size_t i = 0; i < _bufferSize; i++) {
    const uint8_t byteMsb = msb[i];
    const size_t outIdx = i * 2;
    for (uint8_t b = 0; b < 4; b++) {
      const uint8_t bit = (byteMsb >> (7 - b)) & 1;
      _gray4Fb[outIdx] = (_gray4Fb[outIdx] & ~(2 << (b * 2))) | (bit << (b * 2 + 1));
    }
    for (uint8_t b = 0; b < 4; b++) {
      const uint8_t bit = (byteMsb >> (3 - b)) & 1;
      _gray4Fb[outIdx + 1] = (_gray4Fb[outIdx + 1] & ~(2 << (b * 2))) | (bit << (b * 2 + 1));
    }
  }
  _gray4Valid = true;
}

void Ssd2677Driver::displayGray(EpdBus& bus, const uint8_t* fb, bool turnOff, const unsigned char* lut, bool factoryMode) {
  (void)lut; (void)factoryMode;
  if (!_gray4Fb || !_gray4Valid) {
    display(bus, fb, nullptr, RefreshMode::Full, turnOff);
    return;
  }

  latchWaveform(bus, RefreshMode::Full, true);
  streamGray4(bus);

  if (!_refreshPowered) {
    bus.cmd(0x04);
    bus.waitBusy("ssd2677_gray_pon");
    _refreshPowered = true;
  }

  const uint8_t zero = 0x00;
  bus.cmd(0x12);
  bus.data(zero);
  bus.waitBusy("ssd2677_gray_drf_done");

  if (turnOff) {
    bus.cmd(0x02);
    bus.data(zero);
    bus.waitBusy("ssd2677_gray_pof");
    _refreshPowered = false;
  }

  if (_prevFb && fb) {
    memcpy(_prevFb, fb, _bufferSize);
  }
  _gray4Valid = false;
}

PanelDriver& ssd2677Driver() {
  static Ssd2677Driver instance;
  return instance;
}

}  // namespace freeink
