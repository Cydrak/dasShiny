
struct Expansion;

struct Slot2 {
  Slot2();
  void power();
  void load(Expansion* cart);
  Expansion* unload();
  
  uint32 read(uint32 addr, uint32 size);
  void write(uint32 addr, uint32 size, uint32 data);
  
  Expansion *cart;
};

struct Expansion {
  virtual ~Expansion() {}
  virtual void power() {}
  virtual uint16 read(uint32 addr) { return 0xffff; }
  virtual void write(uint32 addr, uint16 data) {}
};

struct GameCart : Expansion {
  GameCart(const stream& memory, uint32 size);
  uint16 read(uint32 addr);
  
  StaticMemory rom;
};


extern Slot2 slot2;
