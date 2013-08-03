
Presentation *presentation = nullptr;

void Presentation::synchronize() {
  for(auto &emulator : emulatorList) emulator->menu.setVisible(false);
  for(auto &emulator : emulatorList) {
    if(emulator->interface == program->active) {
      active = emulator;
      emulator->menu.setVisible(true);
    }
  }

  shaderNone.setChecked();
  if(uiConfig->video.shader == "None") shaderNone.setChecked();
  if(uiConfig->video.shader == "Blur") shaderBlur.setChecked();
  for(auto &shader : shaderList) {
    string name = notdir(nall::basename(uiConfig->video.shader));
    if(auto position = name.find(".")) name[position()] = 0;
    if(name == shader->text()) shader->setChecked();
  }

  switch(uiConfig->video.scaleMode) {
  case 0: centerVideo.setChecked(); break;
  case 1: scaleVideo.setChecked(); break;
  case 2: stretchVideo.setChecked(); break;
  }
  synchronizeVideo.setChecked(uiConfig->video.synchronize);
  synchronizeAudio.setChecked(uiConfig->audio.synchronize);
  muteAudio.setChecked(uiConfig->audio.mute);

  resizeWindow.setVisible(program->active && uiConfig->video.scaleMode != 2);
  synchronizeTime.setVisible(program->active && system().rtc());
}

void Presentation::setSystemName(const string &name) {
  if(active) active->menu.setText(name);
}

Presentation::Presentation() : active(nullptr) {
  bootstrap();
  loadShaders();
  setGeometry({256, 256, 512, 768});
  windowManager->append(this, "Presentation");

  setTitle({::Emulator::Name, " ", ::Emulator::Version});
  setBackgroundColor({0, 0, 0});
  setMenuVisible();
  setStatusVisible();

  loadMenu.setText("Library");
    loadImport.setText("Import Game ...");
  settingsMenu.setText("Settings");
    videoMenu.setText("Video");
      centerVideo.setText("Center");
      scaleVideo.setText("Scale");
      stretchVideo.setText("Stretch");
      RadioItem::group(centerVideo, scaleVideo, stretchVideo);
    shaderMenu.setText("Shader");
      shaderNone.setText("None");
      shaderBlur.setText("Blur");
    synchronizeVideo.setText("Synchronize Video");
    synchronizeAudio.setText("Synchronize Audio");
    muteAudio.setText("Mute Audio");
    configurationSettings.setText("Configuration ...");
    resizeWindow.setText("Resize Window");
    synchronizeTime.setText("Synchronize Time");

  append(loadMenu);
    for(auto &item : loadListSystem) loadMenu.append(*item);
    loadMenu.append(loadSeparator, loadImport);
  for(auto &systemItem : emulatorList) append(systemItem->menu);
  append(settingsMenu);
    settingsMenu.append(videoMenu);
      videoMenu.append(centerVideo, scaleVideo, stretchVideo);
    settingsMenu.append(shaderMenu);
      shaderMenu.append(shaderNone, shaderBlur);
      if(shaderList.size() > 0) shaderMenu.append(*new Separator);
      for(auto &shader : shaderList) shaderMenu.append(*shader);
    settingsMenu.append(*new Separator);
    settingsMenu.append(synchronizeVideo, synchronizeAudio, muteAudio);
    settingsMenu.append(resizeWindow, synchronizeTime);
    if(Intrinsics::platform() != Intrinsics::Platform::OSX) {
      settingsMenu.append(*new Separator);
      settingsMenu.append(configurationSettings);
    }

  append(layout);
  layout.append(viewport, {0, 0, 1, 1});

  onSize = [&] {
    utility->resize();
  };

  onClose = [&] {
    setVisible(false);
    if(Intrinsics::platform() == Intrinsics::Platform::OSX) {
      utility->unload();
    } else {
      Application::quit();
    }
  };

  loadImport.onActivate = [&] {
    string path = browser->select("Import game");
    if(path) utility->importMedia(path);
  };

  shaderNone.onActivate = [&] { uiConfig->video.shader = "None"; utility->updateShader(); };
  shaderBlur.onActivate = [&] { uiConfig->video.shader = "Blur"; utility->updateShader(); };
  centerVideo.onActivate  = [&] { uiConfig->video.scaleMode = 0; utility->resize(); };
  scaleVideo.onActivate   = [&] { uiConfig->video.scaleMode = 1; utility->resize(); };
  stretchVideo.onActivate = [&] { uiConfig->video.scaleMode = 2; utility->resize(); };
  synchronizeVideo.onToggle = [&] { uiConfig->video.synchronize = synchronizeVideo.checked(); utility->synchronizeRuby(); };
  synchronizeAudio.onToggle = [&] { uiConfig->audio.synchronize = synchronizeAudio.checked(); utility->synchronizeRuby(); };
  muteAudio.onToggle = [&] { uiConfig->audio.mute = muteAudio.checked(); utility->synchronizeRuby(); };
  configurationSettings.onActivate = [&] { settings->setVisible(); settings->panelList.setFocused(); };
  resizeWindow.onActivate = [&] { utility->resize(true); };
  synchronizeTime.onActivate = [&] { system().rtcsync(); };

  synchronize();
}

