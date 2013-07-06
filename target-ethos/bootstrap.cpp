#include <nds/interface/interface.hpp>

void Application::bootstrap() {
  interface = new Interface;

  emulator.append(new NintendoDS::Interface);

  for(auto &system : emulator) system->bind = interface;
}
