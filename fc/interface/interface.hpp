#ifndef FC_HPP
namespace Famicom {
#endif

struct ID {
  enum : unsigned {
    System,
    Famicom,
  };

  enum : unsigned {
    ProgramROM,
    ProgramRAM,
    CharacterROM,
    CharacterRAM,
  };

  enum : unsigned {
    Port1 = 1,
    Port2 = 2,
  };
};

struct Interface : Emulator::Interface {
  double videoFrequency();
  double audioFrequency();

  bool loaded();
  string sha256();
  unsigned group(unsigned id);
  void load(unsigned id, const string &manifest);
  void save();
  void load(unsigned id, const stream &stream, const string &manifest = "");
  void save(unsigned id, const stream &stream);
  void unload();

  void power();
  void reset();
  void run();

  serializer serialize();
  bool unserialize(serializer&);

  void cheatSet(const lstring&);

  void paletteUpdate();

  Interface();

private:
  vector<Device> device;
};

extern Interface *interface;

#ifndef FC_HPP
}
#endif
