SUBDIRS=aeson-cbits noninterf

all: $(SUBDIRS)

$(SUBDIRS):
	make -C $@

.PHONY: all $(SUBDIRS)
