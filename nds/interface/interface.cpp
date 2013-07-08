#include <nds/nds.hpp>
#include <sys/time.h>

namespace NintendoDS {

Interface *interface = nullptr;

double Interface::videoFrequency() {
  return 2.*33513982 / (2*6 * 263*355);
}

double Interface::audioFrequency() {
  return 2.*33513982 / (2 * 1024);
}

bool Interface::loaded() {
  return true;
}

string Interface::title() {
  return gameTitle;
}

unsigned Interface::group(unsigned id) {
  if(id == SystemManifest)                 return System;
  if(id == ARM7BIOS  || id == ARM9BIOS)    return System;
  if(id == Firmware  || id == Clock)       return System;
  
  if(id == GameManifest)                   return NintendoDS;
  if(id == Slot1ROM  || id == Slot1EEPROM) return NintendoDS;
  if(id == Slot1FRAM || id == Slot1Flash)  return NintendoDS;
  
  if(id == Slot2ROM  || id == Slot2RAM)    return NintendoDS;
  if(id == Slot2SRAM || id == Slot2EEPROM) return NintendoDS;
  if(id == Slot2FRAM || id == Slot2Flash)  return NintendoDS;
  return 0;
}

void Interface::load(unsigned id) {
  if(id == NintendoDS) {
    interface->loadRequest(SystemManifest, "manifest.xml");
    interface->loadRequest(GameManifest,   "manifest.xml");
    
    if(systemManifest) {
      auto elem = Markup::Document(systemManifest);
      
      interface->loadRequest(ARM7BIOS, elem["system/arm7/bios/data"].data);
      interface->loadRequest(ARM9BIOS, elem["system/arm9/bios/data"].data);
      interface->loadRequest(Firmware, elem["system/flash/data"].data);
      interface->loadRequest(Clock,    elem["system/rtc/data"].data);
      
      if(!arm7.bios.size)       interface->notify("ARM7 ROM not found.");
      if(!arm9.bios.size)       interface->notify("ARM9 ROM not found.");
      if(!system.firmware.size) interface->notify("System firmware not found.");
    } else {
      interface->notify("System manifest not found.");
    }
    
    gameTitle = basename(notdir(interface->path(NintendoDS).rtrim("/")));
    
    if(!gameManifest) {
      // Default to 1GB ROM with no save. Since GameCard bounds-checks,
      // we need only allocate enough to hold the stream passed in.
      gameManifest = "<cartridge><slot1>\n"
                     "  <rom name=\"rom\" size=\"0x40000000\" />\n"
                     "</slot1><cartridge>\n";
    }
    
    // <cartridge title=..>
    XML::Document elem(gameManifest);
    if(elem.error != "") { print(elem.error,"\n"); return; }
    
    if(auto title = elem["cartridge/title"].data)
      gameTitle = title;
    
    const auto &eslot1 = elem["cartridge/slot1"];
    
    // <slot1>
    if(eslot1.exists()) {
      // <rom name=.. id=.. size=.. sha256=.. />
      if(eslot1["rom"].exists()) {
        string file   = string(eslot1["rom/name"].data);
        uint32 size   = numeral(eslot1["rom/size"].data);
        uint32 chipId = numeral(eslot1["rom/id"].data);
        
        print("Loading slot-1 ROM (", file, ").. ");
        slot1.load(new GameCard(chipId));
        interface->loadRequest(Slot1ROM, file);
        print("\n");
      }
      // <save name=.. type=EEPROM,FRAM,Flash size=.. [page|id=..] />
      if(eslot1["save"].exists()) {
        string file   = string(eslot1["save/name"].data);
        string type   = string(eslot1["save/type"].data);
        uint32 size   = numeral(eslot1["save/size"].data);
        uint32 psize  = numeral(eslot1["save/page"].data);  // EEPROM only
        uint32 chipId = numeral(eslot1["save/id"].data);    // Flash only
        
        unsigned id = 0;
        if(auto card = slot1.card) {
          if(type.iequals("eeprom")) id = Slot1EEPROM, card->spi = new EEPROM(size, psize);
          if(type.iequals("flash"))  id = Slot1Flash,  card->spi = new Flash(size, chipId);
          if(type.iequals("fram"))   id = Slot1FRAM,   card->spi = new FRAM(size);
        }
        if(id) {
          print("Loading slot-1 ",eslot1["save/type"].data," (", file, ").. ");
          interface->loadRequest(id, file);
          print("\n");
        }
      }
      // <irport />
      if(eslot1["irport"].exists()) {
        // Required by Pokemon HG/SS and B/W. These cards have an infrared port
        // built-in. Since there's only one /CS, access to flash memory passes
        // through the infrared bridge via an override command.
        slot1.card->spi = new IRPort(slot1.card->spi);
      }
    }
  }
  
  // Provide blank images if needed (we'd crash otherwise).
  if(system.firmware.size == 0) {
    system.firmware.size = 0x40000;
    system.firmware.data = new uint8[system.firmware.size];
    memset(system.firmware.data, 0xff, system.firmware.size);
  }
  if(arm7.bios.size == 0) {
    arm7.bios.size = 4;
    arm7.bios.data = new uint32[arm7.bios.size/4];
    memset(arm7.bios.data, 0xef, arm7.bios.size);
  }
  if(arm9.bios.size == 0) {
    arm9.bios.size = 4;
    arm9.bios.data = new uint32[arm9.bios.size/4];
    memset(arm9.bios.data, 0xef, arm9.bios.size);
  }
}

void Interface::load(unsigned id, const stream& memory) {
  if(id == SystemManifest) { systemManifest = memory.text(); return; }
  if(id == GameManifest)   { gameManifest   = memory.text(); return; }

  if(id == ARM7BIOS) return system.loadArm7Bios(memory);
  if(id == ARM9BIOS) return system.loadArm9Bios(memory);
  if(id == Firmware) return system.loadFirmware(memory);
  if(id == Clock)    return system.loadRTC(memory);
  
  XML::Document elem(gameManifest);
  const auto &eslot1 = elem["cartridge/slot1"];
  
  if(eslot1.exists()) {
    if(eslot1["rom"].exists() && id == Slot1ROM) {
      string hash = string(eslot1["rom/sha256"].data);
      uint32 size = numeral(eslot1["rom/size"].data);
      
      if(auto card = slot1.card) {
        delete card->rom.data;
        card->rom.size = min(memory.size(), size);
        card->rom.data = new uint8[card->rom.size];
        card->size     = bit::round(size);
        
        memory.read(card->rom.data, card->rom.size);
        card->sha256 = ::sha256(card->rom.data, card->rom.size);
        
        if(hash && hash != card->sha256) print("SHA256 mismatch.");
        else                             print("OK.");
      }
    }
    if(eslot1["save"].exists() && slot1.card && (id==Slot1EEPROM || id==Slot1Flash || id==Slot1FRAM)) {
      uint32 size = numeral(eslot1["save/size"].data);
      
      if(auto save = slot1.card->spi) {
        if(auto irport = dynamic_cast<IRPort*>(save))
          save = irport->slave;
        
        if(auto media = dynamic_cast<StaticMemory*>(save)) {
          memory.read(media->data, min(media->size, memory.size()));
          print("OK.");
        }
      }
    }
  }
}



void Interface::save() {
  if(systemManifest) {
    XML::Document elem(systemManifest);
    interface->saveRequest(Firmware, elem["system/flash/data"].data);
    interface->saveRequest(Clock,    elem["system/rtc/data"].data);
  }
  
  XML::Document elem(gameManifest);
  const auto &eslot1 = elem["cartridge/slot1"];
  
  if(eslot1.exists() && eslot1["save"].exists()) {
    string file = eslot1["save/name"].data;
    string type = eslot1["save/type"].data;
    
    print("Saving slot-1 ",type,".. ");
    
    if(type.iequals("eeprom")) interface->saveRequest(Slot1EEPROM, file);
    if(type.iequals("flash"))  interface->saveRequest(Slot1Flash, file);
    if(type.iequals("fram"))   interface->saveRequest(Slot1FRAM, file);
    
    print("\n");
  }
}

void Interface::save(unsigned id, const stream& memory) {
  if(id == Firmware) return system.saveFirmware(memory);
  if(id == Clock)    return system.saveRTC(memory);

  if(slot1.card && (id == Slot1EEPROM || id == Slot1Flash || id == Slot1FRAM)) {
    if(auto save = slot1.card->spi) {
      if(auto irport = dynamic_cast<IRPort*>(save))
        save = irport->slave;
      
      if(auto media = dynamic_cast<StaticMemory*>(save)) {
        memory.write(media->data, media->size);
        print("OK.");
      }
    }
  }
}



void Interface::unload() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  system.clock.freeze(tv.tv_sec, tv.tv_usec);
  system.running = false;
  
