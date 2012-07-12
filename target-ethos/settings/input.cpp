InputSettings *inputSettings = nullptr;

InputSettings::InputSettings() : activeInput(nullptr) {
  title.setFont(application->titleFont);
  title.setText("Input Settings");
  focusLabel.setText("When Focus is Lost:");
  focusPause.setText("Pause Emulation");
  focusAllow.setText("Allow Input");
  inputList.setHeaderText("Name", "Mapping");
  inputList.setHeaderVisible();
  resetButton.setText("Reset");
  eraseButton.setText("Erase");

  append(title, {~0, 0}, 5);
  append(focusLayout, {~0, 0}, 5);
    focusLayout.append(focusLabel, {0, 0}, 5);
    focusLayout.append(focusPause, {0, 0}, 5);
    focusLayout.append(focusAllow, {0, 0});
  append(selectionLayout, {~0, 0}, 5);
    selectionLayout.append(systemList, {~0, 0}, 5);
    selectionLayout.append(portList, {~0, 0}, 5);
    selectionLayout.append(deviceList, {~0, 0});
  append(inputList, {~0, ~0}, 5);
  append(controlLayout, {~0, 0});
    controlLayout.append(assign[0], {100, 0}, 5);
    controlLayout.append(assign[1], {100, 0}, 5);
    controlLayout.append(assign[2], {100, 0}, 5);
    controlLayout.append(assign[3], {100, 0}, 5);
    controlLayout.append(assign[4], {100, 0}, 5);
    controlLayout.append(spacer, {~0, 0});
    controlLayout.append(resetButton, {80, 0}, 5);
    controlLayout.append(eraseButton, {80, 0});

  for(auto &emulator : application->emulator) {
    systemList.append(emulator->information.name);
  }

  focusPause.setChecked(config->input.focusPause);
  focusAllow.setChecked(config->input.focusAllow);
  focusAllow.setEnabled(!config->input.focusPause);

  focusPause.onToggle = [&] { config->input.focusPause = focusPause.checked(); focusAllow.setEnabled(!focusPause.checked()); };
  focusAllow.onToggle = [&] { config->input.focusAllow = focusAllow.checked(); };
  systemList.onChange = {&InputSettings::systemChanged, this};
  portList.onChange = {&InputSettings::portChanged, this};
  deviceList.onChange = {&InputSettings::deviceChanged, this};
  inputList.onChange = {&InputSettings::synchronize, this};
  inputList.onActivate = {&InputSettings::assignInput, this};
  assign[0].onActivate = [&] { assignMouseInput(0); };
  assign[1].onActivate = [&] { assignMouseInput(1); };
  assign[2].onActivate = [&] { assignMouseInput(2); };
  assign[3].onActivate = [&] { assignMouseInput(3); };
  assign[4].onActivate = [&] { assignMouseInput(4); };
  resetButton.onActivate = {&InputSettings::resetInput, this};
  eraseButton.onActivate = {&InputSettings::eraseInput, this};

  systemChanged();
}

void InputSettings::synchronize() {
  if(inputList.selected() == false) {
    assign[0].setVisible(false);
    assign[1].setVisible(false);
    assign[2].setVisible(false);
    assign[3].setVisible(false);
    assign[4].setVisible(false);
  } else {
    unsigned number = activeDevice().order[inputList.selection()];
    auto &input = activeDevice().input[number];
    auto selectedInput = inputManager->inputMap[input.guid];

    if(dynamic_cast<DigitalInput*>(selectedInput)) {
      assign[0].setText("Mouse Left");
      assign[1].setText("Mouse Middle");
      assign[2].setText("Mouse Right");
      assign[0].setVisible(true);
      assign[1].setVisible(true);
      assign[2].setVisible(true);
      assign[3].setVisible(false);
      assign[4].setVisible(false);
    }

    if(dynamic_cast<RelativeInput*>(selectedInput)) {
      assign[0].setText("Mouse X");
      assign[1].setText("Mouse Y");
      assign[0].setVisible(true);
      assign[1].setVisible(true);
      assign[2].setVisible(false);
    }

    if(dynamic_cast<AbsoluteInput*>(selectedInput)) {
      assign[0].setText("Pointer X");
      assign[1].setText("Pointer Y");
      assign[2].setText("Left");
      assign[3].setText("Middle");
      assign[4].setText("Right");
      assign[0].setVisible(true);
      assign[1].setVisible(true);
      assign[2].setVisible(true);
      assign[3].setVisible(true);
      assign[4].setVisible(true);
    }
  }

  eraseButton.setEnabled(inputList.selected());
}

