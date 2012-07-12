#include "../base.hpp"
CPUDebugger *cpuDebugger = nullptr;

#include "registers.cpp"

uint24 CPUDebugger::mirror(uint24 addr) {
  if((addr & 0x40e000) == 0x0000) addr = 0x7e0000 | (addr & 0x1fff);  //$00-3f:80-bf:0000-1fff WRAM
  return addr;
}

uint8 CPUDebugger::read(uint24 addr) {
  if((addr & 0x40e000) == 0x2000) return ~0;  //$00-3f|80-bf:2000-3fff  MMIO
  if((addr & 0x40e000) == 0x4000) return ~0;  //$00-3f|80-bf:4000-5fff  MMIO
  return SFC::bus.read(mirror(addr));
}

void CPUDebugger::write(uint24 addr, uint8 data) {
  if((addr & 0x40e000) == 0x2000) return;  //$00-3f|80-bf:2000-3fff  MMIO
  if((addr & 0x40e000) == 0x4000) return;  //$00-3f|80-bf:4000-5fff  MMIO
  if((addr & 0x40e000) == 0x0000) addr = 0x7e0000 | (addr & 0x1fff);  //$00-3f:80-bf:0000-1fff WRAM
  return SFC::bus.write(mirror(addr), data);
}

unsigned CPUDebugger::opcodeLength(uint24 addr) {
  #define M 5
  #define X 6
  static unsigned lengthTable[256] = {
    2, 2, 2, 2,  2, 2, 2, 2,  1, M, 1, 1,  3, 3, 3, 4,
    2, 2, 2, 2,  2, 2, 2, 2,  1, 3, 1, 1,  3, 3, 3, 4,
    3, 2, 4, 2,  2, 2, 2, 2,  1, M, 1, 1,  3, 3, 3, 4,
    2, 2, 2, 2,  2, 2, 2, 2,  1, 3, 1, 1,  3, 3, 3, 4,

    1, 2, 2, 2,  3, 2, 2, 2,  1, M, 1, 1,  3, 3, 3, 4,
    2, 2, 2, 2,  3, 2, 2, 2,  1, 3, 1, 1,  4, 3, 3, 4,
    1, 2, 3, 2,  2, 2, 2, 2,  1, M, 1, 1,  3, 3, 3, 4,
    2, 2, 2, 2,  2, 2, 2, 2,  1, 3, 1, 1,  3, 3, 3, 4,

    2, 2, 3, 2,  2, 2, 2, 2,  1, M, 1, 1,  3, 3, 3, 4,
    2, 2, 2, 2,  2, 2, 2, 2,  1, 3, 1, 1,  3, 3, 3, 4,
    X, 2, X, 2,  2, 2, 2, 2,  1, M, 1, 1,  3, 3, 3, 4,
    2, 2, 2, 2,  2, 2, 2, 2,  1, 3, 1, 1,  3, 3, 3, 4,

    X, 2, 2, 2,  2, 2, 2, 2,  1, M, 1, 1,  3, 3, 3, 4,
    2, 2, 2, 2,  2, 2, 2, 2,  1, 3, 1, 1,  3, 3, 3, 4,
    X, 2, 2, 2,  2, 2, 2, 2,  1, M, 1, 1,  3, 3, 3, 4,
    2, 2, 2, 2,  3, 2, 2, 2,  1, 3, 1, 1,  3, 3, 3, 4,
  };

  unsigned length = lengthTable[SFC::bus.read(addr)];
  if(length == M) return 3 - (SFC::cpu.regs.e | SFC::cpu.regs.p.m);
  if(length == X) return 3 - (SFC::cpu.regs.e | SFC::cpu.regs.p.x);
  return length;
  #undef M
  #undef X
}

