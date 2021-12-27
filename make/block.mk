ifndef _BLOCK_MK
_BLOCK_MK :=

$(call _Import,utility)

define _Before_block
_VARS := Before After name
$(call _UndefineVars,$(_VARS))
_BLOCK := block
endef

define _After_block
define _Before_$(name)
$(call _UndefineVars,$(vars))
_BLOCK := $(name)
$(value Before)
endef
define _After_$(name)
$(value After)
$(call _UndefineVars,$(vars))
endef
define $(name)
$$(eval $$(_Before_$(name)))
endef
$(call _UndefineVars,$(_VARS))
endef

block = $(eval $(_Before_block))
; = $(eval $(_After_$(_BLOCK)))

endif
