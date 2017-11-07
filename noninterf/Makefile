ARTIFACT=noninterf

TARGET:=$(ARTIFACT)

ifdef BUG_ADD
BUG+=-DBUG_ADD_TAG
TARGET:=$(TARGET).ADD
endif

build:

include ../common.mk
