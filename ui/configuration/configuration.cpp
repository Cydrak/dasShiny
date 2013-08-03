#include "../ui.hpp"
UIConfiguration *uiConfig = nullptr;

UIConfiguration::UIConfiguration() {
  video.append(video.driver = ruby::video.optimalDriver(), "Driver");
  video.append(video.synchronize = false, "Synchronize");
  video.append(video.shader = "Blur", "Shader");
  video.append(video.scaleMode = 0, "ScaleMode");
  video.append(video.saturation = 100, "Saturation");
  video.append(video.gamma = 150, "Gamma");
  video.append(video.luminance = 100, "Luminance");
  video.append(video.startFullScreen = false, "StartFullScreen");
  append(video, "Video");
  
  audio.append(audio.driver = ruby::audio.optimalDriver(), "Driver");
  audio.append(audio.synchronize = true, "Synchronize");
  audio.append(audio.frequency = 48000, "Frequency");
  audio.append(audio.latency = 60, "Latency");
  audio.append(audio.resampler = 2, "Resampler");
  audio.append(audio.volume = 100, "Volume");
  audio.append(audio.mute = false, "Mute");
  append(audio, "Audio");
  
  input.append(input.driver = ruby::input.optimalDriver(), "Driver");
  input.append(input.focusPause = false, "Focus::Pause");
  input.append(input.focusAllow = false, "Focus::AllowInput");
  append(input, "Input");
  
  timing.append(timing.video = 60.0, "Video");
  timing.append(timing.audio = 48000.0, "Audio");
  append(timing, "Timing");
  
  load();
}

void UIConfiguration::load() {
  inherited::load(program->loadPath("settings.bml"));
  save();  //creates file if it does not exist
}

void UIConfiguration::save() {
  inherited::save(program->savePath("settings.bml"));
}
