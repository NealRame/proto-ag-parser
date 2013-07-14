export TARGET   = proto-ag-parser
export CC       = g++
export CFLAGS   = -std=c++11 -Wall -Werror
export CPPFLAGS = -std=c++11 -Wall -Werror
export SOURCES := $(wildcard $(PWD)/sources/*.cpp)
export OBJECTS := $(patsubst $(PWD)/sources/%.cpp,%.o,$(SOURCES))
export VPATH   := $(PWD)/sources
export DEPS    := $(PWD)/Makefile.depends

.PHONY: all clean debug depends realclean release tags

all: debug release

debug:
	mkdir -p Debug
	$(MAKE) --no-print-directory -C Debug   -f ../Makefile.$@ $(TARGET)

release:
	mkdir -p Release
	$(MAKE) --no-print-directory -C Release -f ../Makefile.$@ $(TARGET)

tags:
	$(CC) -M $(INCLUDE_DIRECTORIES) $(SOURCES) \
		| sed -e 's/[\\ ]/\n/g' \
		| sed -e '/^$$/d' -e '/\.o:[ \t]*$$/d' \
		| ctags -L - --c++-kinds=+p --fields=+iaS --extra=+q

depends: $(SOURCES)
	$(CC) $(INCLUDE_DIRECTORIES) -MM $(SOURCES) > $(DEPS)

realclean: clean
	rm -fr log
	rm -fr tags
	rm -fr Debug
	rm -fr Release

clean:
	rm -fr *~
	rm -fr $(DEPS)
