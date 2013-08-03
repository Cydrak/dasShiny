#include <nds/nds.hpp>
#include <sys/time.h>

namespace NintendoDS {

static unsigned decodeSize(string s) {
  auto size = numeral(s);
  if(s.endswith("K")) size <<= 10;
  if(s.endswith("M")) size <<= 20;
  if(s.endswith("G")) size <<= 30;
  return size;
}


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

string Interface::systemInfo(string path) {
  for(auto &sys : systemDoc.find("system")) {
    if(sys.text().imatch("*lite*"))
      return sys[path].text();
  }
  return "";
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

bool Interface::loadSystem() {
  interface->loadRequest(SystemManifest, "manifest.bml");
  
  if(systemManifest) {
    systemDoc = BML::Document(systemManifest);
    if(systemDoc.error) {
      print("Error in system manifest: ",systemDoc.error,"\n");
      return false;
    }
    interface->loadRequest(ARM7BIOS, systemInfo("memory/arm7/data"));
    interface->loadRequest(ARM9BIOS, systemInfo("memory/arm9/data"));
    interface->loadRequest(Firmware, systemInfo("memory/flash/data"));
    interface->loadRequest(Clock,    systemInfo("chipset/clock/data"));
    
    if(!arm7.bios.size)       interface->notify("ARM7 ROM not found.");
    if(!arm9.bios.size)       interface->notify("ARM9 ROM not found.");
    if(!system.firmware.size) interface->notify("System firmware not found.");
  }
  else {
    interface->notify("System manifest not found.");
    return false;
  }
  return true;
}

Markup::Node Interface::gameSaveInfo() {
  Markup::Node save;
  if(!save.exists()) save = gameDoc["title/eeprom"];
  if(!save.exists()) save = gameDoc["title/flash"];
  if(!save.exists()) save = gameDoc["title/fram"];
  return save;
}

void Interface::load(unsigned id) {
  if(id == NintendoDS) {
    if(!loadSystem()) return;
    
    gameManifest = "title\n  rom=1G data:rom\n";  // default
    interface->loadRequest(GameManifest, "manifest.bml");
    
    gameTitle = basename(notdir(interface->path(NintendoDS).rtrim("/")));
    gameDoc   = BML::Document(gameManifest);
    if(gameDoc.error) {
      print("Error in game manifest: ",gameDoc.error,"\n");
      return;
    }
    
    auto title = gameDoc["title"];
    auto save  = gameSaveInfo();
    
    if(title.text())
      gameTitle = title.text();
    
    if(title["rom"].exists()) {
      uint32 chipId = numeral(title["rom/id"].text());
      uint32 size   = decodeSize(title["rom/size"].text());
      string file   = string(title["rom/data"].text());
      
      print("Loading ROM in slot-1 (", file, ").. ");
      slot1.load(new GameCard(chipId));
      interface->loadRequest(Slot1ROM, file);
      print("\n");
    }
    
    auto card = slot1.card;
    
    if(card && save.exists()) {
      string file   =     string(save["data"].text());
      uint32 size   = decodeSize(save["size"].text());
      uint32 psize  =    numeral(save["page"].text());  // EEPROM only
      uint32 chipId =    numeral(save["id"].text());    // Flash only
      
      unsigned id = 0;
      if(save.name == "eeprom") id = Slot1EEPROM, card->spi = new EEPROM(size, psize);
      if(save.name == "flash")  id = Slot1Flash,  card->spi = new Flash(size, chipId);
      if(save.name == "fram")   id = Slot1FRAM,   card->spi = new FRAM(size);
      
      if(id) {
        print("Loading ",save.name," in slot-1 (", file, ").. ");
        interface->loadRequest(id, file);
        print("\n");
      }
    }
    
    if(title["infrared"].exists()) {
      // Required by Pokemon HG/SS and B/W. These cards have an infrared port
      // built-in. Since there's only one /CS, access to flash memory passes
      // through the infrared bridge via an override command.
      slot1.card->spi = new IRPort(slot1.card->spi);
    }
  }
}

void Interface::load(unsigned id, const stream& memory) {
  if(id == SystemManifest) { systemManifest = memory.text(); return; }
  if(id == GameManifest)   { gameManifest   = memory.text(); return; }
  
  // System state
  if(id == ARM7BIOS) return system.loadArm7Bios(memory);
  if(id == ARM9BIOS) return system.loadArm9Bios(memory);
  if(id == Firmware) return system.loadFirmware(memory);
  if(id == Clock)    return system.loadRTC(memory);
  
  // Game card
  auto title = gameDoc["title"];
  auto card = slot1.card; if(!card) return;
  
  if(id == Slot1ROM) {
    uint32 size = decodeSize(title["rom/size"].text());
    string hash = string(title["rom/decrypted/sha256"].text());
    
    delete card->rom.data;
    card->rom.size = min(memory.size(), size);
    card->rom.data = new uint8[card->rom.size];
    card->size     = bit::round(size);
    
    memory.read(card->rom.data, card->rom.size);
    card->sha256 = ::sha256(card->rom.data, card->rom.size);
    
    if(hash && hash != card->sha256) print("SHA256 mismatch.");
    else                             print("OK.");
  }
  
  auto save = slot1.card->spi;
  if(!save) return;
  
  if(id == Slot1EEPROM || id == Slot1Flash || id == Slot1FRAM) {
    if(auto irport = dynamic_cast<IRPort*>(save))
      save = irport->slave;
    
    if(auto media = dynamic_cast<StaticMemory*>(save)) {
      memory.read(media->data, min(media->size, memory.size()));
      print("OK.");
    }
  }
}



void Interface::save() {
  if(systemManifest) {
    interface->saveRequest(Firmware, systemInfo("memory/flash/data"));
    interface->saveRequest(Clock,    systemInfo("chipset/clock/data"));
  }
  
  auto save = gameSaveInfo();
  
  if(save.exists()) {
    string file = save["data"].text();
    print("Saving ",save.name," in slot-1 (", file, ") ..");
    
    if(save.name == "eeprom") interface->saveRequest(Slot1EEPROM, file);
    if(save.name == "flash")  interface->saveRequest(Slot1Flash, file);
    if(save.name == "fram")   interface->saveRequest(Slot1FRAM, file);
    print("\n");
  }
}

void Interface::save(unsigned id, const stream& memory) {
  if(id == Firmware) return system.saveFirmware(memory);
  if(id == Clock)    return system.saveRTC(memory);
  
  auto card = slot1.card; if(!card) return;
  auto save = slot1.card->spi; if(!save) return;
  
  if(id == Slot1EEPROM || id == Slot1Flash || id == Slot1FRAM) {
    if(auto irport = dynamic_cast<IRPort*>(save))
      save = irport->slave;
    
    if(auto media = dynamic_cast<StaticMemory*>(save)) {
      memory.write(media->data, media->size);
      print("OK.");
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

void Interface::keyboardEvent(unsigned type, unsigned long code) {
  uint32 event = type<<24 | code;
  
  if(system.keyboardEvents.size() < 256)
    system.keyboardEvents.append(event);
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
    builtIn.input.append({ID::Sensors::Touched,     0, "Touched (digital)"});
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
