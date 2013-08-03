struct UIConfiguration : Configuration::Document {
  typedef Configuration::Document inherited;
  
  struct Video : Configuration::Node {
    string driver;
    bool synchronize;
    string shader;
    unsigned scaleMode;
    unsigned saturation;
    unsigned gamma;
    unsigned luminance;
    bool startFullScreen;
  } video;

  struct Audio : Configuration::Node {
    string driver;
    bool synchronize;
    unsigned frequency;
    unsigned latency;
    unsigned resampler;
    unsigned volume;
    bool mute;
  } audio;

  struct Input : Configuration::Node {
    string driver;
    bool focusPause;
    bool focusAllow;
  } input;

  struct Timing : Configuration::Node {
    double video;
    double audio;
  } timing;

  void load();
  void save();
  UIConfiguration();
};

extern UIConfiguration *uiConfig;
