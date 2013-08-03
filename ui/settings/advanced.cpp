AdvancedSettings *advancedSettings = nullptr;

TimingAdjustment::TimingAdjustment() {
  assign.setEnabled(false);
  assign.setText("Assign");
  analyze.setText("Analyze");
  stop.setEnabled(false);
  stop.setText("Stop");

  append(name, {40, 0});
  append(value, {100, 0}, 5);
  append(assign, {80, 0}, 5);
  append(spacer, {~0, 0});
  append(analyze, {80, 0}, 5);
  append(stop, {80, 0});
}

AdvancedSettings::AdvancedSettings() {
  timingTitle.setFont(program->titleFont);
  timingTitle.setText("Synchronization");
  videoAdjust.name.setText("Video:");
  videoAdjust.value.setText({uiConfig->timing.video});
  audioAdjust.name.setText("Audio:");
  audioAdjust.value.setText({uiConfig->timing.audio});

  driverTitle.setFont(program->titleFont);
  driverTitle.setText("Drivers");
  videoLabel.setText("Video:");
  audioLabel.setText("Audio:");
  inputLabel.setText("Input:");
  
  libraryTitle.setFont(program->titleFont);
  libraryTitle.setText("Game Library");
  libraryLabel.setText("Path:");
  libraryPath.setEditable(false);
  string path = string::read({configpath(), "dasShiny/library.cfg"}).strip().transform("\\", "/");
  if(path.empty()) path = {userpath(), "Emulation/Nintendo DS"};
  if(path.endswith("/") == false) path.append("/");
  libraryPath.setText(path);
  libraryBrowse.setText("Browse ...");
  
  infoLabel.setFont(program->boldFont);
  infoLabel.setText({
    Emulator::Name, " ", Emulator::Version, "\n",
    "  Author: ", Emulator::Author, "\n",
    "  License: ", Emulator::License, "\n",
    "  Website: ", Emulator::Website
  });

  lstring list;

  list.split(";", video.availableDrivers());
  for(unsigned n = 0; n < list.size(); n++) {
    videoDriver.append(list[n]);
    if(list[n] == uiConfig->video.driver) videoDriver.setSelection(n);
  }

  list.split(";", audio.availableDrivers());
  for(unsigned n = 0; n < list.size(); n++) {
    audioDriver.append(list[n]);
    if(list[n] == uiConfig->audio.driver) audioDriver.setSelection(n);
  }

  list.split(";", input.availableDrivers());
  for(unsigned n = 0; n < list.size(); n++) {
    inputDriver.append(list[n]);
    if(list[n] == uiConfig->input.driver) inputDriver.setSelection(n);
  }

  append(timingTitle, {~0, 0}, 5);
  append(videoAdjust, {~0, 0}, 5);
  append(audioAdjust, {~0, 0}, 5);

  append(driverTitle, {~0, 0}, 5);
  append(driverLayout, {~0, 0}, 15);
    driverLayout.append(videoLabel, {0, 0}, 5);
    driverLayout.append(videoDriver, {~0, 0}, 5);
    driverLayout.append(audioLabel, {0, 0}, 5);
    driverLayout.append(audioDriver, {~0, 0}, 5);
    driverLayout.append(inputLabel, {0, 0}, 5);
    driverLayout.append(inputDriver, {~0, 0});

  append(libraryTitle, {~0, 0}, 5);
  append(libraryLayout, {~0, 0}, 15);
    libraryLayout.append(libraryLabel, {0, 0}, 5);
    libraryLayout.append(libraryPath, {~0, 0}, 5);
    libraryLayout.append(libraryBrowse, {80, 0});
  if(Intrinsics::platform() != Intrinsics::Platform::OSX) {
    append(spacer, {~0, ~0});
    append(infoLabel, {~0, 0});
  }

  videoAdjust.value.onChange = [&] { videoAdjust.assign.setEnabled(true); };
  audioAdjust.value.onChange = [&] { audioAdjust.assign.setEnabled(true); };
  videoAdjust.assign.onActivate = [&] {
    uiConfig->timing.video = atof(videoAdjust.value.text());
    videoAdjust.value.setText({uiConfig->timing.video});
    videoAdjust.assign.setEnabled(false);
    utility->synchronizeDSP();
  };
  audioAdjust.assign.onActivate = [&] {
    uiConfig->timing.audio = atof(audioAdjust.value.text());
    audioAdjust.value.setText({uiConfig->timing.audio});
    audioAdjust.assign.setEnabled(false);
    utility->synchronizeDSP();
  };
  videoAdjust.analyze.onActivate = {&AdvancedSettings::analyzeVideoFrequency, this};
  audioAdjust.analyze.onActivate = {&AdvancedSettings::analyzeAudioFrequency, this};
  videoAdjust.stop.onActivate = audioAdjust.stop.onActivate = [&] { analysis.stop = true; };

  videoDriver.onChange = [&] { uiConfig->video.driver = videoDriver.text(); };
  audioDriver.onChange = [&] { uiConfig->audio.driver = audioDriver.text(); };
  inputDriver.onChange = [&] { uiConfig->input.driver = inputDriver.text(); };

  libraryBrowse.onActivate = [&] {
    string path = BrowserWindow().setParent(*settings).setPath(userpath()).directory();
    if(path.empty()) return;
    file::write({configpath(), "dasShiny/library.cfg"}, path);
    libraryPath.setText(path);
  };
}

