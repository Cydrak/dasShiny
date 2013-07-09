struct Configuration : configuration {
  struct Video {
    string driver;
    bool synchronize;
    string shader;
    unsigned scaleMode;
    unsigned saturation;
    unsigned gamma;
    unsigned luminance;
    bool startFullScreen;
  } video;

  struct Audio {
    string driver;
    bool synchronize;
    unsigned frequency;
    unsigned latency;
    unsigned resampler;
    unsigned volume;
    bool mute;
  } audio;

  struct Input {
    string driver;
    bool focusPause;
    bool focusAllow;
  } input;

  struct Timing {
    double video;
    double audio;
  } timing;

  void load();
  void save();
  Configuration();
};

extern Configuration *config;
