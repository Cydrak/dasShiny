.SUFFIXES:
name     = dasShiny
ARCH     = -march=native
CFLAGS   =  -I. -O3
CXXFLAGS =
LDFLAGS  = -Wl,-Map,build/$(name).map

include Platform.make
include nall/Makefile
include phoenix/Makefile
include ruby/Makefile

add = $(foreach m,$2,$(eval $(call addmodule,$1,$(m))))
rule = $(if $(wildcard $2),$2; @echo Building $1;$3)

define addmodule
  objects += build/$(subst /,-,$1)$2.o
  
  # double $$ defers evaluation so target-specific variables will work
  build/$(subst /,-,$1)$2.o: \
    $(call rule, $1$2, $1$2/$2.c,   $$(CC)  $$(ARCH) $$(CFLAGS) -MMD -MP -MF build/$(subst /,-,$1)$2.d -c $$< -o $$@) \
    $(call rule, $1$2, $1$2/$2.cpp, $$(CXX) $$(ARCH) $$(CFLAGS) $$(CXXFLAGS) -MMD -MP -MF build/$(subst /,-,$1)$2.d -c $$< -o $$@)
endef

all: build $(name)$(exe)
build:; @mkdir $@

ifneq ($(filter osx,$(platform)),)
  # Objective-C++ for ruby/phoenix Cocoa ports
  build/phoenix.o: CXX := $(OBJCXX)
  build/ruby.o:    CXX := $(OBJCXX)
endif
build/phoenix.o: CXXFLAGS += $(phoenixflags)
build/ruby.o:    CXXFLAGS += $(rubyflags)

# Embedded resources (note: both zip and ld are path-sensitive here).
build/data-resources.o: $(shell find data/resources/ -iname '*' | sed 's/ /\\ /g')
	@echo Packing $(notdir $(basename $@))
	@cd data/resources; zip -9qr ../../build/resources.zip .
	@cd build; ld -r -b binary -o $(notdir $@) resources.zip

# Windows icon + manifest
build/win-resources.o: ui/resource.rc data/dasShiny.manifest data/dasShiny.ico
	@echo Building $(notdir $(basename $@))
	@$(WINRC) $< $@

clean:
	-@$(call RM,build/*)
	-@rmdir build

$(call add,nds/, apu cpu gpu interface memory ppu slot1 slot2 system utility video wifi)
$(call add,ui/,  configuration general input interface settings utility window)
$(call add,,     ui phoenix ruby libco)

objects += $(if $(filter win,$(platform)),build/win-resources.o)

$(name)$(exe): $(objects) build/data-resources.o
	@echo Linking $(name)
	@$(LD) $(ARCH) -o $@ $^ $(LDFLAGS) $(phoenixlink) $(rubylink)

%.d:;
-include build/*.d
