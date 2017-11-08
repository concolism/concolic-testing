ARTIFACT=noninterf

TARGET:=$(ARTIFACT)

ifdef BUG_ADD
BUGS+=-DBUG_ADD_TAG
TARGET:=$(TARGET).ADD
endif

build:

include ../common.mk
