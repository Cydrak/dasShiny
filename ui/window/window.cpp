#include "../ui.hpp"
WindowManager *windowManager = nullptr;

void WindowManager::append(Window *window, const string &name) {
  window->setMenuFont(program->normalFont);
  window->setWidgetFont(program->normalFont);
  window->setStatusFont(program->boldFont);
  windowList.append({window, name, window->geometry().text()});
}

void WindowManager::loadGeometry() {
  Configuration::Node node;
  for(auto &window : windowList)
    node.append(window.geometry, window.name);
  
  config.append(node, "Geometry");
  config.load(program->loadPath("geometry.bml"));
  config.save(program->savePath("geometry.bml"));
  for(auto &window : windowList) {
    window.window->setGeometry(window.geometry);
  }
}

void WindowManager::saveGeometry() {
  for(auto &window : windowList) {
    window.geometry = window.window->geometry().text();
  }
  config.save(program->savePath("geometry.bml"));
}

void WindowManager::hideAll() {
  for(auto &window : windowList) {
    window.window->setVisible(false);
  }
}