void CPUDebugger::updateDisassembly() {
  string line[15];
  char text[512];

  SFC::cpu.disassemble_opcode(text, opcodePC);
  text[29] = 0;
  line[7] = { "> ", text };

  signed addr = opcodePC;
  for(signed o = 6; o >= 0; o--) {
    for(signed b = 1; b <= 4; b++) {
      if(addr - b >= 0 && (debugger->cpuUsage.data[addr - b] & Usage::Exec)) {
        addr -= b;
        SFC::cpu.disassemble_opcode(text, addr);
        text[29] = 0;
        line[o] = { "  ", text };
        break;
      }
    }
  }

  addr = opcodePC;
  for(signed o = 8; o <= 14; o++) {
    for(signed b = 1; b <= 4; b++) {
      if(addr + b <= 0xffffff && (debugger->cpuUsage.data[addr + b] & Usage::Exec)) {
        addr += b;
        SFC::cpu.disassemble_opcode(text, addr);
        text[29] = 0;
        line[o] = { "  ", text };
        break;
      }
    }
  }

  string output;
  for(auto &n : line) {
    if(n.empty()) output.append("  ...\n");
    else output.append(n, "\n");
  }
  output.rtrim<1>("\n");

  disassembly.setText(output);
  registers.setText({
     "A:", hex<4>(SFC::cpu.regs.a), " X:", hex<4>(SFC::cpu.regs.x), " Y:", hex<4>(SFC::cpu.regs.y),
    " S:", hex<4>(SFC::cpu.regs.s), " D:", hex<4>(SFC::cpu.regs.d), " DB:", hex<2>(SFC::cpu.regs.db), " ",
    SFC::cpu.regs.p.n ? "N" : "n", SFC::cpu.regs.p.v ? "V" : "v",
    SFC::cpu.regs.e ? (SFC::cpu.regs.p.m ? "1" : "0") : (SFC::cpu.regs.p.m ? "M" : "m"),
    SFC::cpu.regs.e ? (SFC::cpu.regs.p.x ? "B" : "b") : (SFC::cpu.regs.p.x ? "X" : "x"),
    SFC::cpu.regs.p.d ? "D" : "d", SFC::cpu.regs.p.i ? "I" : "i",
    SFC::cpu.regs.p.z ? "Z" : "z", SFC::cpu.regs.p.c ? "C" : "c",
  });
}

CPUDebugger::CPUDebugger() {
  opcodePC = 0x008000;

  setTitle("CPU Debugger");
  setGeometry({128, 128, 350, 255});

  layout.setMargin(5);
  stepInto.setText("Step Into");
  stepNMI.setText("NMI");
  stepIRQ.setText("IRQ");
  autoUpdate.setText("Auto");
  update.setText("Update");
  disassembly.setFont(application->monospaceFont);
  registers.setFont(application->monospaceFont);
  registers.setText(" ");

  layout.append(controlLayout, {~0, 0}, 5);
    controlLayout.append(stepInto, {80, 0}, 5);
    controlLayout.append(stepNMI, {40, 0}, 5);
    controlLayout.append(stepIRQ, {40, 0}, 5);
    controlLayout.append(spacer, {~0, 0});
    controlLayout.append(autoUpdate, {0, 0}, 5);
    controlLayout.append(update, {80, 0});
  layout.append(disassembly, {~0, ~0}, 5);
  layout.append(registers, {~0, 0});
  append(layout);

  stepInto.onActivate = [&] {
    debugger->flags.cpu.stepInto = true;
    debugger->resume();
  };

  stepNMI.onActivate = [&] {
    debugger->flags.cpu.nmi = true;
    debugger->resume();
  };

  stepIRQ.onActivate = [&] {
    debugger->flags.cpu.irq = true;
    debugger->resume();
  };

  update.onActivate = { &CPUDebugger::updateDisassembly, this };

  registers.onActivate = [&] {
    cpuRegisterEditor->loadRegisters();
    cpuRegisterEditor->setVisible();
  };

  windowManager->append(this, "CPUDebugger");
}
