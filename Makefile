include nall/Makefile

fc  := fc
sfc := sfc
gb  := gb
gba := gba
nds := nds

profile := accuracy
target  := ethos

# options += console

# compiler
c       := $(compiler) -std=gnu99
cpp     := $(subst cc,++,$(compiler)) -std=gnu++0x
flags   := -I. -O3
link    := 
objects := libco

# gprof mode
# pgo := profile

# profile-guided optimization mode
# pgo := instrument
# pgo := optimize

ifneq ($(findstring debug,$(options)),)
  flags += -g
  link  += -g
else ifneq ($(pgo),profile)
  link  += -s
endif

ifneq ($(findstring x86,$(options)),)
  flags += -m32
  link  += -m32
  resfmt = --target=pe-i386
endif
ifneq ($(findstring x64,$(options)),)
  flags += -m64
  link  += -m64
  resfmt = --target=pe-x86-64
endif

ifneq ($(pgo),profile)
  flags += -fomit-frame-pointer
endif

ifeq ($(pgo),profile)
  flags += -pg
  link += -pg -lgcov
else ifeq ($(pgo),instrument)
  flags += -fprofile-generate
  link += -lgcov
else ifeq ($(pgo),optimize)
  flags += -fprofile-use
endif

# platform
ifeq ($(platform),x)
  flags += -march=native
  link += -ldl -lX11 -lXext
else ifeq ($(platform),osx)
else ifeq ($(platform),win)
  link += $(if $(findstring console,$(options)),-mconsole,-mwindows)
  link += -mthreads -luuid -lkernel32 -luser32 -lgdi32 -lcomctl32 -lcomdlg32 -lshell32 -lole32
  link += -Wl,-enable-auto-import -Wl,-enable-runtime-pseudo-reloc
else
  unknown_platform: help;
endif

ui := target-$(target)

# implicit rules
compile = \
  $(strip \
    $(if $(filter %.c,$<), \
      $(c) -MMD -MP -MF $*.d $(flags) $1 -c $< -o $@, \
      $(if $(filter %.cpp,$<), \
        $(cpp) -MMD -MP -MF $*.d $(flags) $1 -c $< -o $@ \
      ) \
    ) \
  )

%.d: ;
%.o: $<; $(call compile)

all: build;

obj/libco.o: libco/libco.c libco/*

include $(ui)/Makefile
flags := $(flags) $(foreach o,$(call strupper,$(options)),-D$o)

# targets
clean:
	-@$(call delete,obj/*.d)
	-@$(call delete,obj/*.o)
	-@$(call delete,obj/*.a)
	-@$(call delete,obj/*.so)
	-@$(call delete,obj/*.dylib)
	-@$(call delete,obj/*.dll)
	-@$(call delete,*.res)
	-@$(call delete,*.pgd)
	-@$(call delete,*.pgc)
	-@$(call delete,*.ilk)
	-@$(call delete,*.pdb)
	-@$(call delete,*.manifest)

sync:
	if [ -d ./libco ]; then rm -r ./libco; fi
	if [ -d ./nall ]; then rm -r ./nall; fi
	if [ -d ./ruby ]; then rm -r ./ruby; fi
	if [ -d ./phoenix ]; then rm -r ./phoenix; fi
	cp -r ../libco ./libco
	cp -r ../nall ./nall
	cp -r ../ruby ./ruby
	cp -r ../phoenix ./phoenix
	rm -r libco/doc
	rm -r libco/test
	rm -r nall/test
	rm -r ruby/_test
	rm -r phoenix/nall
	rm -r phoenix/test

archive:
	if [ -f dasShiny.tar.bz2 ]; then rm dasShiny.tar.bz2; fi
	tar -cjf dasShiny.tar.bz2 `ls`

help:;

-include obj/*.d
