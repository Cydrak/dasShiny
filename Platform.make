
# Architecture: x86, amd64 (default: native)
ifneq ($(filter x86,$(arch)),)
  ARCH := -m32
  BITS := 32
else ifneq ($(filter amd64,$(arch)),)
  ARCH := -m64
  BITS := 64
endif

# Build target: debug, release
ifneq ($(filter debug,$(target)),)
  CFLAGS  += -g
  LDFLAGS += -g
else
  CFLAGS  += -fomit-frame-pointer
  LDFLAGS += -s
endif

# Profiling: manual, auto, optimize
ifeq ($(profile),manual)
  CFLAGS  += -pg
  LDFLAGS += -pg -lgcov
else ifeq ($(profile),auto)
  CFLAGS  += -fprofile-generate
  LDFLAGS += -lgcov
else ifeq ($(profile),optimize)
  CFLAGS  += -fprofile-use
endif

# Autodetect host platform: win, osx, x
ifeq ($(platform),)
  uname := $(shell uname -a)
  RM     = rm -f $1
  
  ifneq ($(filter Windows Msys CYGWIN,$(uname)),)
    # Cygwin, Msys, or other POSIX-like variant
    platform := win
    
  else ifeq ($(uname),)
    # Windows command shell
    platform := win
    RM = del $(subst /,\,$1)
    
  else ifneq ($(findstring Darwin,$(uname)),)
    # Mac OS X
    platform := osx
    ruby     += video.cgl audio.openal input.carbon
    
    CC       := clang -x c -std=gnu99
    CXX      := clang++ -x c++ -std=gnu++11
    OBJC     := clang -x objective-c -std=gnu99
    OBJCXX   := clang++ -x objective-c -std=gnu++11
    LD       := clang++
    CFLAGS   += -w -stdlib=libc++
    LDFLAGS  += -lc++ -lobjc
    
  else
    # Assume Linux
    platform := x
    ruby     += video.glx
    ruby     += audio.alsa audio.openal audio.oss audio.pulseaudio audio.pulseaudiosimple audio.ao
    ruby     += input.sdl input.x
    
    CC       := gcc-4.7 -x c -std=gnu99
    CXX      := g++-4.7 -x c++ -std=gnu++11
    LD       := g++-4.7
    LDFLAGS  += -ldl -lX11 -lXext
  endif
  
  ifneq ($(filter win cygwin,$(platform)),)
    ruby     += video.wgl audio.xaudio2 input.rawinput
    
    exe       = .exe
    CC       := gcc -x c -std=gnu99
    CXX      := g++ -x c++ -std=gnu++11
    LD       := g++
    LDFLAGS  += $(if $(filter console,$(options)),-mconsole,-mwindows) \
      -mthreads -lkernel32 -luser32 -lgdi32 -lcomctl32 -lcomdlg32 -luuid \
      -lshell32 -lole32 -Wl,-enable-auto-import -Wl,-enable-runtime-pseudo-reloc
    
    # If needed, detect bitness for the resource compiler. Note, 32-bit shells
    # always have P.._ARCHITECTURE=x86, while possibly P.._ARCHITEW6432=AMD64.
    BITS ?= $(if $(filter AMD64,$(PROCESSOR_ARCHITEW6432) $(PROCESSOR_ARCHITECTURE)),64,32)
    WINRC := windres $(if $(filter 64,$(BITS)),--target=pe-x86-64,--target=pe-i386)
  endif
  
  # suppress defines in nall/Makefile
  compiler := $(CC)
endif
