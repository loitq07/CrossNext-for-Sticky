#include "Microphone.h"

#include <BoardConfig.h>

// Capability-gated like AudioManager/FrontlightManager: boards without
// FREEINK_CAP_MIC compile the stub bodies at the bottom and link no I2S code.
#if FREEINK_CAP_MIC

#include <driver/gpio.h>
#include <driver/i2s_pdm.h>
#include <algorithm>
#include <cstdlib>
#if defined(CONFIG_IDF_TARGET_ESP32S3)
#include <hal/usb_serial_jtag_ll.h>
#include <soc/usb_serial_jtag_reg.h>
#include <soc/rtc_cntl_reg.h>
#endif
#if FREEINK_DEVICE_METALIO_EINK4
#include <MetalioAudio.h>
#endif

namespace freeink {

namespace {
// Assert/deassert the mic power-enable rail per the board profile's polarity.
void setMicPower(const BoardConfig::MicConfig& mic, bool on) {
  if (mic.enable == BoardConfig::PIN_UNASSIGNED) return;
  pinMode(mic.enable, OUTPUT);
  digitalWrite(mic.enable, (on == mic.enableActiveHigh) ? HIGH : LOW);
}
}  // namespace

bool Microphone::begin(uint32_t sampleRate) {
  if (begun_) return sampleRate_ == sampleRate;
#if FREEINK_DEVICE_METALIO_EINK4
  if (BoardConfig::ACTIVE.mic.input == BoardConfig::MicInput::MetalioModule) {
    if (sampleRate != metalio::AUDIO_RATE || !metalio::acquireAudio(this, true)) return false;
    if (!metalio::startAudio(this, true)) {
      metalio::releaseAudio(this, true);
      return false;
    }
    rxChan_ = metalio::audioBus().rx;
    sampleRate_ = sampleRate;
    begun_ = true;
    return true;
  }
#endif
  const BoardConfig::MicConfig& mic = BoardConfig::ACTIVE.mic;
  if (mic.input != BoardConfig::MicInput::Pdm || mic.clk == BoardConfig::PIN_UNASSIGNED ||
      mic.data == BoardConfig::PIN_UNASSIGNED) {
    return false;
  }

#if defined(CONFIG_IDF_TARGET_ESP32S3)
  // On ESP32-S3, GPIO 19 and 20 default to the internal USB-Serial-JTAG controller.
  // We must release the physical pads and disable DP_PULLUP so GPIO 19 (CLK) and 20 (DATA)
  // work as I2S PDM peripheral lines.
  CLEAR_PERI_REG_MASK(USB_SERIAL_JTAG_CONF0_REG, USB_SERIAL_JTAG_USB_PAD_ENABLE | USB_SERIAL_JTAG_DP_PULLUP);
  CLEAR_PERI_REG_MASK(RTC_CNTL_USB_CONF_REG,
                      RTC_CNTL_SW_HW_USB_PHY_SEL | RTC_CNTL_SW_USB_PHY_SEL | RTC_CNTL_USB_PAD_ENABLE);
#endif

  // Reset GPIOs from USB-JTAG or previous state
  gpio_reset_pin(static_cast<gpio_num_t>(mic.clk));
  gpio_reset_pin(static_cast<gpio_num_t>(mic.data));

  setMicPower(mic, false);
  delay(20);
  setMicPower(mic, true);
  // PDM mics need a brief settle after the rail comes up before the clock runs.
  delay(50);

  i2s_chan_handle_t rx = nullptr;
  i2s_chan_config_t chanCfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  if (i2s_new_channel(&chanCfg, nullptr, &rx) != ESP_OK) {
    setMicPower(mic, false);
    return false;
  }

  i2s_pdm_rx_config_t pdmCfg = {
      .clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(sampleRate),
      .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
      .gpio_cfg =
          {
              .clk = static_cast<gpio_num_t>(mic.clk),
              .din = static_cast<gpio_num_t>(mic.data),
              .invert_flags = {.clk_inv = false},
          },
  };
#if SOC_I2S_SUPPORTS_PDM_RX_HP_FILTER
  pdmCfg.slot_cfg.hp_en = true;
  pdmCfg.slot_cfg.hp_cut_off_freq_hz = 35.5f;
  pdmCfg.slot_cfg.amplify_num = 5;  // Hardware 5x digital amplification
#endif

  if (i2s_channel_init_pdm_rx_mode(rx, &pdmCfg) != ESP_OK || i2s_channel_enable(rx) != ESP_OK) {
    i2s_del_channel(rx);
    setMicPower(mic, false);
    return false;
  }

  rxChan_ = rx;
  sampleRate_ = sampleRate;
  begun_ = true;
  return true;
}

int Microphone::read(int16_t* dst, size_t maxSamples, uint32_t timeoutMs) {
  if (!begun_ || !rxChan_ || !dst || maxSamples == 0) return -1;
#if FREEINK_DEVICE_METALIO_EINK4
  if (BoardConfig::ACTIVE.mic.input == BoardConfig::MicInput::MetalioModule) {
    // Bounded scratch space; callers can pump read() for larger recordings.
    int32_t slots[64];
    const size_t frames = maxSamples < 32 ? maxSamples : 32;
    size_t bytes = 0;
    const esp_err_t err = i2s_channel_read(static_cast<i2s_chan_handle_t>(rxChan_), slots,
                                          frames * 2 * sizeof(int32_t), &bytes, timeoutMs);
    if (err != ESP_OK && err != ESP_ERR_TIMEOUT) return -1;
    const size_t count = bytes / (2 * sizeof(int32_t));
    for (size_t i = 0; i < count; ++i) dst[i] = metalio::inputSample(slots[i * 2]);
    return static_cast<int>(count);
  }
#endif
  constexpr size_t kChunk = 128;
  int16_t stereo[kChunk * 2];
  size_t total = 0;
  while (total < maxSamples) {
    const size_t toRead = std::min<size_t>(kChunk, maxSamples - total);
    size_t bytesRead = 0;
    const esp_err_t err = i2s_channel_read(static_cast<i2s_chan_handle_t>(rxChan_), stereo,
                                           toRead * 2 * sizeof(int16_t), &bytesRead, pdMS_TO_TICKS(timeoutMs));
    if (err == ESP_ERR_TIMEOUT) {
      if (total > 0) break;
      return 0;
    }
    if (err != ESP_OK) {
      if (total > 0) break;
      return -1;
    }
    const size_t frames = bytesRead / (2 * sizeof(int16_t));
    if (frames == 0) break;
    for (size_t i = 0; i < frames; ++i) {
      const int16_t l = stereo[i * 2];
      const int16_t r = stereo[i * 2 + 1];
      dst[total + i] = (std::abs(static_cast<int>(l)) >= std::abs(static_cast<int>(r))) ? l : r;
    }
    total += frames;
    if (frames < toRead) break;
  }
  return static_cast<int>(total);
}

void Microphone::end() {
#if FREEINK_DEVICE_METALIO_EINK4
  if (BoardConfig::ACTIVE.mic.input == BoardConfig::MicInput::MetalioModule) {
    metalio::releaseAudio(this, true);
    rxChan_ = nullptr;
    begun_ = false;
    sampleRate_ = 0;
    return;
  }
#endif
  if (rxChan_) {
    i2s_channel_disable(static_cast<i2s_chan_handle_t>(rxChan_));
    i2s_del_channel(static_cast<i2s_chan_handle_t>(rxChan_));
    rxChan_ = nullptr;
  }
  setMicPower(BoardConfig::ACTIVE.mic, false);
  begun_ = false;
  sampleRate_ = 0;
}

}  // namespace freeink

#else  // FREEINK_CAP_MIC — no mic on this board: stub bodies, no I2S linkage.

namespace freeink {
bool Microphone::begin(uint32_t) { return false; }
int Microphone::read(int16_t*, size_t, uint32_t) { return -1; }
void Microphone::end() {}
}  // namespace freeink

#endif  // FREEINK_CAP_MIC
