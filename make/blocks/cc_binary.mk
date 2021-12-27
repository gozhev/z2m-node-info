ifndef _CC_BINARY_MK
_CC_BINARY_MK :=

$(call _Import,block)

$(block)
name = cc_binary
vars = \
	name \
	srcs \
	cxx \
	cxxflags
pre-before=

define before=
cxx = $(call get,cxx)
cxxflags = $(call get,cxxflags)
endef

define pre-after=
_BUILD_PREFIX := $(call get,build_dir)
_TARGET := $(_BUILD_PREFIX)$(name)
_OBJS := $(srcs:%=$(_BUILD_PREFIX)%.o)
endef

define after=
$(_TARGET): $(_OBJS) | $(_BUILD_PREFIX)
	$(cxx) -o $$@ $(cxxflags) $$^

$(_OBJS): $(_BUILD_PREFIX)%.cc.o: %.cc | $(_BUILD_PREFIX)
	$(cxx) -c -o $$@ $(cxxflags) $$^

$(_BUILD_PREFIX):
	mkdir -p $$@

.PHONY: clean/$(name)
clean/$(name):
	rm -f $(_OBJS) |:
	rm -f $(_TARGET) |:
	rmdir -p $(_BUILD_PREFIX) |:

all: $(_TARGET)
clean: clean/$(name)

endef
$;

endif
