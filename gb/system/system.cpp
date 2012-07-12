#include <gb/gb.hpp>

#define SYSTEM_CPP
namespace GameBoy {

#include "serialization.cpp"
System system;

void System::run() {
  scheduler.sync = Scheduler::SynchronizeMode::None;

  scheduler.enter();
  if(scheduler.exit_reason() == Scheduler::ExitReason::FrameEvent) {
    interface->videoRefresh(ppu.screen, 4 * 160, 160, 144);
  }
}

void System::runtosave() {
  scheduler.sync = Scheduler::SynchronizeMode::CPU;
  runthreadtosave();

  scheduler.sync = Scheduler::SynchronizeMode::All;
  scheduler.active_thread = ppu.thread;
  runthreadtosave();

  scheduler.sync = Scheduler::SynchronizeMode::All;
  scheduler.active_thread = apu.thread;
  runthreadtosave();

  scheduler.sync = Scheduler::SynchronizeMode::None;
}

void System::runthreadtosave() {
  while(true) {
    scheduler.enter();
    if(scheduler.exit_reason() == Scheduler::ExitReason::SynchronizeEvent) break;
    if(scheduler.exit_reason() == Scheduler::ExitReason::FrameEvent) {
      interface->videoRefresh(ppu.screen, 4 * 160, 160, 144);
    }
  }
}

void System::init() {
  assert(interface != nullptr);
}

void System::load(Revision revision) {
  this->revision = revision;
  serialize_init();
  if(revision == Revision::SuperGameBoy) return;  //Super Famicom core loads boot ROM for SGB

  string manifest, firmware;
  manifest.readfile({interface->path(ID::System), "manifest.xml"});
  XML::Document document(manifest);
  interface->loadRequest(
    revision == Revision::GameBoy ? ID::GameBoyBootROM : ID::GameBoyColorBootROM,
    document["system"]["cpu"]["firmware"]["name"].data
  );
  if(!file::exists({interface->path(ID::System), document["system"]["cpu"]["firmware"]["name"].data})) {
    interface->notify("Error: required firmware ", firmware, " not found.\n");
  }
}

void System::power() {
  bus.power();
  cartridge.power();
  cpu.power();
  ppu.power();
  apu.power();
  scheduler.init();

  clocks_executed = 0;
}

System::System() {
  for(auto &byte : bootROM.dmg) byte = 0;
  for(auto &byte : bootROM.sgb) byte = 0;
  for(auto &byte : bootROM.cgb) byte = 0;
}

}
