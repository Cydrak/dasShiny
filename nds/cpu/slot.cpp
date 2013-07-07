
uint32 CPUCore::regSlot1Control() {
  return slot1.spi.baud<<0 | slot1.spi.hold<<6
       | slot1.spi.busy<<7 | slot1.spi.enable<<13
       | slot1.transferIrq<<14 | slot1.enable<<15
       | slot1.spi.data<<16;
}

uint32 CPUCore::regSlot1RomControl() {
  return slot1.bfLatency1<<0 | slot1.xorData<<13 | slot1.xorEnable<<14
       | slot1.bfLatency2<<16 | slot1.xorCmds<<22 | slot1.dataReady<<23
       | slot1.blockSize<<24 | slot1.speed<<27 | slot1.bfAddClocks<<28
       | slot1.nReset<<29 | slot1.write<<30 | slot1.transferPending<<31;
}

uint32 CPUCore::regSlot1RomCommand(unsigned index) {
  auto cmd = slot1.command;
  if(index == 0) cmd >>= 32;
  
  return (cmd>>24 & 0xff) <<  0
       | (cmd>>16 & 0xff) <<  8
       | (cmd>> 8 & 0xff) << 16
       | (cmd>> 0 & 0xff) << 24;
}

uint32 CPUCore::regSlot1RomRecv() {
  uint32 data = 0;
  if(slot1.write == false) {
    data += slot1.blockTransfer() << 0;
    data += slot1.blockTransfer() << 8;
    data += slot1.blockTransfer() << 16;
    data += slot1.blockTransfer() << 24;
  }
  return data;
}

void CPUCore::regSlot1RomSend(uint32 data) {
  if(slot1.write == true) {
    slot1.blockTransfer(data>>0);
    slot1.blockTransfer(data>>8);
    slot1.blockTransfer(data>>16);
    slot1.blockTransfer(data>>24);
  }
}

void CPUCore::regSlot1Control(uint32 data, uint32 mask) {
  if(mask & 0x000000ff) {
    slot1.spi.baud = data>>0;
    slot1.spi.hold = data>>6;
  }
  if(mask & 0x0000ff00) {
    slot1.spi.enable = data>>13;
    slot1.transferIrq = data>>14;
    slot1.enable = data>>15;
    
    if(slot1.enable == false || slot1.spi.enable == false) {
      if(slot1.card && slot1.card->spi) {
        //print("slot1 spi: deselect\n");
        slot1.card->spi->select(false);
      }
    }
  }
  if(mask & 0x00ff0000) {
    // SPI transfer
    if(!slot1.spi.enable) return;
    slot1.spi.data = slot1.spiTransfer(data>>16);
    //print("slot1 spi: w ",hex<2>(data>>16)," r ",hex<2>(slot1.spi.data),"\n");
  }
}

void CPUCore::regSlot1RomControl(uint32 data, uint32 mask) {
  if(mask & 0x00001fff) {
    slot1.bfLatency1 ^= (slot1.bfLatency1 ^ data) & mask;
  }
  if(mask & 0x0000e000) {
    slot1.xorData     = data>>13;
    slot1.xorEnable   = data>>14;
    if(data & 1<<15) {
      // Write-only; latches new RNG seeds from 0x040001b0..1bb
      slot1.lfsr[0] = config.xorSeeds[0];
      slot1.lfsr[1] = config.xorSeeds[1];
    }
  }
  if(mask & 0x00ff0000) {
    slot1.bfLatency2  = data>>16;
    slot1.xorCmds     = data>>22;
  }
  if(mask & 0xff000000) {
    slot1.blockSize   = data>>24;
    slot1.speed       = data>>27;
    slot1.bfAddClocks = data>>28;
  //slot1.nReset      = data>>29;  // unsure if this does anything..
    slot1.write       = data>>30;
    if(data & 1<<31)
      slot1.startBlockTransfer();
  }
}

void CPUCore::regSlot1RomCommand(unsigned index, uint32 data, uint32 mask) {
  auto &cmd = slot1.command;
  uint64 ldata = data;
  
  // Big endian commands, argh!
  if(index == 0) {
    if(mask & 0x000000ff) cmd ^= (cmd ^ ldata<<56) & 0xff00000000000000;
    if(mask & 0x0000ff00) cmd ^= (cmd ^ ldata<<40) & 0x00ff000000000000;
    if(mask & 0x00ff0000) cmd ^= (cmd ^ ldata<<24) & 0x0000ff0000000000;
    if(mask & 0xff000000) cmd ^= (cmd ^ ldata<< 8) & 0x000000ff00000000;
  } else {
    if(mask & 0x000000ff) cmd ^= (cmd ^ ldata<<24) & 0x00000000ff000000;
    if(mask & 0x0000ff00) cmd ^= (cmd ^ ldata<< 8) & 0x0000000000ff0000;
    if(mask & 0x00ff0000) cmd ^= (cmd ^ ldata>> 8) & 0x000000000000ff00;
    if(mask & 0xff000000) cmd ^= (cmd ^ ldata>>24) & 0x00000000000000ff;
  }
}

void CPUCore::regSlot1RomSeed(unsigned index, uint32 data, uint32 mask) {
  if(index == 2) {
    data &= 0x7f007f & mask;
    if(mask & 0x00007f) config.xorSeeds[0] = (config.xorSeeds[0] & 0xffffffff) | uint64(data>>0 )<<32;
    if(mask & 0x7f0000) config.xorSeeds[1] = (config.xorSeeds[1] & 0xffffffff) | uint64(data>>16)<<32;
  }
  else {
    config.xorSeeds[index] ^= (config.xorSeeds[index] ^ data) & mask;
  }
}



uint32 CPUCore::regSlot2Control() {
  return config.slot2ramTiming<<0 | config.slot2romTiming0<<2
       | config.slot2romTiming1<<4 | config.slot2phi<<5
       | !arm9.slot2access<<7 | !arm9.slot1access<<11
       | 1<<13 | 1<<14 | !arm9.ramPriority<<15;
}

void CPUCore::regSlot2Control(uint32 data, uint32 mask) {
  if(mask & 0xff) {
    config.slot2ramTiming = data>>0;
    config.slot2romTiming0 = data>>2;
    config.slot2romTiming1 = data>>4;
    config.slot2phi = data>>5;
  }
}
  
