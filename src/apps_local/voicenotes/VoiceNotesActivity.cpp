#include "VoiceNotesActivity.h"

#include <Arduino.h>
#include <HalStorage.h>
#include <I18n.h>
#include <Logging.h>
#include <WiFi.h>
#include <ctime>

#include "../../CrossPointSettings.h"
#include "../../HapticFeedback.h"
#include "../../WifiCredentialStore.h"
#include "../Shelf.h"
#include "../ui/Dashboard.h"
#include "../ui/ExtraIcons.h"
#include "../ui/PixelMenu.h"
#include "../ui/PixelMenuTheme.h"
#include "../ui/ToyboxFonts.h"
#include "../ui/ToyboxScreen.h"
#include "../ui/ToyboxTheme.h"

namespace voicenotes {

namespace {

namespace fui = freeink::ui;

enum Actions : fui::ActionId {
  ActionNone = 0,
  ActionBack = 1,
  ActionRecord = 2,
  ActionStop = 3,
  ActionCancel = 4,
  ActionReload = 5,
  ActionReset = 6
};

}  // namespace

VoiceNotesActivity::VoiceNotesActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("VoiceNotes", renderer, mappedInput) {}

VoiceNotesActivity::~VoiceNotesActivity() {
  recorder_.stopRecording();
}

std::unique_ptr<Activity> VoiceNotesActivity::create(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return std::make_unique<VoiceNotesActivity>(renderer, mappedInput);
}

void VoiceNotesActivity::onEnter() {
  Activity::onEnter();
  mappedInput.setEdgeBackEnabled(true);
  toybox::ensureFonts(renderer);

  loadConfig(config_);

  state_ = State::READY;
  statusMessage_ = "Sẵn sàng ghi âm";
  requestUpdate();
}

void VoiceNotesActivity::onExit() {
  recorder_.stopRecording();
  Activity::onExit();
}

void VoiceNotesActivity::handleReloadConfig() {
  HalBuzzer::click(SETTINGS.buzzerVolume);
  loadConfig(config_);
  if (config_.apiKey.empty()) {
    errorMessage_ = "Chưa tìm thấy API Key! Vui lòng kiểm tra file /settings/voice_ai.json trên thẻ nhớ.";
    state_ = State::ERROR_STATE;
  } else {
    state_ = State::READY;
  }
  requestUpdate();
}

void VoiceNotesActivity::startRecording() {
  if (state_ == State::RECORDING) return;

  if (config_.apiKey.empty()) {
    loadConfig(config_);
    if (config_.apiKey.empty()) {
      state_ = State::READY;
      requestUpdate();
      return;
    }
  }

  if (!recorder_.startRecording()) {
    state_ = State::ERROR_STATE;
    errorMessage_ = "Không thể khởi động microphone (lỗi phần cứng I2S).";
    requestUpdate();
    return;
  }

  recordingStartMs_ = millis();
  state_ = State::RECORDING;
  lastRenderedSec_ = -1;
  lastRenderedLevel_ = -1;
  lastRenderMs_ = millis();
  requestUpdate();
}

void VoiceNotesActivity::finishRecordingAndTranscribe() {
  recorder_.stopRecording();
  recorder_.saveWav("/notes/last_recording.wav");

  const uint32_t dur = recorder_.durationMs();
  if (dur < 1200) {
    LOG_ERR("VOICE", "Recording too short: %u ms", static_cast<unsigned>(dur));
    state_ = State::ERROR_STATE;
    errorMessage_ = "Thời gian ghi âm quá ngắn (< 1.2s)! Vui lòng nói trọn vẹn câu rồi mới bấm dừng.";
    requestUpdate();
    return;
  }

  if (recorder_.recordedSamples() == 0 || recorder_.maxPeak() < 30) {
    LOG_ERR("VOICE", "Audio silence detected (maxPeak=%ld, samples=%u)",
            static_cast<long>(recorder_.maxPeak()), static_cast<unsigned>(recorder_.recordedSamples()));
    state_ = State::ERROR_STATE;
    errorMessage_ = "Không thu được âm thanh từ micro (quá im lặng). Vui lòng thử nói to hơn hoặc đưa máy lại gần!";
    requestUpdate();
    return;
  }

  state_ = State::TRANSCRIBING;
  statusMessage_ = "Đang gửi dữ liệu tới Groq Whisper AI...";
  requestUpdateAndWait();

  if (WiFi.status() != WL_CONNECTED) {
    const std::string lastSsid = WIFI_STORE.getLastConnectedSsid();
    if (!lastSsid.empty()) {
      auto cred = WIFI_STORE.findCredential(lastSsid);
      if (cred.has_value()) {
        LOG_INF("VOICE", "Connecting to Wi-Fi (%s)...", cred->ssid.c_str());
        WiFi.begin(cred->ssid.c_str(), cred->password.c_str());
        const uint32_t startWait = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - startWait < 8000)) {
          delay(100);
        }
      }
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    state_ = State::ERROR_STATE;
    errorMessage_ = "Chưa kết nối Wi-Fi! Vui lòng kết nối Wi-Fi trong Cài đặt trước khi ghi âm.";
    requestUpdate();
    return;
  }

  const TranscribeResult res = transcribeAudio(config_, recorder_.pcmData(), recorder_.recordedBytes());

  if (res.status == TranscribeStatus::SUCCESS) {
    // Filter out known Whisper YouTube outro hallucinations unconditionally
    std::string lower = res.text;
    for (char& c : lower) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));

    bool isHallucination = false;
    if (lower.find("subscribe") != std::string::npos ||
        lower.find("ghiền mì gõ") != std::string::npos ||
        lower.find("ghien mi go") != std::string::npos ||
        lower.find("nghiền mì gõ") != std::string::npos ||
        lower.find("la la school") != std::string::npos ||
        lower.find("đăng ký kênh") != std::string::npos ||
        lower.find("dang ky kenh") != std::string::npos ||
        lower.find("kênh youtube") != std::string::npos ||
        lower.find("video hấp dẫn") != std::string::npos ||
        lower.find("cảm ơn các bạn đã theo dõi") != std::string::npos ||
        lower.find("like và chia sẻ") != std::string::npos) {
      isHallucination = true;
    }

    if (isHallucination) {
      LOG_ERR("VOICE", "Filtered Whisper hallucination: %s", res.text.c_str());
      state_ = State::ERROR_STATE;
      errorMessage_ = "Không phát hiện được giọng nói rõ ràng. Vui lòng thử nói to hơn gần micro!";
    } else {
      transcribedText_ = res.text;
      state_ = State::RESULT;
      saveTranscribedNote();
    }
  } else {
    state_ = State::ERROR_STATE;
    errorMessage_ = res.errorMessage;
  }

  requestUpdate();
}

void VoiceNotesActivity::cancelRecording() {
  recorder_.stopRecording();
  state_ = State::READY;
  statusMessage_ = "Đã hủy ghi âm";
  requestUpdate();
}

void VoiceNotesActivity::saveTranscribedNote() {
  if (transcribedText_.empty()) return;

  Storage.ensureDirectoryExists("/notes");

  char filename[64];
  const time_t now = time(nullptr);
  struct tm t;
  if (now > 1600000000 && localtime_r(&now, &t)) {
    std::snprintf(filename, sizeof(filename), "/notes/Voice_%04d-%02d-%02d_%02d%02d%02d.md", t.tm_year + 1900,
                  t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
  } else {
    std::snprintf(filename, sizeof(filename), "/notes/Voice_Note_%lu.md", static_cast<unsigned long>(millis() / 1000));
  }

  std::string content = "# Ghi chú thoại (Voice Note)\n\n";
  content += transcribedText_;
  content += "\n\n---\n*Chuyển ngữ bởi Groq Whisper trên reTerminal Sticky*\n";

  if (Storage.writeFile(filename, String(content.c_str()))) {
    lastSavedPath_ = filename;
    LOG_INF("VOICE", "Saved note to %s", filename);
  } else {
    LOG_ERR("VOICE", "Failed to save note to %s", filename);
  }
}

void VoiceNotesActivity::handleAction(fui::ActionId action) {
  switch (action) {
    case ActionBack:
      if (state_ == State::READY) {
        shelf::leave(renderer, mappedInput);
      } else {
        state_ = State::READY;
        requestUpdate();
      }
      break;

    case ActionRecord:
      startRecording();
      break;

    case ActionStop:
      if (millis() - recordingStartMs_ < 1200) {
        LOG_INF("VOICE", "Premature stop click ignored (debounce)");
        return;
      }
      finishRecordingAndTranscribe();
      break;

    case ActionCancel:
      cancelRecording();
      break;

    case ActionReload:
      handleReloadConfig();
      break;

    case ActionReset:
      state_ = State::READY;
      requestUpdate();
      break;

    default:
      break;
  }
}

void VoiceNotesActivity::handleInput() {
  const bool backPressed = mappedInput.wasPressed(MappedInputManager::Button::Back) ||
                           mappedInput.wasReleased(MappedInputManager::Button::Back);
  const bool confirmPressed = mappedInput.wasPressed(MappedInputManager::Button::Confirm);
  if (backPressed) {
    if (state_ == State::RECORDING) {
      cancelRecording();
    } else if (state_ == State::RESULT || state_ == State::ERROR_STATE) {
      state_ = State::READY;
      requestUpdate();
    } else {
      shelf::leave(renderer, mappedInput);
    }
    return;
  }

  if (confirmPressed) {
    haptic_feedback::touchAction();
    if (state_ == State::READY) {
      if (config_.apiKey.empty()) {
        handleReloadConfig();
      } else {
        startRecording();
      }
    } else if (state_ == State::RECORDING) {
      if (millis() - recordingStartMs_ >= 1200) {
        finishRecordingAndTranscribe();
      }
    } else if (state_ == State::RESULT || state_ == State::ERROR_STATE) {
      state_ = State::READY;
      requestUpdate();
    }
    return;
  }

  int tapX = 0, tapY = 0;
  if (mappedInput.wasScreenTapped(tapX, tapY)) {
    fui::InputSnapshot tap;
    tap.touchReleased = true;
    tap.touchX = tapX;
    tap.touchY = tapY;
    const auto event = interactions_.route(tap);
    if (event.action) {
      if (event.action == ActionStop && millis() - recordingStartMs_ < 1200) {
        LOG_INF("VOICE", "Tap Stop ignored: premature debounce");
        return;
      }
      haptic_feedback::touchAction();
      handleAction(event.action);
      return;
    }
  }
}

void VoiceNotesActivity::loop() {
  handleInput();

  if (state_ == State::RECORDING) {
    recorder_.update();

    if (!recorder_.isRecording()) {
      finishRecordingAndTranscribe();
      return;
    }

    // Only refresh display ONCE PER SECOND to avoid blocking touch input with E-Ink refreshes
    const int sec = static_cast<int>(recorder_.durationMs() / 1000);
    if (sec != lastRenderedSec_) {
      lastRenderedSec_ = sec;
      requestUpdate(true);
    }
  }
}

void VoiceNotesActivity::render(RenderLock&&) {
  interactions_.clear();
  renderer.clearScreen();

  auto target = toybox::makeTarget(renderer);
  pixelui::bindFonts(target, renderer, I18N.getLanguage() != Language::EN);

  const fui::InputSnapshot noInput{};
  toybox::Frame frame(target, target.deviceContext(), noInput, interactions_);
  toybox::Screen screen(frame);

  const bool isVi = (I18N.getLanguage() == Language::VI);

  // Chrome Header
  pixelui::Header header;
  header.title = isVi ? "GHI CHÚ THOẠI" : "VOICE NOTES";
  header.back = screen.device().hasTouch;
  header.wifi = (WiFi.status() == WL_CONNECTED);
  if (state_ == State::RECORDING) {
    header.rightLabel = "REC";
  }
  pixelui::header(screen, header);

  const auto body = pixelui::geometry(screen.device()).body;
  constexpr int kPad = 6;
  const int contentW = body.width - 2 * kPad;
  const int startX = body.x + kPad;

  switch (state_) {
    case State::READY: {
      if (config_.apiKey.empty()) {
        // --- SETUP GUIDE SCREEN ---
        const auto noticeRect = fui::makeRect(startX, body.y + 4, contentW, 44);
        dashboard::button(screen, noticeRect, isVi ? "CHƯA CÀI ĐẶT GROQ API KEY" : "GROQ API KEY REQUIRED",
                          ActionReload, 0, true);

        const int guideH = std::min(400, static_cast<int>(body.height) - 180);
        const auto guideBox = fui::makeRect(startX, noticeRect.bottom() + 12, contentW, guideH);
        dashboard::frame(screen, guideBox, 2);

        int textY = guideBox.y + 14;
        const int innerW = guideBox.width - 24;
        const int innerX = guideBox.x + 12;

        // Step 1
        dashboard::text(screen, fui::makeRect(innerX, textY, innerW, 24),
                        isVi ? "1. Lấy Groq API Key miễn phí (30s):" : "1. Get free Groq API Key (30s):",
                        pixelui::kSmall, fui::TextAlign::Left);
        textY += 26;
        dashboard::prose(screen, fui::makeRect(innerX, textY, innerW, 54),
                         isVi ? "Vào console.groq.com/keys trên máy tính/điện thoại để tạo API key miễn phí (dạng gsk_...)."
                              : "Visit console.groq.com/keys in browser to create a free API key (gsk_...).",
                         fui::TextAlign::Left, 3);
        textY += 62;

        // Step 2
        dashboard::text(screen, fui::makeRect(innerX, textY, innerW, 24),
                        isVi ? "2. Mở thẻ nhớ microSD trên máy tính:" : "2. Open microSD card on PC:",
                        pixelui::kSmall, fui::TextAlign::Left);
        textY += 26;
        dashboard::prose(screen, fui::makeRect(innerX, textY, innerW, 54),
                         isVi ? "Mở file /settings/voice_ai.json (hoặc /notes/voice_ai.json) trên thẻ nhớ microSD."
                              : "Open /settings/voice_ai.json (or /notes/voice_ai.json) on the microSD card.",
                         fui::TextAlign::Left, 3);
        textY += 62;

        // Step 3
        dashboard::text(screen, fui::makeRect(innerX, textY, innerW, 24),
                        isVi ? "3. Dán Key và lưu file:" : "3. Paste key and save file:",
                        pixelui::kSmall, fui::TextAlign::Left);
        textY += 26;
        dashboard::prose(screen, fui::makeRect(innerX, textY, innerW, 54),
                         isVi ? "Điền chuỗi key vào mục \"api_key\": \"gsk_...\" rồi cắm thẻ nhớ lại vào Sticky."
                              : "Paste key string into \"api_key\": \"gsk_...\" and insert card into Sticky.",
                         fui::TextAlign::Left, 3);

        // Action Reload Button
        const auto reloadRect = fui::makeRect(startX, guideBox.bottom() + 16, contentW, 58);
        dashboard::action(screen, reloadRect, isVi ? "ĐÃ THÊM KEY -> TẢI LẠI" : "KEY ADDED -> RELOAD",
                          ActionReload, nullptr, true);

        // Back to Menu Button
        const auto backRect = fui::makeRect(startX, reloadRect.bottom() + 12, contentW, 46);
        dashboard::button(screen, backRect, isVi ? "QUAY LẠI MENU" : "BACK TO MENU", ActionBack, 0, false);
      } else {
        // --- READY SCREEN ---
        const auto infoBox = fui::makeRect(startX, body.y + 8, contentW, 200);
        dashboard::frame(screen, infoBox, 2);

        dashboard::text(screen, fui::makeRect(infoBox.x + 12, infoBox.y + 14, infoBox.width - 24, 28),
                        isVi ? "CHUYỂN ĐỔI GIỌNG NÓI (GROQ AI)" : "VOICE TRANSCRIBING",
                        pixelui::kBody, fui::TextAlign::Left);
        screen.target().line({int16_t(infoBox.x + 12), int16_t(infoBox.y + 46)},
                             {int16_t(infoBox.right() - 12), int16_t(infoBox.y + 46)}, 1,
                             fui::Paint::solid(fui::Color::Black));

        dashboard::text(screen, fui::makeRect(infoBox.x + 14, infoBox.y + 56, infoBox.width - 28, 22),
                        isVi ? "• Mô hình: Whisper Large v3 Turbo (Cloud AI)" : "• Model: Whisper Large v3 Turbo",
                        pixelui::kSmall, fui::TextAlign::Left);
        dashboard::text(screen, fui::makeRect(infoBox.x + 14, infoBox.y + 84, infoBox.width - 28, 22),
                        isVi ? "• Ngôn ngữ: Tiếng Việt (vi) / Tự động" : "• Language: Vietnamese (vi) / Auto",
                        pixelui::kSmall, fui::TextAlign::Left);
        dashboard::text(screen, fui::makeRect(infoBox.x + 14, infoBox.y + 112, infoBox.width - 28, 22),
                        isVi ? "• Lưu file: Tự động ghi vào /notes/*.md" : "• Storage: Auto-saved to /notes/*.md",
                        pixelui::kSmall, fui::TextAlign::Left);

        if (!lastSavedPath_.empty()) {
          dashboard::text(screen, fui::makeRect(infoBox.x + 14, infoBox.y + 148, infoBox.width - 28, 20),
                          isVi ? "Ghi chú vừa tạo:" : "Last saved note:", pixelui::kSmall, fui::TextAlign::Left);
          dashboard::text(screen, fui::makeRect(infoBox.x + 14, infoBox.y + 170, infoBox.width - 28, 20),
                          lastSavedPath_.c_str(), pixelui::kSmall, fui::TextAlign::Left);
        }

        // Prominent Record Action Button
        const auto recBox = fui::makeRect(startX, infoBox.bottom() + 28, contentW, 80);
        dashboard::action(screen, recBox, isVi ? "BẮT ĐẦU GHI ÂM" : "START RECORDING",
                          ActionRecord, &extra_mic_32, true);

        // Subtext hint
        dashboard::text(screen, fui::makeRect(startX, recBox.bottom() + 10, contentW, 26),
                        isVi ? "Chạm nút trên hoặc bấm nút Nguồn để thu âm" : "Tap above or press Power to record",
                        pixelui::kSmall, fui::TextAlign::Center);

        // Back button at bottom
        const auto backRect = fui::makeRect(startX, body.bottom() - 56, contentW, 48);
        dashboard::button(screen, backRect, isVi ? "QUAY LẠI MENU" : "BACK TO MENU", ActionBack, 0, false);
      }
      break;
    }

    case State::RECORDING: {
      // --- RECORDING SCREEN ---
      const auto cardBox = fui::makeRect(startX, body.y + 16, contentW, 260);
      dashboard::frame(screen, cardBox, 2);

      // Title
      dashboard::text(screen, fui::makeRect(cardBox.x + 12, cardBox.y + 18, cardBox.width - 24, 28),
                      isVi ? "ĐANG GHI ÂM..." : "RECORDING...", pixelui::kBody, fui::TextAlign::Center);

      // Big Timer
      const int sec = static_cast<int>(recorder_.durationMs() / 1000);
      char timeBuf[32];
      std::snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d / 00:30", sec / 60, sec % 60);
      dashboard::text(screen, fui::makeRect(cardBox.x + 12, cardBox.y + 60, cardBox.width - 24, 48),
                      timeBuf, pixelui::kTitle, fui::TextAlign::Center);

      // Audio Level Bar
      const auto meterFrame = fui::makeRect(cardBox.x + 28, cardBox.y + 130, cardBox.width - 56, 18);
      screen.target().stroke(meterFrame, fui::Paint::solid(fui::Color::Black), 2);
      const int curLvl = std::min(100, std::max(0, recorder_.audioLevel()));
      const int fillW = (meterFrame.width - 4) * curLvl / 100;
      if (fillW > 0) {
        screen.target().fill(fui::makeRect(meterFrame.x + 2, meterFrame.y + 2, fillW, meterFrame.height - 4),
                             fui::Paint::solid(fui::Color::Black));
      }

      dashboard::text(screen, fui::makeRect(cardBox.x + 12, cardBox.y + 165, cardBox.width - 24, 22),
                      isVi ? "Nói rõ vào micro ở cạnh máy..." : "Speak into the microphone...",
                      pixelui::kSmall, fui::TextAlign::Center);
      dashboard::text(screen, fui::makeRect(cardBox.x + 12, cardBox.y + 195, cardBox.width - 24, 22),
                      isVi ? "Bấm Nguồn hoặc chạm Hoàn tất để kết thúc" : "Press Power or Stop to finish",
                      pixelui::kSmall, fui::TextAlign::Center);

      // Stop & Transcribe Button
      const auto stopBox = fui::makeRect(startX, cardBox.bottom() + 24, contentW, 68);
      dashboard::button(screen, stopBox, isVi ? "HOÀN TẤT & CHUYỂN VĂN BẢN" : "FINISH & TRANSCRIBE",
                        ActionStop, 0, true);

      // Cancel Button
      const auto cancelBox = fui::makeRect(startX, stopBox.bottom() + 14, contentW, 50);
      dashboard::button(screen, cancelBox, isVi ? "HỦY GHI ÂM" : "CANCEL", ActionCancel, 0, false);
      break;
    }

    case State::TRANSCRIBING: {
      // --- TRANSCRIBING SCREEN ---
      const auto cardBox = fui::makeRect(startX, body.y + 80, contentW, 200);
      dashboard::frame(screen, cardBox, 2);

      dashboard::text(screen, fui::makeRect(cardBox.x + 12, cardBox.y + 30, cardBox.width - 24, 32),
                      isVi ? "ĐANG XỬ LÝ..." : "TRANSCRIBING...", pixelui::kTitle, fui::TextAlign::Center);
      dashboard::text(screen, fui::makeRect(cardBox.x + 12, cardBox.y + 80, cardBox.width - 24, 26),
                      isVi ? "Đang gửi âm thanh tới Groq Whisper AI..." : "Sending audio to Groq Whisper AI...",
                      pixelui::kBody, fui::TextAlign::Center);
      dashboard::text(screen, fui::makeRect(cardBox.x + 12, cardBox.y + 120, cardBox.width - 24, 22),
                      isVi ? "Vui lòng đợi trong giây lát" : "Please wait a moment",
                      pixelui::kSmall, fui::TextAlign::Center);
      break;
    }

    case State::RESULT: {
      // --- RESULT SCREEN ---
      const auto cardBox = fui::makeRect(startX, body.y + 8, contentW, std::min(400, static_cast<int>(body.height) - 150));
      dashboard::frame(screen, cardBox, 2);

      dashboard::text(screen, fui::makeRect(cardBox.x + 12, cardBox.y + 12, cardBox.width - 24, 26),
                      isVi ? "KẾT QUẢ CHUYỂN VĂN BẢN" : "TRANSCRIBED RESULT",
                      pixelui::kBody, fui::TextAlign::Left);
      screen.target().line({int16_t(cardBox.x + 12), int16_t(cardBox.y + 42)},
                           {int16_t(cardBox.right() - 12), int16_t(cardBox.y + 42)}, 1,
                           fui::Paint::solid(fui::Color::Black));

      // Transcribed text prose
      const auto textBox = fui::makeRect(cardBox.x + 12, cardBox.y + 52, cardBox.width - 24, cardBox.height - 100);
      dashboard::prose(screen, textBox, transcribedText_.c_str(), fui::TextAlign::Left, 8);

      // Note path at bottom of card
      if (!lastSavedPath_.empty()) {
        std::string savedLabel = (isVi ? "Đã lưu vào: " : "Saved to: ") + lastSavedPath_;
        dashboard::text(screen, fui::makeRect(cardBox.x + 12, cardBox.bottom() - 32, cardBox.width - 24, 22),
                        savedLabel.c_str(), pixelui::kSmall, fui::TextAlign::Left);
      }

      // Next Action buttons
      const auto newRecBox = fui::makeRect(startX, cardBox.bottom() + 18, contentW, 60);
      dashboard::button(screen, newRecBox, isVi ? "GHI ÂM TIẾP" : "RECORD AGAIN",
                        ActionRecord, 0, true, &extra_mic_32);

      const auto doneBox = fui::makeRect(startX, newRecBox.bottom() + 12, contentW, 48);
      dashboard::button(screen, doneBox, isVi ? "XONG - VỀ MENU" : "DONE - MENU",
                        ActionBack, 0, false);
      break;
    }

    case State::ERROR_STATE: {
      // --- ERROR SCREEN ---
      const auto cardBox = fui::makeRect(startX, body.y + 40, contentW, 260);
      dashboard::frame(screen, cardBox, 2);

      dashboard::text(screen, fui::makeRect(cardBox.x + 12, cardBox.y + 20, cardBox.width - 24, 30),
                      isVi ? "CÓ LỖI XẢY RA" : "ERROR OCCURRED", pixelui::kBody, fui::TextAlign::Center);
      screen.target().line({int16_t(cardBox.x + 20), int16_t(cardBox.y + 54)},
                           {int16_t(cardBox.right() - 20), int16_t(cardBox.y + 54)}, 1,
                           fui::Paint::solid(fui::Color::Black));

      const auto errBox = fui::makeRect(cardBox.x + 16, cardBox.y + 70, cardBox.width - 32, 140);
      dashboard::prose(screen, errBox, errorMessage_.c_str(), fui::TextAlign::Center, 5);

      const auto okBox = fui::makeRect(startX, cardBox.bottom() + 24, contentW, 56);
      dashboard::button(screen, okBox, isVi ? "QUAY LẠI" : "GO BACK", ActionReset, 0, true);
      break;
    }
  }

  renderer.displayBuffer(HalDisplay::FAST_REFRESH);
}

}  // namespace voicenotes
