libdir = $(prefix)/lib
lib_LTLIBRARIES = libservice_provider_sercos_protocol.la

extraincludedir = $(includedir)/service_provider/sercos_protocol
extrainclude_HEADERS = include/service_provider/sercos_protocol/base.h

libservice_provider_sercos_protocol_la_SOURCES  = service_id.cpp provider.cpp
libservice_provider_sercos_protocol_la_CXXFLAGS = --std=c++11 -fno-strict-aliasing -Wall 		\
												  -I$(srcdir) -I$(builddir) -I$(srcdir)/include \
												  @ROBOTKERNEL_CFLAGS@ 	
libservice_provider_sercos_protocol_la_LDFLAGS  = -Wall -release $(PACKAGE_VERSION)

install-exec-hook:
	mkdir -p $(prefix)/share/ln
	cp -rf $(top_builddir)/share/ln/service_provider_sercos_protocol.inc.lnc $(prefix)/share/ln/
	find $(DESTDIR)$(libdir) -type f -name \*.la -delete