Emulator::Interface& InputSettings::activeSystem() {
  return *application->emulator[systemList.selection()];
}

Emulator::Interface::Port& InputSettings::activePort() {
  return activeSystem().port[portList.selection()];
}

Emulator::Interface::Device& InputSettings::activeDevice() {
  return activePort().device[deviceList.selection()];
}

void InputSettings::systemChanged() {
  portList.reset();
  for(auto &port : activeSystem().port) {
    portList.append(port.name);
  }
  portChanged();
}

void InputSettings::portChanged() {
  deviceList.reset();
  for(auto &device : activePort().device) {
    deviceList.append(device.name);
  }
  deviceChanged();
}

void InputSettings::deviceChanged() {
  inputList.reset();
  for(unsigned number : activeDevice().order) inputList.append("", "");
  inputChanged();
  synchronize();
}

void InputSettings::inputChanged() {
  unsigned index = 0;
  for(unsigned number : activeDevice().order) {
    auto &input = activeDevice().input[number];
    auto abstract = inputManager->inputMap(input.guid);
    string mapping = abstract->mapping;
    mapping.replace("KB0::", "");
    mapping.replace("MS0::", "Mouse::");
    if(dynamic_cast<AbsoluteInput*>(inputManager->inputMap[input.guid]))
      mapping.replace("Mouse::", "Pointer::");

    mapping.replace(",", " or ");
    inputList.modify(index++, input.name, mapping);
  }
}

void InputSettings::resetInput() {
  if(MessageWindow::question(*settings, "All inputs will be erased. Are you sure you want to do this?")
  == MessageWindow::Response::No) return;

  auto &device = activeDevice();
  unsigned length = device.input.size();
  for(unsigned n = 0; n < length; n++) {
    activeInput = inputManager->inputMap[device.input[n].guid];
    inputEvent(Scancode::None, 1);
  }
}

void InputSettings::eraseInput() {
  unsigned number = activeDevice().order[inputList.selection()];
  auto &input = activeDevice().input[number];
  activeInput = inputManager->inputMap[input.guid];
  inputEvent(Scancode::None, 1);
}

void InputSettings::assignInput() {
  unsigned number = activeDevice().order[inputList.selection()];
  auto &input = activeDevice().input[number];
  activeInput = inputManager->inputMap[input.guid];

  settings->setStatusText({"Set assignment for [", activeDevice().name, "::", input.name, "] ..."});
  settings->layout.setEnabled(false);
  setEnabled(false);
}

void InputSettings::assignMouseInput(unsigned n) {
  unsigned number = activeDevice().order[inputList.selection()];
  auto &input = activeDevice().input[number];
  activeInput = inputManager->inputMap[input.guid];

  if(dynamic_cast<DigitalInput*>(activeInput)) {
    return inputEvent(mouse(0).button(n), 1, true);
  }

  if(dynamic_cast<RelativeInput*>(activeInput)) {
    return inputEvent(mouse(0).axis(n), 1, true);
  }

  if(dynamic_cast<AbsoluteInput*>(activeInput)) {
    if(n < 2) return inputEvent(mouse(0).axis(n), 1, true);
    else      return inputEvent(mouse(0).button(n-2), 1, true);
  }
}

void InputSettings::inputEvent(unsigned scancode, int16_t value, bool allowMouseInput) {
  using nall::Mouse;
  if(activeInput == nullptr) return;
  if(allowMouseInput == false && (Mouse::isAnyButton(scancode) || Mouse::isAnyAxis(scancode))) return;
  if(activeInput->bind(scancode, value) == false) return;

  activeInput = nullptr;
  inputChanged();
  settings->setStatusText("");
  settings->layout.setEnabled(true);
  setEnabled(true);
  synchronize();
}
