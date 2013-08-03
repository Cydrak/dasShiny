#include "ui.hpp"
#include <nds/interface/interface.hpp>
#include <nds/utility/utility.hpp>
#include "resource/resource.cpp"

Program *program = nullptr;
DSP dspaudio;

Emulator::Interface& system() {
  if(program->active == nullptr) throw;
  return *program->active;
}

bool Program::focused() {
  return config->input.focusAllow || presentation->focused();
}

optional<vector<uint8>> Program::getResource(string rpath) {
  for(auto &file : resource.file)
    if(file.name == rpath)
      return {true, resource.extract(file)};
  return {};
}

optional<vector<uint8>> Program::getUserResource(string rpath) {
  string filepath = program->loadPath(rpath);
  if(file::exists(filepath))
    return {true, file::read(filepath)};
  
  return getResource(rpath);
}

string Program::loadPath(const string &filename) {
  string path = {userpath, filename};
  if(file::exists(path) || directory::exists(path)) return path;
  return {basepath, filename};
}

string Program::savePath(const string &filename) {
  string path = {userpath, filename};
  if(file::exists(path) || directory::exists(path)) return path;
  return {userpath, filename};
}

void Program::main() {
  inputManager->poll();
  utility->updateStatus();
  autopause = config->input.focusPause && presentation->focused() == false;

  if(active == nullptr || system().loaded() == false || pause || autopause) {
    audio.clear();
    usleep(20 * 1000);
    return;
  }

  system().run();
}

Program::Program(int argc, char **argv) {
  program = this;
  pause = false;
  autopause = false;

  extern char _binary_resources_zip_start[];
  extern char _binary_resources_zip_size[];
  
  resource.open((uint8_t*)_binary_resources_zip_start,
                (size_t)_binary_resources_zip_size);
  
  basepath = dir(realpath(argv[0]));
  userpath = {nall::configpath(), "dasShiny/"};
  directory::create(userpath);

  interface = new Interface;
  emulator.append(new NintendoDS::Interface);
  for(auto &system : emulator)
    system->bind = interface;
  
  string sysfolder = program->savePath("Nintendo DS.sys/");
  if(!directory::exists(sysfolder))
    directory::create(sysfolder);
  
  if(auto db = program->getUserResource("Nintendo DS.sys/database.bml"))
    NintendoDS::loadDatabase(vectorstream(db()).text());

  active = nullptr;

  if(Intrinsics::platform() == Intrinsics::Platform::OSX) {
    normalFont = Font::sans(12);
    boldFont = Font::sans(12, "Bold");
    titleFont = Font::sans(18, "Bold");
    monospaceFont = Font::monospace(8);
  } else if(Intrinsics::platform() == Intrinsics::Platform::Windows) {
    normalFont = "Tahoma, 8";
    boldFont = "Tahoma, 8, Bold";
    titleFont = "Tahoma, 11, Bold";
    monospaceFont = "Lucida Console, 8";
  } else {
    normalFont = Font::sans(8);
    boldFont = Font::sans(8, "Bold");
    titleFont = Font::sans(16, "Bold");
    monospaceFont = Font::monospace(8);
  }
  
  config = new Configuration;
  video.driver(config->video.driver);
  audio.driver(config->audio.driver);
  input.driver(config->input.driver);

  utility = new Utility;
  inputManager = new InputManager;
  windowManager = new WindowManager;
  browser = new Browser;
  presentation = new Presentation;
  outputSettings = new OutputSettings;
  inputSettings = new InputSettings;
  hotkeySettings = new HotkeySettings;
  advancedSettings = new AdvancedSettings;
  settings = new Settings;
  windowManager->loadGeometry();
  presentation->setVisible();
  utility->resize();

  video.set(Video::Handle, presentation->viewport.handle());
  if(!video.cap(Video::Depth) || !video.set(Video::Depth, depth = 30u)) {
    video.set(Video::Depth, depth = 24u);
  }
  if(video.init() == false) { video.driver("None"); video.init(); }

  audio.set(Audio::Handle, presentation->viewport.handle());
  if(audio.init() == false) { audio.driver("None"); audio.init(); }

  input.set(Input::Handle, presentation->viewport.handle());
  if(input.init() == false) { input.driver("None"); input.init(); }

  dspaudio.setPrecision(16);
  dspaudio.setBalance(0.0);
  dspaudio.setFrequency(96000);

  utility->synchronizeRuby();
  utility->updateShader();

  if(config->video.startFullScreen && argc >= 2) utility->toggleFullScreen();
  Application::processEvents();

  if(argc >= 2) utility->loadMedia(argv[1]);

  Application::main = {&Program::main, this};
  Application::run();

  utility->unload();
  config->save();
  browser->saveConfiguration();
  inputManager->saveConfiguration();
  windowManager->saveGeometry();
}

int main(int argc, char **argv) {
  #if defined(PLATFORM_WINDOWS)
  utf8_args(argc, argv);
  #endif

  Application::setName("dasShiny");

  Application::Cocoa::onActivate = [&] {
    presentation->setVisible();
  };

  Application::Cocoa::onAbout = [&] {
    MessageWindow()
    .setTitle({"About ", Emulator::Name})
    .setText({
      Emulator::Name, " ", Emulator::Version, "\n",
      "Author: ", Emulator::Author, "\n",
      "License: ", Emulator::License, "\n",
      "Website: ", Emulator::Website
    })
    .information();
  };

  Application::Cocoa::onPreferences = [&] {
    settings->setVisible();
    settings->panelList.setFocused();
  };

  Application::Cocoa::onQuit = [&] {
    Application::quit();
  };

  new Program(argc, argv);
  delete program;
  return 0;
}
