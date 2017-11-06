ARTIFACT=unescape_string

TARGET:=$(ARTIFACT)

ifdef DEST_TOO_SMALL
BUGS+=-DBUG_DEST_TOO_SMALL
TARGET:=$(TARGET).DTS
endif

ifdef DEST_TOO_SMALL_BIS
BUGS+=-DBUG_DEST_TOO_SMALL_BIS
TARGET:=$(TARGET).DTS2
endif

ifdef ASCII_SOURCE
BUGS+=-DASCII_SOURCE # Not actually a bug...
TARGET:=$(TARGET).AS
endif

build:

include ../common.mk
