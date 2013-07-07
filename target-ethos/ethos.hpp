#include <emulator/emulator.hpp>

#include <nall/platform.hpp>
#include <nall/config.hpp>
#include <nall/directory.hpp>
#include <nall/dsp.hpp>
#include <nall/invoke.hpp>
#include <nall/map.hpp>
#include <nall/stream/file.hpp>
#include <nall/stream/memory.hpp>
#include <nall/stream/mmap.hpp>
#include <nall/stream/vector.hpp>
using namespace nall;

#include <phoenix/phoenix.hpp>
using namespace phoenix;

#include <ruby/ruby.hpp>
using namespace ruby;

#include "configuration/configuration.hpp"
#include "interface/interface.hpp"
#include "utility/utility.hpp"
#include "input/input.hpp"
#include "window/window.hpp"
#include "general/general.hpp"
#include "settings/settings.hpp"
#include "resource/resource.hpp"

Emulator::Interface& system();

struct Program {
  vector<Emulator::Interface*> emulator;
  Emulator::Interface *active = nullptr;

  bool quit;
  bool pause;
  bool autopause;
  unsigned depth;  //color depth; 24 or 30 bpp

  string basepath;
  string userpath;

  string normalFont;
  string boldFont;
  string titleFont;
  string monospaceFont;

  bool focused();
  string path(const string &filename, bool forwriting = false);
  void main();
  void bootstrap();
  Program(int argc, char **argv);
};

extern Program *program;
extern DSP dspaudio;
