#include "../ethos.hpp"

Utility *utility = nullptr;

void Utility::setInterface(Emulator::Interface *emulator) {
  application->active = emulator;
  presentation->synchronize();
}

//load menu option selected
void Utility::loadMedia(Emulator::Interface *emulator, Emulator::Interface::Media &media) {
  string pathname;
  if(!media.load.empty()) pathname = application->path({media.load, "/"});
  if(!directory::exists(pathname)) pathname = browser->select("Load Media", media.type);
  if(!directory::exists(pathname)) return;
  if(!file::exists({pathname, "manifest.xml"})) return;
  loadMedia(emulator, media, pathname);
}

//load menu cartridge selected or command-line load
void Utility::loadMedia(Emulator::Interface *emulator, Emulator::Interface::Media &media, const string &pathname) {
  unload();
  setInterface(emulator);
  path(0) = application->path({media.name, ".sys/"});
  path(media.id) = pathname;
  if(media.load.empty()) this->pathname.append(pathname);

  string manifest;
  manifest.readfile({pathname, "manifest.xml"});
  system().load(media.id, manifest);
  system().power();

  if(this->pathname.size() == 0) this->pathname.append(pathname);
  presentation->setSystemName(media.name);
  load();
}

//request from emulation core to load non-volatile media folder
void Utility::loadRequest(unsigned id, const string &name, const string &type) {
  string pathname = browser->select({"Load ", name}, type);
  if(pathname.empty()) return;
  path(id) = pathname;
  this->pathname.append(pathname);

  string manifest;
  manifest.readfile({pathname, "manifest.xml"});
  system().load(id, manifest);
}

//request from emulation core to load non-volatile media file
void Utility::loadRequest(unsigned id, const string &path) {
  string pathname = {this->path(system().group(id)), path};
  if(file::exists(pathname) == false) return;
  mmapstream stream(pathname);
  return system().load(id, stream);
}

//request from emulation core to save non-volatile media file
void Utility::saveRequest(unsigned id, const string &path) {
  string pathname = {this->path(system().group(id)), path};
  filestream stream(pathname, file::mode::write);
  return system().save(id, stream);
}

void Utility::connect(unsigned port, unsigned device) {
  if(application->active == nullptr) return;
  system().connect(port, device);
}

void Utility::power() {
  if(application->active == nullptr) return;
  system().power();
}

void Utility::reset() {
  if(application->active == nullptr) return;
  system().reset();
}

void Utility::load() {
  string title;
  for(auto &path : pathname) {
    string name = path;
    name.rtrim<1>("/");
    title.append(notdir(nall::basename(name)), " + ");
  }
  title.rtrim<1>(" + ");
  presentation->setTitle(title);

  cheatEditor->load({pathname[0], "cheats.xml"});
  stateManager->load({pathname[0], "dasShiny/states.bsa"}, 1);

  system().paletteUpdate();
  synchronizeDSP();

  resize();
  cheatEditor->synchronize();
  cheatEditor->refresh();
}

void Utility::unload() {
  if(application->active == nullptr) return;
  if(tracerEnable) tracerToggle();

  cheatEditor->save({pathname[0], "cheats.xml"});
  stateManager->save({pathname[0], "dasShiny/states.bsa"}, 1);

  system().unload();
  path.reset();
  pathname.reset();
  cheatEditor->reset();
  stateManager->reset();
  setInterface(nullptr);

  video.clear();
  audio.clear();
  presentation->setTitle({Emulator::Name, " v", Emulator::Version});
  cheatDatabase->setVisible(false);
  cheatEditor->setVisible(false);
  stateManager->setVisible(false);
}

void Utility::saveState(unsigned slot) {
  if(application->active == nullptr) return;
  serializer s = system().serialize();
  if(s.size() == 0) return;
  directory::create({pathname[0], "dasShiny/"});
  if(file::write({pathname[0], "dasShiny/state-", slot, ".bsa"}, s.data(), s.size()) == false);
  showMessage({"Save to slot ", slot});
}

void Utility::loadState(unsigned slot) {
  if(application->active == nullptr) return;
  auto memory = file::read({pathname[0], "dasShiny/state-", slot, ".bsa"});
  if(memory.size() == 0) return showMessage({"Unable to locate slot ", slot, " state"});
  serializer s(memory.data(), memory.size());
  if(system().unserialize(s) == false) return showMessage({"Slot ", slot, " state incompatible"});
  showMessage({"Loaded from slot ", slot});
}