void Presentation::bootstrap() {
  for(auto &emulator : program->emulator) {
    auto iEmulator = new Emulator;
    iEmulator->interface = emulator;

    for(auto &media : emulator->media) {
      if(media.bootable == false) continue;
      Item *item = new Item;
      item->onActivate = [=, &media] {
        utility->loadMedia(iEmulator->interface, media);
      };
      item->setText({media.name, " ..."});
      loadListSystem.append(item);
    }

    iEmulator->menu.setText(emulator->information.name);
    iEmulator->power.setText("Power");
    iEmulator->reset.setText("Reset");
    iEmulator->unload.setText("Unload");

    for(auto &port : emulator->port) {
      auto iPort = new Emulator::Port;
      iPort->menu.setText(port.name);
      iEmulator->port.append(iPort);

      for(auto &device : port.device) {
        auto iDevice = new RadioItem;
        iDevice->setText(device.name);
        iDevice->onActivate = [=] { utility->connect(port.id, device.id); };
        iPort->group.append(*iDevice);
        iPort->device.append(iDevice);
      }

      RadioItem::group(iPort->group);
    }

    iEmulator->menu.append(iEmulator->power);
    if(emulator->information.resettable)
    iEmulator->menu.append(iEmulator->reset);
    iEmulator->menu.append(*new Separator);
    unsigned visiblePorts = 0;
    for(auto &iPort : iEmulator->port) {
      iEmulator->menu.append(iPort->menu);
      if(iPort->device.size() <= 1) iPort->menu.setVisible(false);
      else visiblePorts++;
      for(auto &iDevice : iPort->device) {
        iPort->menu.append(*iDevice);
      }
    }
    iEmulator->menu.append(iEmulator->controllerSeparator);
    if(visiblePorts == 0) iEmulator->controllerSeparator.setVisible(false);
    iEmulator->menu.append(iEmulator->unload);

    iEmulator->power.onActivate = {&Utility::power, utility};
    iEmulator->reset.onActivate = {&Utility::reset, utility};
    iEmulator->unload.onActivate = {&Utility::unload, utility};

    emulatorList.append(iEmulator);
  }
}

void Presentation::loadShaders() {
  string pathname = program->loadPath("Video Shaders/");
  lstring files = directory::files(pathname);
  for(auto &filename : files) {
    lstring name = string{filename}.split(".");
    //only add shaders that work with current video driver
    if(name(1) != uiConfig->video.driver) continue;

    auto shader = new RadioItem;
    shader->setText(name(0));
    shader->onActivate = [=] {
      uiConfig->video.shader = {pathname, filename};
      utility->updateShader();
    };
    shaderList.append(shader);
  }

  nall::group<RadioItem> group;
  group.append(shaderNone);
  group.append(shaderBlur);
  for(auto &shader : shaderList) group.append(*shader);
  RadioItem::group(group);
}
