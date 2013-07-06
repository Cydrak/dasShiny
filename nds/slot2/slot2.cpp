#include <nds/nds.hpp>

namespace NintendoDS {

Slot2 slot2;


Slot2::Slot2() {
  cart = nullptr;
}

void Slot2::load(Expansion* cart) {
  this->cart = cart;
  cart->power();
}

Expansion* Slot2::unload() {
  auto r = cart;
  cart = nullptr;
  return r;
}

void Slot2::power() {
  if(cart) cart->power();
}

uint32 Slot2::read(uint32 addr, uint32 size) {
  if(!cart) return 0xffffffff;
  if(size == Word) {
    addr &= 0x01fffffc;
    return cart->read(addr+0)<< 0
         | cart->read(addr+2)<<16;
  }
  return cart->read(addr & 0x01fffffe) * 0x00010001;
}

void Slot2::write(uint32 addr, uint32 size, uint32 data) {
  if(size == Byte || !cart) return;
  if(size == Word) {
    addr &= 0x01fffffc;
    cart->write(addr+0, data>> 0);
    cart->write(addr+2, data>>16);
    return;
  }
  cart->write(addr & 0x01fffffe, data);
}


GameCart::GameCart(const stream& memory, uint32 esize) {
  rom.size = esize;
  rom.data = new uint8[rom.size];
  memset(rom.data, 0xff, rom.size);
  memory.read(rom.data, min(memory.size(), rom.size));
}

uint16 GameCart::read(uint32 addr) {
  return addr < rom.size? rom.read(addr, Half) : 0xffff;
}

}
