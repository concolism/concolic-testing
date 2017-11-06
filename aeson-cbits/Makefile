ARTIFACT=unescape_string

TARGET:=$(ARTIFACT)

ifdef DEST_TOO_LONG
BUGS+=-DBUG_DEST_TOO_SMALL
TARGET:=$(TARGET).DTL
endif

ifdef ASCII_SOURCE
BUGS+=-DASCII_SOURCE # Not actually a bug...
TARGET:=$(TARGET).AS
endif

build:

include ../common.mk
