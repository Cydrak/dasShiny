struct TimingAdjustment : HorizontalLayout {
  Label name;
  LineEdit value;
  Button assign;
  Widget spacer;
  Button analyze;
  Button stop;

  TimingAdjustment();
};

struct AdvancedSettings : SettingsLayout {
  Label timingTitle;
  TimingAdjustment videoAdjust;
  TimingAdjustment audioAdjust;

  Label driverTitle;
  HorizontalLayout driverLayout;
    Label videoLabel;
    ComboButton videoDriver;
    Label audioLabel;
    ComboButton audioDriver;
    Label inputLabel;
    ComboButton inputDriver;

  Label libraryTitle;
  HorizontalLayout libraryLayout;
    Label libraryLabel;
    LineEdit libraryPath;
    Button libraryBrowse;

  Widget spacer;
  Label infoLabel;

  AdvancedSettings();
  
  void analyzeVideoFrequency();
  void analyzeAudioFrequency();

  void analyzeStart();
  bool analyzeTick(const string &type);
  void analyzeStop();

  struct Analysis {
    bool stop;
    unsigned seconds;
    unsigned counter;
    vector<unsigned> sample;
    time_t systemTime;
  } analysis;
};

extern AdvancedSettings *advancedSettings;
