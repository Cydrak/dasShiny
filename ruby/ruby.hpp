/*
  ruby
  version: 0.08 (2011-11-25)
  license: public domain
*/

#ifndef RUBY_H
#define RUBY_H

#include <nall/platform.hpp>
#include <nall/algorithm.hpp>
#include <nall/any.hpp>
#include <nall/bit.hpp>
#include <nall/input.hpp>
#include <nall/intrinsics.hpp>
#include <nall/sort.hpp>
#include <nall/stdint.hpp>
#include <nall/string.hpp>
#include <nall/vector.hpp>

namespace ruby {

#include <ruby/video.hpp>
#include <ruby/audio.hpp>
#include <ruby/input.hpp>

struct VideoInterface {
  void driver(const char *driver = "");
  const char* default_driver();
  const char* driver_list();
  bool init();
  void term();

  bool cap(const nall::string& name);
  nall::any get(const nall::string& name);
  bool set(const nall::string& name, const nall::any& value);

  bool lock(uint32_t *&data, unsigned &pitch, unsigned width, unsigned height);
  void unlock();
  void clear();
  void refresh();
  VideoInterface();
  ~VideoInterface();

private:
  Video *p;
};

struct AudioInterface {
  void driver(const char *driver = "");
  const char* default_driver();
  const char* driver_list();
  bool init();
  void term();

  bool cap(const nall::string& name);
  nall::any get(const nall::string& name);
  bool set(const nall::string& name, const nall::any& value);

  void sample(uint16_t left, uint16_t right);
  void clear();
  AudioInterface();
  ~AudioInterface();

private:
  Audio *p;
};

struct InputInterface {
  void driver(const char *driver = "");
  const char* default_driver();
  const char* driver_list();
  bool init();
  void term();

  bool cap(const nall::string& name);
  nall::any get(const nall::string& name);
  bool set(const nall::string& name, const nall::any& value);

  bool acquire();
  bool unacquire();
  bool acquired();

  bool poll(int16_t *table);
  InputInterface();
  ~InputInterface();

private:
  Input *p;
};

extern VideoInterface video;
extern AudioInterface audio;
extern InputInterface input;

};

#endif
