#ifdef CARTRIDGE_CPP

void Cartridge::parse_markup(const char *markup) {
  mapping.reset();

  XML::Document document(markup);
  auto &cartridge = document["cartridge"];
  region = cartridge["region"].data != "PAL" ? Region::NTSC : Region::PAL;

  parse_markup_cartridge(cartridge);
  parse_markup_icd2(cartridge["icd2"]);
  parse_markup_bsx(cartridge["bsx"]);
  parse_markup_bsxslot(cartridge["bsxslot"]);
  parse_markup_sufamiturbo(cartridge["sufamiturbo"]);
  parse_markup_nss(cartridge["nss"]);
  parse_markup_sa1(cartridge["sa1"]);
  parse_markup_superfx(cartridge["superfx"]);
  parse_markup_armdsp(cartridge["armdsp"]);
  parse_markup_hitachidsp(cartridge["hitachidsp"]);
  parse_markup_necdsp(cartridge["necdsp"]);
  parse_markup_epsonrtc(cartridge["epsonrtc"]);
  parse_markup_sharprtc(cartridge["sharprtc"]);
  parse_markup_spc7110(cartridge["spc7110"]);
  parse_markup_sdd1(cartridge["sdd1"]);
  parse_markup_obc1(cartridge["obc1"]);
  parse_markup_msu1(cartridge["msu1"]);
}

//

void Cartridge::parse_markup_map(Mapping &m, XML::Node &map) {
  m.offset = numeral(map["offset"].data);
  m.size = numeral(map["size"].data);

  string data = map["mode"].data;
  if(data == "direct") m.mode = Bus::MapMode::Direct;
  if(data == "linear") m.mode = Bus::MapMode::Linear;
  if(data == "shadow") m.mode = Bus::MapMode::Shadow;

  lstring part;
  part.split(":", map["address"].data);
  if(part.size() != 2) return;

  lstring subpart;
  subpart.split("-", part[0]);
  if(subpart.size() == 1) {
    m.banklo = hex(subpart[0]);
    m.bankhi = m.banklo;
  } else if(subpart.size() == 2) {
    m.banklo = hex(subpart[0]);
    m.bankhi = hex(subpart[1]);
  }

  subpart.split("-", part[1]);
  if(subpart.size() == 1) {
    m.addrlo = hex(subpart[0]);
    m.addrhi = m.addrlo;
  } else if(subpart.size() == 2) {
    m.addrlo = hex(subpart[0]);
    m.addrhi = hex(subpart[1]);
  }
}

void Cartridge::parse_markup_memory(MappedRAM &ram, XML::Node &node, unsigned id, bool writable) {
  string name = node["name"].data;
  unsigned size = numeral(node["size"].data);
  ram.map(allocate<uint8>(size, 0xff), size);
  if(name.empty() == false) {
    interface->loadRequest(id, name);
    if(writable) memory.append({id, name});
  }
}

//

void Cartridge::parse_markup_cartridge(XML::Node &root) {
  if(root.exists() == false) return;

  parse_markup_memory(rom, root["rom"], ID::ROM, false);
  parse_markup_memory(ram, root["ram"], ID::RAM, true);

  for(auto &node : root) {
    if(node.name != "map") continue;

    if(node["id"].data == "rom") {
      Mapping m(rom);
      parse_markup_map(m, node);
      if(m.size == 0) m.size = rom.size();
      mapping.append(m);
    }

    if(node["id"].data == "ram") {
      Mapping m(ram);
      parse_markup_map(m, node);
      if(m.size == 0) m.size = ram.size();
      mapping.append(m);
    }
  }
}