  save();
  delete slot1.unload();
  delete slot2.unload();
}

void Interface::power() {
  system.power();
}

void Interface::run() {
  if(!system.running) {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    system.clock.thaw(tv.tv_sec, tv.tv_usec);
    system.running = true;
  }
  system.run();
}

serializer Interface::serialize() {
  return {};
}

bool Interface::unserialize(serializer &s) {
  return false;
}

void Interface::paletteUpdate() {
  for(unsigned color = 0; color < 01000000; color++) {
    uint16 r = uint6(color>> 0) * 010101/4;
    uint16 g = uint6(color>> 6) * 010101/4;
    uint16 b = uint6(color>>12) * 010101/4;
    
    palette[color] = interface->videoColor(color, r,g,b);
  }
}

void Interface::videoRefresh(const uint32_t *data, unsigned pitch, unsigned width, unsigned height) {
  static uint32_t pixels[256*384];
  
  for(unsigned y = 0; y < 384; y++) {
    const uint32 *src = &data[y*pitch/4];
    uint32 *dest = &pixels[y*256];
    
    for(unsigned x = 0; x < 256; x++)
      dest[x] = palette[src[x] & 0777777];
  }
  return bind->videoRefresh(pixels, 256*4, 256, 384);
}


Interface::Interface() {
  interface = this;
  
  information.name = "Nintendo DS";
  information.width = 256;
  information.height = 384;
  information.aspectRatio = 1.0;
  information.overscan = false;
  information.resettable = false;
  information.capability.states = false;
  information.capability.cheats = false;
  
  media.append({NintendoDS, "Nintendo DS", "nds", true});
  
  // Input devices and ports
  emptySlot = Device{ID::Device::Empty,    1<<ID::Port::Slot1
                                         | 1<<ID::Port::Slot2, "Empty"};
  gameCard  = Device{ID::Device::GameCard, 1<<ID::Port::Slot1, "Game Card"};
  gamePak   = Device{ID::Device::GamePak,  1<<ID::Port::Slot2, "Game Pak"};
  builtIn   = Device{ID::Device::BuiltIn,  1<<ID::Port::BuiltIn, ""};
  
    builtIn.input.append({ID::Buttons::A,           0, "A"});
    builtIn.input.append({ID::Buttons::B,           0, "B"});
    builtIn.input.append({ID::Buttons::Select,      0, "Select"});
    builtIn.input.append({ID::Buttons::Start,       0, "Start"});
    builtIn.input.append({ID::Buttons::Right,       0, "Right"});
    builtIn.input.append({ID::Buttons::Left,        0, "Left"});
    builtIn.input.append({ID::Buttons::Up,          0, "Up"});
    builtIn.input.append({ID::Buttons::Down,        0, "Down"});
    builtIn.input.append({ID::Buttons::R,           0, "R"});
    builtIn.input.append({ID::Buttons::L,           0, "L"});
    builtIn.input.append({ID::Buttons::X,           0, "X"});
    builtIn.input.append({ID::Buttons::Y,           0, "Y"});
    
    builtIn.input.append({ID::Sensors::X,           2, "Touch X"});
    builtIn.input.append({ID::Sensors::Y,           2, "Touch Y"});
    builtIn.input.append({ID::Sensors::PressureD,   0, "Touched (digital)"});
    builtIn.input.append({ID::Sensors::Pressure,    2, "Pressure (analog)"});
    
    builtIn.order.append(6, 7, 5, 4, 2, 3, 1, 0, 10, 11, 9, 8, 12, 13, 14, 15);
  
  // Ports
  port.append({ID::Port::BuiltIn, "Built-in"});
//port.append({ID::Port::Slot1,   "Slot 1"});
//port.append({ID::Port::Slot2,   "Slot 2"});
  
  device.append(builtIn);
//device.append(emptySlot);
  
  for(auto &port : this->port)
    for(auto &device : this->device)
      if(device.portmask & 1<<port.id)
        port.device.append(device);
}

}
