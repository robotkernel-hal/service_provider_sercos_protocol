ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libservice_provider_sercos_protocol.la

BUILT_SOURCES = $(top_builddir)/src/service_definitions.h
CLEANFILES = $(top_builddir)/src/service_definitions.h

extraincludedir = $(includedir)/service_provider_sercos_protocol
extrainclude_HEADERS = $(top_srcdir)/include/service_provider_sercos_protocol/base.h

libservice_provider_sercos_protocol_la_SOURCES  = service_id.cpp provider.cpp
libservice_provider_sercos_protocol_la_CXXFLAGS = -fno-strict-aliasing -Wall 		\
												  -I$(top_srcdir)/include -I$(builddir)/include \
												  @ROBOTKERNEL_CFLAGS@ 	
libservice_provider_sercos_protocol_la_LDFLAGS  = -Wall -release $(PACKAGE_VERSION)

VPATH = $(srcdir):$(top_srcdir)/share/service_definitions

SERVICE_DEFINITIONS = service_provider_sercos_protocol/read_id \
					  service_provider_sercos_protocol/write_id

$(top_builddir)/src/service_definitions.h: Makefile $(SERVICE_DEFINITIONS)
	@echo creating $@
	rk_generate --indir $(top_srcdir)/share/service_definitions --out $@ $(SERVICE_DEFINITIONS)
	touch $@


