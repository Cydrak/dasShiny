void InputManager::appendHotkeys() {
  {
    auto hotkey = new HotkeyInput;
    hotkey->name    = "Toggle Fullscreen Mode";
    hotkey->mapping = "KB0::F11";

    hotkey->press = [] {
      utility->toggleFullScreen();
    };
  }

  {
    auto hotkey = new HotkeyInput;
    hotkey->name    = "Toggle Mouse Capture";
    hotkey->mapping = "KB0::F12";

    hotkey->press = [] {
      input.acquired() ? input.unacquire() : input.acquire();
    };
  }

  {
    auto hotkey = new HotkeyInput;
    hotkey->name = "Pause Emulation";
    hotkey->mapping = "KB0::P";

    hotkey->press = [] {
      program->pause = !program->pause;
    };
  }

  {
    auto hotkey = new HotkeyInput;
    hotkey->name    = "Fast Forward";
    hotkey->mapping = "KB0::Tilde";

    hotkey->press = [] {
      video.set(Video::Synchronize, false);
      audio.set(Audio::Synchronize, false);
    };

    hotkey->release = [] {
      video.set(Video::Synchronize, uiConfig->video.synchronize);
      audio.set(Audio::Synchronize, uiConfig->audio.synchronize);
    };
  }

  {
    auto hotkey = new HotkeyInput;
    hotkey->name    = "Power Cycle";
    hotkey->mapping = "None";

    hotkey->press = [] {
      utility->power();
    };
  }

  {
    auto hotkey = new HotkeyInput;
    hotkey->name    = "Soft Reset";
    hotkey->mapping = "None";

    hotkey->press = [] {
      utility->reset();
    };
  }

  {
    auto hotkey = new HotkeyInput;
    hotkey->name    = "Close Emulator";
    hotkey->mapping = "None";

    hotkey->press = [] {
      Application::quit();
    };
  }

  {
    auto hotkey = new HotkeyInput;
    hotkey->name    = "Toggle Tracer";
    hotkey->mapping = "None";

    hotkey->press = [&] {
      utility->tracerToggle();
    };
  }

  {
    auto hotkey = new HotkeyInput;
    hotkey->name    = "Export Memory";
    hotkey->mapping = "None";

    hotkey->press = [&] {
      if(program->active == nullptr) return;
      system().exportMemory();
      utility->showMessage("Memory exported");
    };
  }

  Configuration::Node node;
  for(auto& hotkey : hotkeyMap)
    node.append(hotkey->mapping, string{hotkey->name}.replace(" ", ""));
  
  config.append(node, "Hotkey");
}

void InputManager::pollHotkeys() {
  for(auto &hotkey : hotkeyMap) {
    bool state = hotkey->poll();
    if(hotkey->state == 0 && state == 1) if(hotkey->press) hotkey->press();
    if(hotkey->state == 1 && state == 0) if(hotkey->release) hotkey->release();
    hotkey->state = state;
  }
}
