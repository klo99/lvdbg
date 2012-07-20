TESTS = check_configuration \
        check_vsscanner \
        check_input \
        check_text \
        check_objects \
        check_forkpty \
        check_misc \
        check_win_handler \
        check_view \
        check_mi2_interface \
        check_mi2_parser

check_PROGRAMS = check_configuration \
                 check_vsscanner \
                 check_input \
                 check_text \
                 check_objects \
                 check_forkpty \
                 check_misc \
                 check_win_handler \
                 check_view \
                 check_mi2_interface \
                 check_mi2_parser

LDADD =

VALGRIND=valgrind
VALGRIND_OPT=--leak-check=full    \
             --show-reachable=yes \
             -v --track-fds=yes   \
             --trace-children=yes \
             --suppressions=valgrind_supp.txt

AM_CPPFLAGS = -DCONFDIR="\"$(srcdir)/\"" @GCOV_CFLAGS@
AM_CHECK_CFLAGS=-g -O0

check_configuration_SOURCES = check_configuration.c \
			      test.conf \
			      test_bad11.conf
check_configuration_CFLAGS = @CHECK_CFLAGS@ \
                             @GCOV_CFLAGS@
check_configuration_LDADD = @CHECK_LIBS@ \
                            @GCOV_LIBS@ \
                            $(top_builddir)/src/configuration.o

check_vsscanner_SOURCES = check_vsscanner.c
check_vsscanner_CFLAGS = @CHECK_CFLAGS@ \
                     @GCOV_CFLAGS@
check_vsscanner_LDADD = @CHECK_LIBS@ \
                    @GCOV_LIBS@ \
                    $(top_builddir)/src/vsscanner.o

check_input_SOURCES = check_input.c
check_input_CFLAGS = @CHECK_CFLAGS@ \
                     @GCOV_CFLAGS@
check_input_LDADD = @CHECK_LIBS@ \
                    @GCOV_LIBS@ \
                    $(top_builddir)/src/input.o

check_text_SOURCES = check_text.c \
		     text_test.txt
check_text_CFLAGS = @CHECK_CFLAGS@ \
                    @GCOV_CFLAGS@
check_text_LDADD = @CHECK_LIBS@ \
                   @GCOV_LIBS@ \
                   $(top_builddir)/src/text.o

check_objects_SOURCES = check_objects.c
check_objects_CFLAGS = @CHECK_CFLAGS@ \
                      @GCOV_CFLAGS@
check_objects_LDADD = @CHECK_LIBS@ \
                      @GCOV_LIBS@ \
                      $(top_builddir)/src/objects.o

check_forkpty_SOURCES = check_forkpty.c
check_forkpty_CFLAGS = @CHECK_CFLAGS@ \
                       @GCOV_CFLAGS@
check_forkpty_LDADD = @CHECK_LIBS@ \
                      @GCOV_LIBS@ \
                      -lutil \
                      $(top_builddir)/src/pseudo_fork.o

check_misc_SOURCES = check_misc.c
check_misc_CFLAGS = @CHECK_CFLAGS@ \
                    @GCOV_CFLAGS@
check_misc_LDADD = @CHECK_LIBS@ \
                   @GCOV_LIBS@ \
                   $(top_builddir)/src/misc.o

check_win_handler_SOURCES = check_win_handler.c \
			    test.conf \
			    test_bad1.conf \
			    test_bad10.conf \
			    test_bad11.conf \
			    test_bad12.conf \
			    test_bad13.conf \
			    test_bad14.conf \
			    test_bad2.conf \
			    test_bad3.conf \
			    test_bad4.conf \
			    test_bad5.conf \
			    test_bad6.conf \
			    test_bad7.conf \
			    test_bad8.conf \
			    test_bad9.conf \
			    text_test.txt \
			    text_test_bad.txt \
			    text_test_long.txt \
			    text_test_short_file.txt

check_win_handler_CFLAGS = @CHECK_CFLAGS@ \
                           @GCOV_CFLAGS@
