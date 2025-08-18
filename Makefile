lib.name = pddp
class.sources = \
	helplink.c \
	pddplink.c \
	$(empty)

datafiles = \
	$(wildcard *.pd) \
	$(wildcard *.txt) \
	$(wildcard *.md) \
	pddplink.tcl \
	$(empty)

datadirs = \
	examples \
	$(empty)

PDLIBBUILDER_DIR=pd-lib-builder
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder
