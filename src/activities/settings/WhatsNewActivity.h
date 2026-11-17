#pragma once
#include <string>

#include "activities/UiListActivity.h"

class WhatsNewActivity final : public UiListActivity {
 public:
  explicit WhatsNewActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);

  static constexpr int ITEM_COUNT = 6;

  // setup() arms this on the first ordinary boot of a new version; Home takes
  // it on its first loop so What's New stacks over Home (and pops back to it)
  // ahead of Home's own first-visit prompts, never over the boot splash.
  static void armForBoot() { bootPending_ = true; }
  static bool takeBootPending() {
    const bool pending = bootPending_;
    bootPending_ = false;
    return pending;
  }

  void onEnter() override;

 private:
  int listCount() const override { return ITEM_COUNT; }
  void buildScreen(UiScreen& screen) override;
  void activateIndex(int index) override;
  void onBackButton() override;
  void dismiss();
  const char* headerTitle() const override;

  static inline bool bootPending_ = false;

  std::string rowLabels_[ITEM_COUNT];
  std::string rowSubtitles_[ITEM_COUNT];
  freeink::ui::ListItem rowItems_[ITEM_COUNT]{};
};