void Cartridge::parse_markup_icd2(XML::Node &root) {
  if(root.exists() == false) return;
  has_gb_slot = true;
  icd2.revision = max(1, numeral(root["revision"].data));

  interface->loadRequest(ID::SuperGameBoy, "Game Boy", "gb");

  string firmware = root["firmware"]["name"].data;
  interface->loadRequest(ID::SuperGameBoyBootROM, firmware);
  if(!file::exists({interface->path(ID::SuperFamicom), firmware})) {
    interface->notify("Error: required firmware ", firmware, " not found.\n");
  }

  for(auto &node : root) {
    if(node.name != "map") continue;

    if(node["id"].data == "io") {
      Mapping m({&ICD2::read, &icd2}, {&ICD2::write, &icd2});
      parse_markup_map(m, node);
      mapping.append(m);
    }
  }
}

void Cartridge::parse_markup_bsx(XML::Node &root) {
  if(root.exists() == false) return;
  has_bs_cart = true;
  has_bs_slot = true;

  interface->loadRequest(ID::Satellaview, "BS-X Satellaview", "bs");

  parse_markup_memory(bsxcartridge.rom, root["rom"], ID::BsxROM, false);
  parse_markup_memory(bsxcartridge.ram, root["ram"], ID::BsxRAM, true);
  parse_markup_memory(bsxcartridge.psram, root["psram"], ID::BsxPSRAM, true);

  for(auto &node : root) {
    if(node.name != "map") continue;

    if(node["id"].data == "rom" || node["id"].data == "ram") {
      Mapping m({&BSXCartridge::mcu_read, &bsxcartridge}, {&BSXCartridge::mcu_write, &bsxcartridge});
      parse_markup_map(m, node);
      mapping.append(m);
    }

    if(node["id"].data == "io") {
      Mapping m({&BSXCartridge::mmio_read, &bsxcartridge}, {&BSXCartridge::mmio_write, &bsxcartridge});
      parse_markup_map(m, node);
      mapping.append(m);
    }
  }
}

void Cartridge::parse_markup_bsxslot(XML::Node &root) {
  if(root.exists() == false) return;
  has_bs_slot = true;

  interface->loadRequest(ID::Satellaview, "BS-X Satellaview", "bs");

  for(auto &node : root) {
    if(node.name != "map") continue;

    if(node["id"].data == "rom") {
      if(bsxflash.memory.size() == 0) continue;

      Mapping m(bsxflash.memory);
      parse_markup_map(m, node);
      mapping.append(m);
    }
  }
}

void Cartridge::parse_markup_sufamiturbo(XML::Node &root) {
  if(root.exists() == false) return;
  has_st_slots = true;

  //load required slot A (will request slot B if slot A cartridge is linkable)
  interface->loadRequest(ID::SufamiTurboSlotA, "Sufami Turbo - Slot A", "st");

  for(auto &slot : root) {
    if(slot.name != "slot") continue;
    bool slotid = slot["id"].data == "A" ? 0 : slot["id"].data == "B" ? 1 : 0;

    for(auto &node : slot) {
      if(node.name != "map") continue;

      if(node["id"].data == "rom") {
        SuperFamicom::Memory &memory = slotid == 0 ? sufamiturbo.slotA.rom : sufamiturbo.slotB.rom;
        if(memory.size() == 0) continue;

        Mapping m(memory);
        parse_markup_map(m, node);
        if(m.size == 0) m.size = memory.size();
        if(m.size) mapping.append(m);
      }

      if(node["id"].data == "ram") {
        SuperFamicom::Memory &memory = slotid == 0 ? sufamiturbo.slotA.ram : sufamiturbo.slotB.ram;
        if(memory.size() == 0) continue;

        Mapping m(memory);
        parse_markup_map(m, node);
        if(m.size == 0) m.size = memory.size();
        if(m.size) mapping.append(m);
      }
    }
  }
}

void Cartridge::parse_markup_nss(XML::Node &root) {
  if(root.exists() == false) return;
  has_nss_dip = true;
  nss.dip = interface->dipSettings(root);

  for(auto &node : root) {
    if(node.name != "map") continue;

    if(node["id"].data == "io") {
      Mapping m({&NSS::read, &nss}, {&NSS::write, &nss});
      parse_markup_map(m, node);
      mapping.append(m);
    }
  }
}

