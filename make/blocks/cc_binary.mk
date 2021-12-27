ifndef _BINARY_MK
_BINARY_MK :=

$(call _Import,block)

$(block)
name = cc_binary
vars = \
	name \
	srcs \
	cxx \
	cxxflags

define After
all: $(name)

_OBJS := $(srcs:%=%.o)

$(name): $$(_OBJS)
	$(cxx) -o $$@ $(cxxflags) $$^

$$(_OBJS): %.cc.o: %.cc
	$(cxx) -c -o $$@ $(cxxflags) $$^

endef
$;

endif
