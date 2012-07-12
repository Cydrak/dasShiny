struct TimingAdjustment : HorizontalLayout {
  Label name;
  LineEdit value;
  Button assign;
  Widget spacer;
  Button analyze;
  Button stop;

  TimingAdjustment();
};

struct TimingSettings : SettingsLayout {
  Label title;
  TimingAdjustment videoAdjust;
  TimingAdjustment audioAdjust;

  void analyzeVideoFrequency();
  void analyzeAudioFrequency();

  void analyzeStart();
  bool analyzeTick(const string &type);
  void analyzeStop();

  TimingSettings();

  struct Analysis {
    bool stop;
    unsigned seconds;
    unsigned counter;
    vector<unsigned> sample;
    time_t systemTime;
  } analysis;
};

extern TimingSettings *timingSettings;