void Cartridge::parse_markup_sa1(XML::Node &root) {
  if(root.exists() == false) return;
  has_sa1 = true;

  parse_markup_memory(sa1.rom, root["rom"], ID::SA1ROM, false);
  parse_markup_memory(sa1.iram, root["iram"], ID::SA1IRAM, true);
  parse_markup_memory(sa1.bwram, root["bwram"], ID::SA1BWRAM, true);

  for(auto &node : root) {
    if(node.name != "map") continue;

    if(node["id"].data == "io") {
      Mapping m({&SA1::mmio_read, &sa1}, {&SA1::mmio_write, &sa1});
      parse_markup_map(m, node);
      mapping.append(m);
    }

    if(node["id"].data == "rom") {
      Mapping m({&SA1::mmcrom_read, &sa1}, {&SA1::mmcrom_write, &sa1});
      parse_markup_map(m, node);
      mapping.append(m);
    }

    if(node["id"].data == "iram") {
      Mapping m(sa1.cpuiram);
      parse_markup_map(m, node);
      if(m.size == 0) m.size = sa1.cpuiram.size();
      mapping.append(m);
    }

    if(node["id"].data == "bwram") {
      Mapping m({&SA1::mmcbwram_read, &sa1}, {&SA1::mmcbwram_write, &sa1});
      parse_markup_map(m, node);
      mapping.append(m);
    }
  }
}

void Cartridge::parse_markup_superfx(XML::Node &root) {
  if(root.exists() == false) return;
  has_superfx = true;

  parse_markup_memory(superfx.rom, root["rom"], ID::SuperFXROM, false);
  parse_markup_memory(superfx.ram, root["ram"], ID::SuperFXRAM, true);

  for(auto &node : root) {
    if(node.name != "map") continue;

    if(node["id"].data == "io") {
      Mapping m({&SuperFX::mmio_read, &superfx}, {&SuperFX::mmio_write, &superfx});
      parse_markup_map(m, node);
      mapping.append(m);
    }

    if(node["id"].data == "rom") {
      Mapping m(superfx.cpurom);
      parse_markup_map(m, node);
      mapping.append(m);
    }

    if(node["id"].data == "ram") {
      Mapping m(superfx.cpuram);
      parse_markup_map(m, node);
      if(m.size == 0) m.size = superfx.ram.size();
      mapping.append(m);
    }
  }
}

void Cartridge::parse_markup_armdsp(XML::Node &root) {
  if(root.exists() == false) return;
  has_armdsp = true;

  string firmware = root["firmware"]["name"].data;
  string sha256 = root["firmware"]["sha256"].data;

  interface->loadRequest(ID::ArmDSP, firmware);

  for(auto &node : root) {
    if(node.name != "map") continue;

    if(node["id"].data == "io") {
      Mapping m({&ArmDSP::mmio_read, &armdsp}, {&ArmDSP::mmio_write, &armdsp});
      parse_markup_map(m, node);
      mapping.append(m);
    }
  }
}

void Cartridge::parse_markup_hitachidsp(XML::Node &root) {
  if(root.exists() == false) return;
  has_hitachidsp = true;

  for(unsigned n = 0; n < 1024; n++) hitachidsp.dataROM[n] = 0x000000;

  hitachidsp.frequency = numeral(root["frequency"].data);
  if(hitachidsp.frequency == 0) hitachidsp.frequency = 20000000;
  string firmware = root["firmware"]["name"].data;
  string sha256 = root["firmware"]["sha256"].data;

  interface->loadRequest(ID::HitachiDSP, firmware);

  parse_markup_memory(hitachidsp.rom, root["rom"], ID::HitachiDSPROM, false);

  for(auto &node : root) {
    if(node.name != "map") continue;

    if(node["id"].data == "io") {
      Mapping m({&HitachiDSP::dsp_read, &hitachidsp}, {&HitachiDSP::dsp_write, &hitachidsp});
      parse_markup_map(m, node);
      mapping.append(m);
    }

    if(node["id"].data == "rom") {
      Mapping m({&HitachiDSP::rom_read, &hitachidsp}, {&HitachiDSP::rom_write, &hitachidsp});
      parse_markup_map(m, node);
      mapping.append(m);
    }
  }
}

