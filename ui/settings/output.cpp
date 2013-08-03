OutputSettings *outputSettings = nullptr;

OutputSlider::OutputSlider() {
  append(name, {75, 0});
  append(value, {75, 0});
  append(slider, {~0, 0});
}

OutputSettings::OutputSettings() {
  videoTitle.setFont(program->titleFont);
  videoTitle.setText("Video");
  saturation.name.setText("Saturation:");
  saturation.slider.setLength(201);
  gamma.name.setText("Gamma:");
  gamma.slider.setLength(101);
  luminance.name.setText("Luminance:");
  luminance.slider.setLength(101);

  audioTitle.setFont(program->titleFont);
  audioTitle.setText("Audio");
  frequencyLabel.setText("Frequency:");
  frequency.append("32000hz", "44100hz", "48000hz", "96000hz");
  latencyLabel.setText("Latency:");
  latency.append("20ms", "40ms", "60ms", "80ms", "100ms");
  resamplerLabel.setText("Resampler:");
  resampler.append("Linear", "Hermite", "Sinc");
  volume.name.setText("Volume:");
  volume.slider.setLength(201);

  append(videoTitle, {~0, 0}, 5);
  append(saturation, {~0, 0});
  append(gamma, {~0, 0});
  append(luminance, {~0, 0}, 5);

  append(audioTitle, {~0, 0}, 5);
  append(audioLayout, {~0, 0}, 5);
    audioLayout.append(frequencyLabel, {0, 0}, 5);
    audioLayout.append(frequency, {~0, 0}, 5);
    audioLayout.append(latencyLabel, {0, 0}, 5);
    audioLayout.append(latency, {~0, 0}, 5);
    audioLayout.append(resamplerLabel, {0, 0}, 5);
    audioLayout.append(resampler, {~0, 0});
  append(volume, {~0, 0});

  saturation.slider.setPosition(uiConfig->video.saturation);
  gamma.slider.setPosition(uiConfig->video.gamma - 100);
  luminance.slider.setPosition(uiConfig->video.luminance);

  switch(uiConfig->audio.frequency) { default:
  case 32000: frequency.setSelection(0); break;
  case 44100: frequency.setSelection(1); break;
  case 48000: frequency.setSelection(2); break;
  case 96000: frequency.setSelection(3); break;
  }
  switch(uiConfig->audio.latency) { default:
  case  20: latency.setSelection(0); break;
  case  40: latency.setSelection(1); break;
  case  60: latency.setSelection(2); break;
  case  80: latency.setSelection(3); break;
  case 100: latency.setSelection(4); break;
  }
  resampler.setSelection(uiConfig->audio.resampler);
  volume.slider.setPosition(uiConfig->audio.volume);

  synchronize();

  saturation.slider.onChange = gamma.slider.onChange = luminance.slider.onChange =
  frequency.onChange = latency.onChange = resampler.onChange = volume.slider.onChange =
  {&OutputSettings::synchronize, this};
}

void OutputSettings::synchronize() {
  uiConfig->video.saturation = saturation.slider.position();
  uiConfig->video.gamma = 100 + gamma.slider.position();
  uiConfig->video.luminance = luminance.slider.position();

  saturation.value.setText({uiConfig->video.saturation, "%"});
  gamma.value.setText({uiConfig->video.gamma, "%"});
  luminance.value.setText({uiConfig->video.luminance, "%"});

  if(program->active) system().paletteUpdate();

  switch(frequency.selection()) {
  case 0: uiConfig->audio.frequency = 32000; break;
  case 1: uiConfig->audio.frequency = 44100; break;
  case 2: uiConfig->audio.frequency = 48000; break;
  case 3: uiConfig->audio.frequency = 96000; break;
  }
  switch(latency.selection()) {
  case 0: uiConfig->audio.latency =  20; break;
  case 1: uiConfig->audio.latency =  40; break;
  case 2: uiConfig->audio.latency =  60; break;
  case 3: uiConfig->audio.latency =  80; break;
  case 4: uiConfig->audio.latency = 100; break;
  }
  uiConfig->audio.resampler = resampler.selection();
  uiConfig->audio.volume = volume.slider.position();

  volume.value.setText({uiConfig->audio.volume, "%"});

  utility->synchronizeRuby();
}
