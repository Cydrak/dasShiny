//S-CPU
//=====

void Debugger::cpu_op_exec(uint24 addr) {
  cpuUsage.data[addr] |= Usage::Exec;
  cpuDebugger->opcodePC = addr;
  bool breakpointHit = breakpointEditor->testExecCPU(addr);

  if((debug.cpu && tracer->enabled() && !tracer->maskCPU(addr))
  || (debug.cpu && flags.step)
  || flags.cpu.stepInto
  || breakpointHit
  ) {
    char text[512];
    SFC::cpu.disassemble_opcode(text, addr);

    if(debug.cpu && tracer->enabled()) tracer->print(text, "\n");
    if((debug.cpu && flags.step) || flags.cpu.stepInto || breakpointHit) {
      print(text, "\n");
      if(debug.cpu && flags.step) {
        consoleWindow->stepButton.setFocused();
      }
      if(flags.cpu.stepInto) {
        cpuDebugger->stepInto.setFocused();
        cpuDebugger->updateDisassembly();
      }
      suspend();
      SFC::scheduler.debug();
    }
  }
}

void Debugger::cpu_op_read(uint24 addr) {
  cpuUsage.data[addr] |= Usage::Read;
  bool breakpointHit = breakpointEditor->testReadCPU(addr);

  if(breakpointHit) {
    char text[512];
    SFC::cpu.disassemble_opcode(text, cpuDebugger->opcodePC);
    print(text, "\n");

    suspend();
    SFC::scheduler.debug();
  }
}

void Debugger::cpu_op_write(uint24 addr, uint8 data) {
  cpuUsage.data[addr] |= Usage::Write;
  bool breakpointHit = breakpointEditor->testWriteCPU(addr, data);

  if(breakpointHit) {
    char text[512];
    SFC::cpu.disassemble_opcode(text, cpuDebugger->opcodePC);
    print(text, "\n");

    suspend();
    SFC::scheduler.debug();
  }
}

void Debugger::cpu_op_nmi() {
  if(flags.cpu.nmi) {
    print("CPU NMI\n");  //, text, "\n");
    flags.cpu.stepInto = true;
  }
}

void Debugger::cpu_op_irq() {
  if(flags.cpu.irq) {
    print("CPU IRQ\n");
    flags.cpu.stepInto = true;
  }
}

//S-SMP
//=====

void Debugger::smp_op_exec(uint16 addr) {
  apuUsage.data[addr] |= Usage::Exec;
  smpDebugger->opcodePC = addr;
  bool breakpointHit = breakpointEditor->testExecSMP(addr);

  if((debug.cpu && tracer->enabled() && !tracer->maskSMP(addr))
  || (debug.smp && flags.step)
  || flags.smp.stepInto
  || breakpointHit
  ) {
    string text = SFC::smp.disassemble_opcode(addr);

    if(debug.smp && tracer->enabled()) tracer->print(text, "\n");
    if((debug.smp && flags.step) || flags.smp.stepInto || breakpointHit) {
      print(text, "\n");
      if(debug.smp && flags.step) {
        consoleWindow->stepButton.setFocused();
      }
      if(flags.smp.stepInto) {
        smpDebugger->stepInto.setFocused();
        smpDebugger->updateDisassembly();
      }
      suspend();
      SFC::scheduler.debug();
    }
  }
}

void Debugger::smp_op_read(uint16 addr) {
  apuUsage.data[addr] |= Usage::Read;
  bool breakpointHit = breakpointEditor->testReadSMP(addr);

  if(breakpointHit) {
    print(SFC::smp.disassemble_opcode(smpDebugger->opcodePC), "\n");
    suspend();
    SFC::scheduler.debug();
  }
}

void Debugger::smp_op_write(uint16 addr, uint8 data) {
  apuUsage.data[addr] |= Usage::Write;
  bool breakpointHit = breakpointEditor->testWriteSMP(addr, data);

  if(breakpointHit) {
    print(SFC::smp.disassemble_opcode(smpDebugger->opcodePC), "\n");
    suspend();
    SFC::scheduler.debug();
  }
}

//S-PPU
//=====

void Debugger::ppu_vram_read(uint16 addr) {
  bool breakpointHit = breakpointEditor->testReadVRAM(addr);

  if(breakpointHit) {
    char text[512];
    SFC::cpu.disassemble_opcode(text, cpuDebugger->opcodePC);
    print(text, "\n");

    suspend();
    SFC::scheduler.debug();
  }
}

void Debugger::ppu_vram_write(uint16 addr, uint8 data) {
  bool breakpointHit = breakpointEditor->testWriteVRAM(addr, data);

  if(breakpointHit) {
    char text[512];
    SFC::cpu.disassemble_opcode(text, cpuDebugger->opcodePC);
    print(text, "\n");

    suspend();
    SFC::scheduler.debug();
  }
}

void Debugger::ppu_oam_read(uint16 addr) {
  bool breakpointHit = breakpointEditor->testReadOAM(addr);

  if(breakpointHit) {
    char text[512];
    SFC::cpu.disassemble_opcode(text, cpuDebugger->opcodePC);
    print(text, "\n");

    suspend();
    SFC::scheduler.debug();
  }
}

void Debugger::ppu_oam_write(uint16 addr, uint8 data) {
  bool breakpointHit = breakpointEditor->testWriteOAM(addr, data);

  if(breakpointHit) {
    char text[512];
    SFC::cpu.disassemble_opcode(text, cpuDebugger->opcodePC);
    print(text, "\n");

    suspend();
    SFC::scheduler.debug();
  }
}

void Debugger::ppu_cgram_read(uint16 addr) {
  bool breakpointHit = breakpointEditor->testReadCGRAM(addr);

  if(breakpointHit) {
    char text[512];
    SFC::cpu.disassemble_opcode(text, cpuDebugger->opcodePC);
    print(text, "\n");

    suspend();
    SFC::scheduler.debug();
  }
}

void Debugger::ppu_cgram_write(uint16 addr, uint8 data) {
  bool breakpointHit = breakpointEditor->testWriteCGRAM(addr, data);

  if(breakpointHit) {
    char text[512];
    SFC::cpu.disassemble_opcode(text, cpuDebugger->opcodePC);
    print(text, "\n");

    suspend();
    SFC::scheduler.debug();
  }
}
