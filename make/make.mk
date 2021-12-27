ifndef _MAKE_MK
_MAKE_MK :=

_IMPORT_PREFIX := $(dir $(lastword $(MAKEFILE_LIST)))
_Import = $(eval include $(_IMPORT_PREFIX)$1.mk)
_ImportAll = $(eval $(foreach x,$(wildcard $(1)*.mk),$(NL)include $(x)))

_MODULE := /
. = $(_MODULE)

_GetVar = $(if $(2),$(if $($(2)$(1)),$($(2)$(1)),$(call _GetVar,$(1),$(dir $(patsubst %/,%,$(2))))))
get = $(call _GetVar,$(1),$(_MODULE))

$(call _ImportAll,$(_IMPORT_PREFIX)/blocks/)
$(call _Import,globals)

.PHONY: all
all:

.PHONY: clean
clean:

endif
