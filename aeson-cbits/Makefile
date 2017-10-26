ARTIFACT=unescape_string

TARGET:=$(ARTIFACT)

ifdef DEST_TOO_LONG
BUGS+=-DBUG_DEST_TOO_SMALL
TARGET:=$(TARGET).DTL
endif

build:

include ../common.mk
