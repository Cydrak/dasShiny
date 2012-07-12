#include <fc/fc.hpp>

namespace Famicom {

#include "chip/chip.cpp"
#include "board/board.cpp"
Cartridge cartridge;

void Cartridge::Main() {
  cartridge.main();
}

void Cartridge::main() {
  board->main();
}

void Cartridge::load(const string &manifest) {
  information.markup = manifest;

  Board::load(manifest);  //this call will set Cartridge::board if successful
  if(board == nullptr) return;

  sha256_ctx sha;
  uint8 hash[32];
  sha256_init(&sha);
  sha256_chunk(&sha, board->prgrom.data, board->prgrom.size);
  sha256_chunk(&sha, board->chrrom.data, board->chrrom.size);
  sha256_final(&sha);
  sha256_hash(&sha, hash);
  string result;
  for(auto &byte : hash) result.append(hex<2>(byte));
  sha256 = result;

  system.load();
  loaded = true;
}

void Cartridge::unload() {
  if(loaded == false) return;
  loaded = false;
  memory.reset();
}

void Cartridge::power() {
  board->power();
}

void Cartridge::reset() {
  create(Cartridge::Main, 21477272);
  board->reset();
}

Cartridge::Cartridge() {
  loaded = false;
}

uint8 Cartridge::prg_read(unsigned addr) {
  return board->prg_read(addr);
}

void Cartridge::prg_write(unsigned addr, uint8 data) {
  return board->prg_write(addr, data);
}

uint8 Cartridge::chr_read(unsigned addr) {
  return board->chr_read(addr);
}

void Cartridge::chr_write(unsigned addr, uint8 data) {
  return board->chr_write(addr, data);
}

void Cartridge::scanline(unsigned y) {
  return board->scanline(y);
}

void Cartridge::serialize(serializer &s) {
  Thread::serialize(s);
  return board->serialize(s);
}

}
