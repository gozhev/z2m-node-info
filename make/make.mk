ifndef _MAKE_MK
_MAKE_MK :=

_IMPORT_PREFIX := $(dir $(lastword $(MAKEFILE_LIST)))
_Import = $(eval include $(_IMPORT_PREFIX)$1.mk)
_ImportAll = $(eval $(foreach x,$(wildcard $(1)*.mk),$(NL)include $(x)))

.PHONY: all
all:

$(call _ImportAll,$(_IMPORT_PREFIX)/blocks/)

endif
