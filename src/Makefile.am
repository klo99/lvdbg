CFILES =\
	main.c \
	misc.c \
	pseudo_fork.c \
	configuration.c \
	win_handler.c \
	win_form.c \
	text.c \
	input.c \
	mi2_interface.c \
	mi2_parser.c \
	objects.c \
	vsscanner.c \
	view.c

HFILES=\
	debug.h \
	misc.h \
	pseudo_fork.h \
	configuration.h \
	win_handler.h \
	win_form.h \
	text.h \
	input.h \
	mi2_interface.h \
	mi2_parser.h \
	objects.h \
	vsscanner.h \
	view.h

bin_PROGRAMS = lvdbg
lvdbg_SOURCES = $(CFILES) $(HFILES)
lvdbg_LDADD = @GCOV_LIBS@ \
              -lpanel \
              -lutil \
              -lncurses

AM_CFLAGS = @GCOV_CFLAGS@ @DEBUG_FLAGS@ \
            -Wall \
            -Werror

CLEANFILES = *.gcda *.gcno
