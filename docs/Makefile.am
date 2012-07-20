info_TEXINFOS = lvdbg.texi
lvdbg_TEXINFOS = fdl.texi

examples = examples/Makefile.am \
           examples/example_1.c \
           examples/example_2.c \
           examples/example_3.c \
           examples/example_4.c \
           examples/example_5_main.c \
           examples/example_5_bar.c \
           examples/example_5_bar.h \
           examples/example_6.c

EXTRA_DIST = $(examples)

exdir = $(datadir)/doc/$(PACKAGE)

examplesdir = $(exdir)/examples
examples_DATA = $(examples)

devel_docs:
	doxygen
