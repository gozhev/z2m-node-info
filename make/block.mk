ifndef _BLOCK_MK
_BLOCK_MK :=

$(call _Import,utility)

define _PreBefore_block
_BLOCK := block
_VARS := \
	name \
	vars \
	pre-before \
	before \
	pre-after \
	after
endef

define _Before_block
$(foreach x,$(_VARS),$(NL)undefine $(x))
endef

define _PreAfter_block
endef

define _After_block
define _PreBefore_$(name)
$(foreach x,$(vars),$(NL)undefine $(x))
_BLOCK := $(name)
$(value pre-before)
endef
define _Before_$(name)
$(value before)
endef
define _PreAfter_$(name)
$(value pre-after)
endef
define _After_$(name)
$(value after)
$(foreach x,$(vars),$(NL)undefine $(x))
endef
define $(name)
$$(eval $$(value _PreBefore_$(name)))
$$(eval $$(_Before_$(name)))
endef
$(foreach x,$(_VARS),$(NL)undefine $(x))
endef

define block
$(eval $(value _PreBefore_block))
$(eval $(_Before_block))
endef

define ;
$(eval $(value _PreAfter_$(_BLOCK)))
$(eval $(_After_$(_BLOCK)))
endef

endif
