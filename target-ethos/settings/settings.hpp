struct SettingsLayout : HorizontalLayout {
  Widget spacer;
  VerticalLayout layout;

  void append(Sizable &widget, const Size &size, unsigned spacing = 0);
  SettingsLayout();
};

#include "video.hpp"
#include "input.hpp"
#include "hotkey.hpp"
#include "advanced.hpp"

struct Settings : Window {
  HorizontalLayout layout;
  ListView panelList;

  void panelChanged();
  Settings();
};

extern Settings *settings;
