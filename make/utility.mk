ifndef _UTILITY_MK
_UTILITY_MK :=

define NL


endef

define _UndefineVars
$(foreach x,$(1),$(NL)undefine $(x))
endef

endif
