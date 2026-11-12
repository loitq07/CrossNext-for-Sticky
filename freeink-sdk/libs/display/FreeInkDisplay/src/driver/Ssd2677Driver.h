#pragma once

// FreeInk SDK — SSD2677 panel driver.
//
// SSD2677 e-paper controller (800x480) for Seeed reTerminal Sticky new hardware revisions.
// Features active-low BUSY line, built-in OTP waveforms (full, partial, and 4-gray),
// and interleaved 2-bpp streaming to command 0x10.
// Temperature is dynamically read or latched to choose the optimal OTP waveform bucket.

#include "PanelDriver.h"

namespace freeink {

struct Ssd2677Config {
  uint32_t spiHz = 10000000;
  uint8_t fallbackTemperature = 25;
};

const Ssd2677Config& ssd2677DefaultConfig();

class Ssd2677Driver : public PanelDriver {
 public:
  explicit Ssd2677Driver(const Ssd2677Config& cfg = ssd2677DefaultConfig());
  ~Ssd2677Driver() override;

  uint32_t spiHz() const override;
  BusyPolarity busyPolarity() const override { return BusyPolarity::ActiveLow; }
  PanelGeometry geometry() const override;

  void begin(EpdBus& bus) override;
  void deepSleep(EpdBus& bus) override;

  void display(EpdBus& bus, const uint8_t* fb, const uint8_t* prev, RefreshMode mode, bool turnOff) override;
  bool displayStart(EpdBus& bus, const uint8_t* fb, const uint8_t* prev, RefreshMode mode, bool turnOff) override;
  void displayFinish(EpdBus& bus, const uint8_t* fb) override;
  bool supportsAsyncDisplay() const override { return true; }
  void displayWindow(EpdBus& bus, const uint8_t* fb, const uint8_t* prev, uint16_t x, uint16_t y, uint16_t w,
                     uint16_t h, bool turnOff) override;

  void seedPreviousFrame(EpdBus& bus, const uint8_t* buf) override;

  GrayscaleCapabilities grayscaleCapabilities(GrayscaleMode mode = GrayscaleMode::Overlay) const override;
  void copyGrayscaleLsb(EpdBus& bus, const uint8_t* lsb) override;
  void copyGrayscaleMsb(EpdBus& bus, const uint8_t* msb) override;
  void displayGray(EpdBus& bus, const uint8_t* fb, bool turnOff, const unsigned char* lut, bool factoryMode) override;

 private:
  void initController(EpdBus& bus);
  uint8_t readTemperature(EpdBus& bus);
  void latchWaveform(EpdBus& bus, RefreshMode mode, bool gray4);
  void streamFullMono(EpdBus& bus, const uint8_t* fb);
  void streamPartialDiff(EpdBus& bus, const uint8_t* prev, const uint8_t* curr);
  void streamGray4(EpdBus& bus);

  Ssd2677Config _cfg;
  uint16_t _w;
  uint16_t _h;
  uint16_t _wb;
  uint32_t _bufferSize;
  uint32_t _gray4BufferSize;

  bool _refreshPowered = false;
  uint8_t* _prevFb = nullptr;
  uint8_t* _gray4Fb = nullptr;
  bool _gray4Valid = false;
  RefreshMode _lastRefreshMode = RefreshMode::Full;
};

PanelDriver& ssd2677Driver();

}  // namespace freeink
