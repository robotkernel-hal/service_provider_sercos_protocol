ACLOCAL_AMFLAGS = -I m4 

libdir = $(prefix)/lib
lib_LTLIBRARIES = libinterface_sercos_protocol.la
BUILT_SOURCES = ln_messages.h
CLEANFILES = ln_messages.h

libinterface_sercos_protocol_la_SOURCES  = service_id.cpp interface_sercos_protocol.cpp
libinterface_sercos_protocol_la_CXXFLAGS = -fno-strict-aliasing -Wall -I$(top_srcdir)/src \
										   -I$(top_builddir) @YAMLCPP_CFLAGS@ @LN_CFLAGS@ 
libinterface_sercos_protocol_la_LDFLAGS  = -Wall @PTHREAD_LIBS@ @RT_LIBS@ -release $(PACKAGE_VERSION)

VPATH = $(srcdir):$(srcdir)/message_definitions

LN_MESSAGES = \
			  robotkernel/sercos_protocol/read_id \
			  robotkernel/sercos_protocol/write_id \
			  robotkernel/sercos_protocol/set_command

ln_messages.h: Makefile $(LN_MESSAGES)
	@echo creating ln_messages.h
	@LN_GENERATE@ -o $@ $(LN_MESSAGES)
	touch $@

install-exec-hook:
	mkdir -p $(prefix)/share/message_definitions
	cp -r $(srcdir)/message_definitions/* $(prefix)/share/message_definitions/
	