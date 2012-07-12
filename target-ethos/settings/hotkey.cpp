HotkeySettings *hotkeySettings = nullptr;

HotkeySettings::HotkeySettings() : activeInput(nullptr) {
  title.setFont(application->titleFont);
  title.setText("Hotkey Bindings");

  inputList.setHeaderText("Name", "Mapping");
  inputList.setHeaderVisible();
  eraseButton.setText("Erase");

  append(title, {~0, 0}, 5);
  append(inputList, {~0, ~0}, 5);
  append(controlLayout, {~0, 0});
    controlLayout.append(spacer, {~0, 0});
    controlLayout.append(eraseButton, {80, 0});

  inputList.onChange = {&HotkeySettings::synchronize, this};
  inputList.onActivate = {&HotkeySettings::assignInput, this};
  eraseButton.onActivate = {&HotkeySettings::eraseInput, this};

  for(auto &hotkey : inputManager->hotkeyMap) inputList.append("", "");
  refresh();
}

void HotkeySettings::synchronize() {
  eraseButton.setEnabled(inputList.selected());
}

void HotkeySettings::refresh() {
  unsigned index = 0;
  for(auto &hotkey : inputManager->hotkeyMap) {
    string mapping = hotkey->mapping;
    mapping.replace("KB0::", "");
    mapping.replace("MS0::", "Mouse::");
    mapping.replace(",", " and ");
    inputList.modify(index++, hotkey->name, mapping);
  }
  synchronize();
}

void HotkeySettings::eraseInput() {
  activeInput = inputManager->hotkeyMap[inputList.selection()];
  inputEvent(Scancode::None, 1);
}

void HotkeySettings::assignInput() {
  activeInput = inputManager->hotkeyMap[inputList.selection()];

  settings->setStatusText({"Set assignment for [", activeInput->name, "] ..."});
  settings->layout.setEnabled(false);
  setEnabled(false);
}

void HotkeySettings::inputEvent(unsigned scancode, int16_t value) {
  using nall::Mouse;

  if(activeInput == nullptr) return;
  if(value != 1) return;
  if(Mouse::isAnyButton(scancode) || Mouse::isAnyAxis(scancode)) return;
  if(Joypad::isAnyAxis(scancode)) return;
  if(activeInput->bind(scancode, value) == false) return;

  activeInput = nullptr;
  settings->setStatusText("");
  settings->layout.setEnabled(true);
  setEnabled(true);
  refresh();
}