void Cartridge::parse_markup_necdsp(XML::Node &root) {
  if(root.exists() == false) return;
  has_necdsp = true;

  for(unsigned n = 0; n < 16384; n++) necdsp.programROM[n] = 0x000000;
  for(unsigned n = 0; n <  2048; n++) necdsp.dataROM[n] = 0x0000;

  necdsp.frequency = numeral(root["frequency"].data);
  if(necdsp.frequency == 0) necdsp.frequency = 8000000;
  necdsp.revision
  = root["model"].data == "uPD7725"  ? NECDSP::Revision::revuPD7725
  : root["model"].data == "uPD96050" ? NECDSP::Revision::revuPD96050
  : NECDSP::Revision::revuPD7725;
  string firmware = root["firmware"]["name"].data;
  string sha256 = root["firmware"]["sha256"].data;

  if(necdsp.revision == NECDSP::Revision::revuPD7725) {
    interface->loadRequest(ID::Nec7725DSP, firmware);
  }

  if(necdsp.revision == NECDSP::Revision::revuPD96050) {
    interface->loadRequest(ID::Nec96050DSP, firmware);
    string name = root["ram"]["name"].data;
    interface->loadRequest(ID::NecDSPRAM, name);
    memory.append({ID::NecDSPRAM, name});
  }

  for(auto &node : root) {
    if(node.name != "map") continue;

    if(node["id"].data == "dr") {
      Mapping m({&NECDSP::dr_read, &necdsp}, {&NECDSP::dr_write, &necdsp});
      parse_markup_map(m, node);
      mapping.append(m);
    }

    if(node["id"].data == "sr") {
      Mapping m({&NECDSP::sr_read, &necdsp}, {&NECDSP::sr_write, &necdsp});
      parse_markup_map(m, node);
      mapping.append(m);
    }

    if(node["id"].data == "ram") {
      Mapping m({&NECDSP::dp_read, &necdsp}, {&NECDSP::dp_write, &necdsp});
      parse_markup_map(m, node);
      mapping.append(m);
    }
  }
}

void Cartridge::parse_markup_epsonrtc(XML::Node &root) {
  if(root.exists() == false) return;
  has_epsonrtc = true;

  string name = root["ram"]["name"].data;
  interface->loadRequest(ID::EpsonRTC, name);
  memory.append({ID::EpsonRTC, name});

  for(auto &node : root) {
    if(node.name != "map") continue;

    if(node["id"].data == "io") {
      Mapping m({&EpsonRTC::read, &epsonrtc}, {&EpsonRTC::write, &epsonrtc});
      parse_markup_map(m, node);
      mapping.append(m);
    }
  }
}

void Cartridge::parse_markup_sharprtc(XML::Node &root) {
  if(root.exists() == false) return;
  has_sharprtc = true;

  string name = root["ram"]["name"].data;
  interface->loadRequest(ID::SharpRTC, name);
  memory.append({ID::SharpRTC, name});

  for(auto &node : root) {
    if(node.name != "map") continue;

    if(node["id"].data == "io") {
      Mapping m({&SharpRTC::read, &sharprtc}, {&SharpRTC::write, &sharprtc});
      parse_markup_map(m, node);
      mapping.append(m);
    }
  }
}