void AdvancedSettings::analyzeVideoFrequency() {
  video.set(Video::Synchronize, true);
  audio.set(Audio::Synchronize, false);
  videoAdjust.stop.setEnabled(true);
  analyzeStart();
  do {
    uint32_t *output;
    unsigned pitch;
    if(video.lock(output, pitch, 16, 16)) {
      pitch >>= 2;
      for(unsigned y = 0; y < 16; y++) memset(output + y * pitch, 0, 4 * 16);
      video.unlock();
      video.refresh();
    }
  } while(analyzeTick("Video"));
  analyzeStop();
}

void AdvancedSettings::analyzeAudioFrequency() {
  video.set(Video::Synchronize, false);
  audio.set(Audio::Synchronize, true);
  audioAdjust.stop.setEnabled(true);
  analyzeStart();
  do {
    audio.sample(0, 0);
  } while(analyzeTick("Audio"));
  analyzeStop();
}

void AdvancedSettings::analyzeStart() {
  audio.clear();

  settings->panelList.setEnabled(false);
  videoAdjust.analyze.setEnabled(false);
  audioAdjust.analyze.setEnabled(false);
  settings->setStatusText("Initializing ...");
  Application::processEvents();

  analysis.stop = false;
  analysis.seconds = 0;
  analysis.counter = 0;
  analysis.sample.reset();
  analysis.systemTime = time(0);
}

bool AdvancedSettings::analyzeTick(const string &type) {
  analysis.counter++;

  time_t systemTime = time(0);
  if(systemTime > analysis.systemTime) {
    analysis.systemTime = systemTime;
    Application::processEvents();

    if(analysis.seconds < 3) {
      analysis.seconds++;
    } else {
      analysis.sample.append(analysis.counter);
      uintmax_t sum = 0;
      for(auto &point : analysis.sample) sum += point;
      settings->setStatusText({
        type, " sample rate: ", (double)sum / analysis.sample.size(), "hz",
        " (", analysis.sample.size(), " sample points)"
      });
    }

    analysis.counter = 0;
  }

  return !analysis.stop;
}

void AdvancedSettings::analyzeStop() {
  video.set(Video::Synchronize, uiConfig->video.synchronize);
  audio.set(Audio::Synchronize, uiConfig->audio.synchronize);

  settings->panelList.setEnabled(true);
  videoAdjust.analyze.setEnabled(true);
  audioAdjust.analyze.setEnabled(true);
  videoAdjust.stop.setEnabled(false);
  audioAdjust.stop.setEnabled(false);
}