void Utility::tracerToggle() {
  if(application->active == nullptr) return;
  tracerEnable = !tracerEnable;
  bool result = system().tracerEnable(tracerEnable);
  if( tracerEnable &&  result) return utility->showMessage("Tracer activated");
  if( tracerEnable && !result) return tracerEnable = false, utility->showMessage("Unable to activate tracer");
  if(!tracerEnable &&  result) return utility->showMessage("Tracer deactivated");
  if(!tracerEnable && !result) return utility->showMessage("Unable to deactivate tracer");
}

void Utility::synchronizeDSP() {
  if(application->active == nullptr) return;

  if(config->video.synchronize == false) {
    return dspaudio.setFrequency(system().audioFrequency());
  }

  double inputRatio = system().audioFrequency() / system().videoFrequency();
  double outputRatio = config->timing.audio / config->timing.video;
  double frequency = inputRatio / outputRatio * config->audio.frequency;

  dspaudio.setFrequency(frequency);
}

void Utility::synchronizeRuby() {
  video.set(Video::Synchronize, config->video.synchronize);
  audio.set(Audio::Synchronize, config->audio.synchronize);
  audio.set(Audio::Frequency, config->audio.frequency);
  audio.set(Audio::Latency, config->audio.latency);

  switch(config->audio.resampler) {
  case 0: dspaudio.setResampler(DSP::ResampleEngine::Linear);  break;
  case 1: dspaudio.setResampler(DSP::ResampleEngine::Hermite); break;
  case 2: dspaudio.setResampler(DSP::ResampleEngine::Sinc);    break;
  }
  dspaudio.setResamplerFrequency(config->audio.frequency);
  dspaudio.setVolume(config->audio.mute ? 0.0 : config->audio.volume * 0.01);
  synchronizeDSP();
}

void Utility::updateShader() {
  if(config->video.shader == "None") {
    video.set(Video::Shader, (const char*)"");
    video.set(Video::Filter, 0u);
    return;
  }
  if(config->video.shader == "Blur") {
    video.set(Video::Shader, (const char*)"");
    video.set(Video::Filter, 1u);
    return;
  }
  string data;
  data.readfile(config->video.shader);
  video.set(Video::Shader, (const char*)data);
}

void Utility::resize(bool resizeWindow) {
  if(application->active == nullptr) return;
  Geometry geometry = presentation->geometry();
  unsigned width  = system().information.width;
  unsigned height = system().information.height;

  unsigned scaledWidth  = geometry.width  / width;
  unsigned scaledHeight = geometry.height / height;
  unsigned multiplier   = max(1u, min(scaledWidth, scaledHeight));

  if(config->video.aspectCorrection) {
    width *= system().information.aspectRatio;
  }

  width  *= multiplier;
  height *= multiplier;

  unsigned scaleMode = 0;

  if(config->video.scaleMode == 1) {
    width  = (double)width * ((double)geometry.height / height);
    height = geometry.height;
  }

  if(config->video.scaleMode == 2) {
    width  = geometry.width;
    height = geometry.height;
  }

  if(resizeWindow == false) {
    if(geometry.width  < width ) width  = geometry.width;
    if(geometry.height < height) height = geometry.height;

    presentation->viewport.setGeometry({
      (geometry.width - width) / 2, (geometry.height - height) / 2, width, height
    });
  } else {
    presentation->setGeometry({geometry.x, geometry.y, width, height});
    presentation->viewport.setGeometry({0, 0, width, height});
  }

  presentation->synchronize();
}

void Utility::toggleFullScreen() {
  static Geometry geometry;

  if(presentation->fullScreen() == false) {
    geometry = presentation->geometry();
    presentation->setMenuVisible(false);
    presentation->setStatusVisible(false);
    presentation->setFullScreen(true);
    input.acquire();
  } else {
    input.unacquire();
    presentation->setMenuVisible(true);
    presentation->setStatusVisible(true);
    presentation->setFullScreen(false);
    presentation->setGeometry(geometry);
  }

  resize();
}

void Utility::updateStatus() {
  time_t currentTime = time(0);
  string text;
  if((currentTime - statusTime) <= 2) {
    text = statusMessage;
  } else if(application->active == nullptr) {
    text = "No cartridge loaded";
  } else if(application->pause || application->autopause) {
    text = "Paused";
  } else {
    text = statusText;
  }
  if(text != presentation->statusText()) {
    presentation->setStatusText(text);
  }
}

void Utility::setStatusText(const string &text) {
  statusText = text;
}

void Utility::showMessage(const string &message) {
  statusTime = time(0);
  statusMessage = message;
}

Utility::Utility() {
  tracerEnable = false;
  statusTime = 0;
}