void Cartridge::parse_markup_spc7110(XML::Node &root) {
  if(root.exists() == false) return;
  has_spc7110 = true;

  parse_markup_memory(spc7110.prom, root["prom"], ID::SPC7110PROM, false);
  parse_markup_memory(spc7110.drom, root["drom"], ID::SPC7110DROM, false);
  parse_markup_memory(spc7110.ram, root["ram"], ID::SPC7110RAM, true);

  for(auto &node : root) {
    if(node.name != "map") continue;

    if(node["id"].data == "io") {
      Mapping m({&SPC7110::read, &spc7110}, {&SPC7110::write, &spc7110});
      parse_markup_map(m, node);
      mapping.append(m);
    }

    if(node["id"].data == "rom") {
      Mapping m({&SPC7110::mcurom_read, &spc7110}, {&SPC7110::mcurom_write, &spc7110});
      parse_markup_map(m, node);
      mapping.append(m);
    }

    if(node["id"].data == "ram") {
      Mapping m({&SPC7110::mcuram_read, &spc7110}, {&SPC7110::mcuram_write, &spc7110});
      parse_markup_map(m, node);
      mapping.append(m);
    }
  }
}

void Cartridge::parse_markup_sdd1(XML::Node &root) {
  if(root.exists() == false) return;
  has_sdd1 = true;

  parse_markup_memory(sdd1.rom, root["rom"], ID::SDD1ROM, false);
  parse_markup_memory(sdd1.ram, root["ram"], ID::SDD1RAM, true);

  for(auto &node : root) {
    if(node.name != "map") continue;

    if(node["id"].data == "io") {
      Mapping m({&SDD1::read, &sdd1}, {&SDD1::write, &sdd1});
      parse_markup_map(m, node);
      mapping.append(m);
    }

    if(node["id"].data == "rom") {
      Mapping m({&SDD1::mcurom_read, &sdd1}, {&SDD1::mcurom_write, &sdd1});
      parse_markup_map(m, node);
      mapping.append(m);
    }

    if(node["id"].data == "ram") {
      Mapping m({&SDD1::mcuram_read, &sdd1}, {&SDD1::mcuram_write, &sdd1});
      parse_markup_map(m, node);
      mapping.append(m);
    }
  }
}

void Cartridge::parse_markup_obc1(XML::Node &root) {
  if(root.exists() == false) return;
  has_obc1 = true;

  parse_markup_memory(obc1.ram, root["ram"], ID::OBC1RAM, true);

  for(auto &node : root) {
    if(node.name != "map") continue;

    if(node["id"].data == "io") {
      Mapping m({&OBC1::read, &obc1}, {&OBC1::write, &obc1});
      parse_markup_map(m, node);
      mapping.append(m);
    }
  }
}

void Cartridge::parse_markup_msu1(XML::Node &root) {
  if(root.exists() == false) return;
  has_msu1 = true;

  for(auto &node : root) {
    if(node.name != "map") continue;

    if(node["id"].data == "io") {
      Mapping m({&MSU1::mmio_read, &msu1}, {&MSU1::mmio_write, &msu1});
      parse_markup_map(m, node);
      mapping.append(m);
    }
  }
}

Cartridge::Mapping::Mapping() {
  mode = Bus::MapMode::Direct;
  banklo = bankhi = addrlo = addrhi = offset = size = 0;
}

Cartridge::Mapping::Mapping(SuperFamicom::Memory &memory) {
  read = {&SuperFamicom::Memory::read, &memory};
  write = {&SuperFamicom::Memory::write, &memory};
  mode = Bus::MapMode::Direct;
  banklo = bankhi = addrlo = addrhi = offset = size = 0;
}

Cartridge::Mapping::Mapping(const function<uint8 (unsigned)> &read_, const function<void (unsigned, uint8)> &write_) {
  read = read_;
  write = write_;
  mode = Bus::MapMode::Direct;
  banklo = bankhi = addrlo = addrhi = offset = size = 0;
}

#endif
