ARTIFACT=noninterf

TARGET:=$(ARTIFACT)

ifdef RANDOMIZE
BUGS+=-DRANDOMIZE
TARGET:=$(TARGET).RAND
endif

ifdef BUG_ADD
BUGS+=-DBUG_ADD_TAG
TARGET:=$(TARGET).ADD
endif

ifdef BUG_STORE
BUGS+=-DBUG_STORE_TAG
TARGET:=$(TARGET).STORE
endif

ifdef BUG_STORE_2
BUGS+=-DBUG_STORE_TAG_2
TARGET:=$(TARGET).STORE2
endif

ifdef BUG_LOAD
BUGS+=-DBUG_LOAD_TAG
TARGET:=$(TARGET).LOAD
endif

build:

include ../common.mk

random: $(TARGET).rand

$(TARGET).rand: $(ARTIFACT).c buildanyway
	$(CC) -Wall -DRANDOM $(CC_EXTRA_OPTS) $(BUGS) $< -o $@
