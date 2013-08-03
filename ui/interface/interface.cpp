#include "../ui.hpp"
Interface *interface = nullptr;

void Interface::loadRequest(unsigned id, const string &name, const string &type) {
  return utility->loadRequest(id, name, type);
}

void Interface::loadRequest(unsigned id, const string &path) {
  return utility->loadRequest(id, path);
}

void Interface::saveRequest(unsigned id, const string &path) {
  return utility->saveRequest(id, path);
}

uint32_t Interface::videoColor(unsigned source, uint16_t r, uint16_t g, uint16_t b) {
  if(uiConfig->video.saturation != 100) {
    uint16_t grayscale = uclamp<16>((r + g + b) / 3);
    double saturation = uiConfig->video.saturation * 0.01;
    double inverse = max(0.0, 1.0 - saturation);
    r = uclamp<16>(r * saturation + grayscale * inverse);
    g = uclamp<16>(g * saturation + grayscale * inverse);
    b = uclamp<16>(b * saturation + grayscale * inverse);
  }

  if(uiConfig->video.gamma != 100) {
    double exponent = uiConfig->video.gamma * 0.01;
    double reciprocal = 1.0 / 32767.0;
    r = r > 32767 ? r : 32767 * pow(r * reciprocal, exponent);
    g = g > 32767 ? g : 32767 * pow(g * reciprocal, exponent);
    b = b > 32767 ? b : 32767 * pow(b * reciprocal, exponent);
  }

  if(uiConfig->video.luminance != 100) {
    double luminance = uiConfig->video.luminance * 0.01;
    r = r * luminance;
    g = g * luminance;
    b = b * luminance;
  }

  if(program->depth == 30) {
    r >>= 6, g >>= 6, b >>= 6;
    return r << 20 | g << 10 | b << 0;
  }

  if(program->depth == 24) {
    r >>= 8, g >>= 8, b >>= 8;
    return r << 16 | g << 8 | b << 0;
  }

  return 0u;
}

void Interface::videoRefresh(const uint32_t *data, unsigned pitch, unsigned width, unsigned height) {
  uint32_t *output;
  unsigned outputPitch;

  if(video.lock(output, outputPitch, width, height)) {
    pitch >>= 2, outputPitch >>= 2;

    for(unsigned y = 0; y < height; y++)
      memcpy(output + y * outputPitch, data + y * pitch, 4 * width);

    video.unlock();
    video.refresh();
  }

  static unsigned frameCounter = 0;
  static time_t previous, current;
  frameCounter++;

  time(&current);
  if(current != previous) {
    previous = current;
    //utility->setStatusText({"FPS: ", frameCounter});
    frameCounter = 0;
  }
}

void Interface::audioSample(int16_t lsample, int16_t rsample) {
  signed samples[] = {lsample, rsample};
  dspaudio.sample(samples);
  while(dspaudio.pending()) {
    dspaudio.read(samples);
    audio.sample(samples[0], samples[1]);
  }
}

int16_t Interface::inputPoll(unsigned port, unsigned device, unsigned input) {
  unsigned guid = system().port[port].device[device].input[input].guid;
  return inputManager->inputMap[guid]->poll();
}

unsigned Interface::dipSettings(const Markup::Node &node) {
  return 0;
}

string Interface::path(unsigned group) {
  return utility->pathRequest(group);
}

string Interface::server() {
  return "";
}

void Interface::notify(const string &text) {
  MessageWindow().setParent(*presentation).setText(text).information();
}
