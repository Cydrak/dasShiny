#include "../ethos.hpp"
#include <nds/utility/utility.hpp>

Utility *utility = nullptr;

void Utility::setInterface(Emulator::Interface *emulator) {
  program->active = emulator;
  presentation->synchronize();
}

string Utility::libraryPath() {
  string path = string::read({configpath(), "dasShiny/library.cfg"}).strip().transform("\\", "/");
  if(path.empty()) path = {userpath(), "Emulation/Nintendo DS/"};
  if(!path.endswith("/")) path.append("/");
  return path;
}

void Utility::importMedia(string pathname) {
  string container;
  print("Importing ",pathname,"\n");
  
  if(!NintendoDS::validateHeader(filestream(pathname, file::mode::read))) {
    MessageWindow().setTitle("Import failed").setText(
      {"Couldn't import ",pathname,".\n"
       "\n"
       "This file doesn't look like a Nintendo DS title.\n"
       "If it is, the header might be damaged or ill-formed."})
      .error();
  }
  else if(!NintendoDS::importROMImage(container, libraryPath(), pathname)) {
    MessageWindow().setTitle("Import failed").setText(
      {"Couldn't import ",pathname,".\n"
       "\n"
       "Check to see if you've got sufficient permissions and disk space."})
      .error();
  }
  else {
    auto e = program->emulator[0];
    loadMedia(e, e->media[0], container);
  }
}

//load from command-line, etc
void Utility::loadMedia(string pathname) {
  pathname.transform("\\", "/");
  if(pathname.endswith("/")) pathname.rtrim("/");

  //if a filename was provided: convert to game folder and then load
  if(!directory::exists(pathname) && file::exists(pathname))
    return importMedia(pathname);

  if(!directory::exists(pathname)) return;
  string type = extension(pathname);

  //determine type by comparing extension against all emulation cores
  for(auto &emulator : program->emulator) {
    for(auto &media : emulator->media) {
      if(media.bootable == false) continue;
      if(type != media.type) continue;
      return loadMedia(emulator, media, {pathname, "/"});
    }
  }

  MessageWindow().setText("Unable to determine media type.").warning();
}

//load menu option selected
void Utility::loadMedia(Emulator::Interface *emulator, Emulator::Interface::Media &media) {
  string pathname = browser->select({"Load ", media.name}, media.type);
  if(!directory::exists(pathname)) return;
  return loadMedia(emulator, media, pathname);
}

//load base cartridge
void Utility::loadMedia(Emulator::Interface *emulator, Emulator::Interface::Media &media, const string &pathname) {
  unload();
  setInterface(emulator);
  path(0) = {media.name, ".sys/"};
  path(media.id) = pathname;
  this->pathname.append(pathname);

  system().load(media.id);
  system().power();

  presentation->setSystemName(media.name);
  presentation->setVisible();
  load();
}

//request from emulation core to load non-volatile media folder
void Utility::loadRequest(unsigned id, const string &name, const string &type) {
  string pathname = browser->select({"Load ", name}, type);
  if(pathname.empty()) return;
  path(id) = pathname;
  this->pathname.append(pathname);

  system().load(id);
}

//request from emulation core to load non-volatile media file
void Utility::loadRequest(unsigned id, const string &path) {
  string pathname = {pathRequest(system().group(id)), path};
  
  // First try loading from a folder as normal.
  if(file::exists(pathname)) {
    mmapstream stream(pathname);
    return system().load(id, stream);
  }
  // Otherwise, if system folder, look for a resource.
  if(system().group(id) == 0) {
    lstring lpath = this->path(system().group(id)).split("/");
    pathname = {lpath(lpath.size() - 2),"/", path};
    
    if(auto contents = program->getUserResource(pathname)) {
      vectorstream stream(contents());
      return system().load(id, stream);
    }
  }
}

//request from emulation core to save non-volatile media file
void Utility::saveRequest(unsigned id, const string &path) {
  string pathname = {pathRequest(system().group(id)), path};
  filestream stream(pathname, file::mode::write);
  return system().save(id, stream);
}

//request to return path that would be loaded or saved.
//Note that group(id) isn't called; this uses the group directly.
string Utility::pathRequest(unsigned group) {
  string pathname = this->path(group);
  
  // Substitute correct system folder
  if(group == 0)
    pathname = program->savePath(pathname);
  
  return pathname;
}

void Utility::connect(unsigned port, unsigned device) {
  if(program->active == nullptr) return;
  system().connect(port, device);
}

void Utility::power() {
  if(program->active == nullptr) return;
  system().power();
}

void Utility::reset() {
  if(program->active == nullptr) return;
  system().reset();
}

void Utility::load() {
  presentation->setTitle(system().title());

  system().paletteUpdate();
  synchronizeDSP();

  resize();
}

void Utility::unload() {
  if(program->active == nullptr) return;
  if(tracerEnable) tracerToggle();

  system().unload();
  path.reset();
  pathname.reset();
  setInterface(nullptr);

  video.clear();
  audio.clear();
  presentation->setTitle({Emulator::Name, " ", Emulator::Version});
}

void Utility::saveState(unsigned slot) {
  if(program->active == nullptr) return;
}

void Utility::loadState(unsigned slot) {
  if(program->active == nullptr) return;
}

void Utility::tracerToggle() {
  if(program->active == nullptr) return;
  tracerEnable = !tracerEnable;
  bool result = system().tracerEnable(tracerEnable);
  if( tracerEnable &&  result) return utility->showMessage("Tracer activated");
  if( tracerEnable && !result) return tracerEnable = false, utility->showMessage("Unable to activate tracer");
  if(!tracerEnable &&  result) return utility->showMessage("Tracer deactivated");
  if(!tracerEnable && !result) return utility->showMessage("Unable to deactivate tracer");
}

void Utility::synchronizeDSP() {
  if(program->active == nullptr) return;

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
  if(program->active == nullptr) {
    auto geometry = presentation->geometry();
    presentation->viewport.setGeometry({0, 0, geometry.width, geometry.height});
    return;
  }

  Geometry geometry = presentation->geometry();
  unsigned width  = system().information.width;
  unsigned height = system().information.height;

  unsigned scaledWidth  = geometry.width  / width;
  unsigned scaledHeight = geometry.height / height;
  unsigned multiplier   = max(1u, min(scaledWidth, scaledHeight));

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
  } else if(program->active == nullptr) {
    text = "No cartridge loaded";
  } else if(program->pause || program->autopause) {
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
