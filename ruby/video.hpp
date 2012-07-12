struct Video {
  static const char *Handle;
  static const char *Synchronize;
  static const char *Depth;
  static const char *Filter;
  static const char *Shader;
  static const char *FragmentShader;
  static const char *VertexShader;

  enum Filter {
    FilterPoint,
    FilterLinear,
  };

  virtual bool cap(const nall::string& name) { return false; }
  virtual nall::any get(const nall::string& name) { return false; }
  virtual bool set(const nall::string& name, const nall::any& value) { return false; }

  virtual bool lock(uint32_t *&data, unsigned &pitch, unsigned width, unsigned height) { return false; }
  virtual void unlock() {}

  virtual void clear() {}
  virtual void refresh() {}
  virtual bool init() { return true; }
  virtual void term() {}

  Video() {}
  virtual ~Video() {}
};
