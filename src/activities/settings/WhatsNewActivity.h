#pragma once
#include <string>

#include "activities/UiListActivity.h"

class WhatsNewActivity final : public UiListActivity {
 public:
  explicit WhatsNewActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);

  static constexpr int ITEM_COUNT = 6;

  void onEnter() override;

 private:
  int listCount() const override { return ITEM_COUNT; }
  void buildScreen(UiScreen& screen) override;
  void activateIndex(int index) override;
  void onBackButton() override;
  void dismiss();
  const char* headerTitle() const override;

  std::string rowLabels_[ITEM_COUNT];
  std::string rowSubtitles_[ITEM_COUNT];
  freeink::ui::ListItem rowItems_[ITEM_COUNT]{};
};