check_win_handler_LDADD = @CHECK_LIBS@ \
                          @GCOV_LIBS@ \
                          -lncurses \
                          -lpanel \
                          $(top_builddir)/src/text.o \
                          $(top_builddir)/src/misc.o \
                          $(top_builddir)/src/vsscanner.o \
                          $(top_builddir)/src/win_handler.o

check_view_SOURCES = check_view.c
check_view_CFLAGS = @CHECK_CFLAGS@ \
                    @GCOV_CFLAGS@
check_view_LDADD = @CHECK_LIBS@ \
                   @GCOV_LIBS@ \
                   -lncurses \
                   -lpanel \
                   $(top_builddir)/src/text.o \
                   $(top_builddir)/src/objects.o \
                   $(top_builddir)/src/win_handler.o \
                   $(top_builddir)/src/misc.o \
                   $(top_builddir)/src/vsscanner.o \
                   $(top_builddir)/src/view.o

check_mi2_interface_SOURCES = check_mi2_interface.c
check_mi2_interface_CFLAGS = @CHECK_CFLAGS@ \
                            @GCOV_CFLAGS@
check_mi2_interface_LDADD = @CHECK_LIBS@ \
                            @GCOV_LIBS@ \
                            $(top_builddir)/src/mi2_interface.o

check_mi2_parser_SOURCES = check_mi2_parser.c
check_mi2_parser_CFLAGS = @CHECK_CFLAGS@ \
                          @GCOV_CFLAGS@
check_mi2_parser_LDADD = @CHECK_LIBS@ \
                         @GCOV_LIBS@ \
                         $(top_builddir)/src/mi2_parser.o \
                         $(top_builddir)/src/misc.o \
                         $(top_builddir)/src/objects.o

CLEANFILES = *.gcno *.gcda

LCOV_OUTPUT  = lcov.info
LCOV_HTML  = lcov_html
LCOV_LCOV  = @LCOV@
LCOV_GENHTML  = @GENHTML@
LCOV_INPUT = $(check_PROGRAMS:%=%.info)
LCOV_DIR = $(top_builddir)/src/
LCOV_SRC = $(LCOV_DIR)/misc.gcda \
           $(LCOV_DIR)/mi2_interface.gcda\
					 $(LCOV_DIR)/misc.gcda
LCOV_FLAGS = --directory $(LCOV_DIR) \
						 --base-directory $(LCOV_DIR)

cov: $(check_PROGRAMS) $(LCOV_HTML)

$(LCOV_INPUT): $(top_builddir)/src/lvdbg $(top_builddir)/src/*.gcda
	$(LCOV_LCOV) --no-recursion -z  $(LCOV_FLAGS)
	$(LCOV_LCOV) --no-recursion -c -i $(LCOV_FLAGS) -t $(@:%.info=%) --output-file $(@:%.info=%_base.info)
	./$(@:%.info=%)
	$(LCOV_LCOV) --no-recursion --capture $(LCOV_FLAGS) -t $(@:%.info=%) --output-file $(@:%.info=%_capture.info)
	$(LCOV_LCOV) -a $(@:%.info=%_base.info) -a $(@:%.info=%_capture.info) -t $(@:%.info=%) --output-file $(@:%.info=%_tmp.info)
	$(LCOV_LCOV) --no-recursion  $(LCOV_FLAGS) -t $(@:%.info=%) --output-file $@ --remove $(@:%.info=%_tmp.info) main.c

$(LCOV_OUTPUT): $(LCOV_INPUT)
	$(LCOV_LCOV) $(LCOV_INPUT:%=-a %) -o $@

$(LCOV_HTML): $(LCOV_OUTPUT)
	-$(RM) -r $@
	LANG=C $(LCOV_GENHTML) --output-directory $@ --title "Tests Code Coverage" --show-details $<
	@echo "Point a web browser at $(LCOV_HTML)/index.html to see results."

checkmem: $(TESTS)
checkmem: $(TESTS)
	for i in $(TESTS); do \
		CK_FORK=no $(VALGRIND) $(VALGRIND_OPT) --log-file=val_log_$$i.log ./$$i; \
	done

clean-local: lcov-clean

.PHONY: lcov-clean
lcov-clean:
	-$(RM) -r $(LCOV_HTML) $(LCOV_OUTPUT)
