struct Presentation : Window {
  FixedLayout layout;
  Viewport viewport;

  struct Emulator {
    ::Emulator::Interface *interface;

    Menu menu;
      Item power;
      Item reset;
      Item unload;
      Separator controllerSeparator;
      struct Port {
        Menu menu;
        set<RadioItem&> group;
        vector<RadioItem*> device;
      };
      vector<Port*> port;
    function<void (string)> callback;
  };
  vector<Emulator*> emulatorList;

  Menu loadMenu;
    vector<Item*> loadListSystem;
    Separator loadSeparator;
    Item loadImport;
  Menu settingsMenu;
    Menu videoMenu;
      RadioItem centerVideo;
      RadioItem scaleVideo;
      RadioItem stretchVideo;
    Menu shaderMenu;
      RadioItem shaderNone;
      RadioItem shaderBlur;
      vector<RadioItem*> shaderList;
    CheckItem synchronizeVideo;
    CheckItem synchronizeAudio;
    CheckItem muteAudio;
    Item resizeWindow;
    Item synchronizeTime;
    Item configurationSettings;

  void synchronize();
  void setSystemName(const string &name);
  void loadShaders();
  void bootstrap();
  Presentation();

  Emulator *active;
};

extern Presentation *presentation;
