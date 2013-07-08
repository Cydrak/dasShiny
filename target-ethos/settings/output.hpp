struct OutputSlider : HorizontalLayout {
  Label name;
  Label value;
  HorizontalSlider slider;

  OutputSlider();
};

struct OutputSettings : SettingsLayout {
  Label videoTitle;
  Label colorAdjustment;
  OutputSlider saturation;
  OutputSlider gamma;
  OutputSlider luminance;

  Label audioTitle;
  HorizontalLayout audioLayout;
    Label frequencyLabel;
    ComboButton frequency;
    Label latencyLabel;
    ComboButton latency;
    Label resamplerLabel;
    ComboButton resampler;
  OutputSlider volume;

  void synchronize();
  OutputSettings();
};

extern OutputSettings *outputSettings;
