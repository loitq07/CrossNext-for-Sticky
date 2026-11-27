#pragma once

#include "../../activities/UiListActivity.h"
#include "../home/HomeMenuOrderUi.h"
#include "../ui/PixelMenu.h"
#include "../ui/QuickActionSupport.h"
#include "../ui/QuickSettingsScreen.h"
#include "QuickMenuStore.h"

class QuickSettingsConfigActivity final : public UiListActivity {
 public:
  QuickSettingsConfigActivity(GfxRenderer& renderer, MappedInputManager& input)
      : UiListActivity("QuickSettingsConfig", renderer, input) {}
  void onEnter() override;

 protected:
  int listCount() const override;
  const char* headerTitle() const override;
  void buildScreen(UiScreen&) override;
  void activateIndex(int index) override;
  void onBackButton() override;
  void drawChrome() override;
  void drawFooter() override {}

 private:
  enum class View { Root, Visibility, ChooseGame, ChooseApp, Order };
  View view = View::Root;
  View pickerReturnView = View::Visibility;
  quickmenu::Config draft;
  uint32_t eligibleMask = 0;
  quickui::Shortcut baseChoices[quickmenu::kBaseCount]{};
  int baseCount = 0;
  int homeSectionStart = 0;
  pixelui::RowSource rowText;
  freeink::ui::ListProps props;
  homemenuui::OrderWork orderWork;
  char subtitleBuf[48]{};
  const char* noticeText = nullptr;
  bool plugins = false;

  void show(View next, View returnView = View::Visibility);
  bool save();
  int visibilityRows() const;
  int pickerFolder() const { return view == View::ChooseGame ? 0 : 1; }
  quickui::Shortcut idAtRow(int index) const;
  static void provideRow(void* context, uint16_t index, freeink::ui::ListItem& row);
  static void moveAction(const freeink::ui::ActionEvent& event, void* context);
};
