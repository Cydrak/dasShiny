#include "../ethos.hpp"
#include "video.cpp"
#include "input.cpp"
#include "hotkey.cpp"
#include "advanced.cpp"
Settings *settings = nullptr;

void SettingsLayout::append(Sizable &sizable, const Size &size, unsigned spacing) {
  layout.append(sizable, size, spacing);
}

SettingsLayout::SettingsLayout() {
  setMargin(5);
  HorizontalLayout::append(spacer, {120, ~0}, 5);
  HorizontalLayout::append(layout, { ~0, ~0});
}

Settings::Settings() {
  setGeometry({128, 128, 640, 360});
  windowManager->append(this, "Settings");

  setTitle("Configuration Settings");
  setStatusVisible();

  layout.setMargin(5);
  panelList.setFont(program->boldFont);
  panelList.append("Audiovisual");
  panelList.append("Input");
  panelList.append("Hotkeys");
  panelList.append("Advanced");

  append(layout);
  layout.append(panelList, {120, ~0}, 5);
  append(*outputSettings);
  append(*inputSettings);
  append(*hotkeySettings);
  append(*advancedSettings);

  onClose = [&] {
    advancedSettings->analysis.stop = true;
    setVisible(false);
  };

  panelList.onChange = {&Settings::panelChanged, this};

  panelList.setSelection(2);
  panelChanged();
}

void Settings::panelChanged() {
  setStatusText("");
  outputSettings->setVisible(false);
  inputSettings->setVisible(false);
  hotkeySettings->setVisible(false);
  advancedSettings->setVisible(false);
  if(panelList.selected() == false) return;

  switch(panelList.selection()) {
  case 0: return outputSettings->setVisible();
  case 1: return inputSettings->setVisible();
  case 2: return hotkeySettings->setVisible();
  case 3: return advancedSettings->setVisible();
  }
}
