bin_PROGRAMS = kgpg

## INCLUDES were found outside kdevelop specific part

kgpg_SOURCES = kgpgoptions.cpp listkeys.cpp popuppublic.cpp kgpgview.cpp kgpg.cpp main.cpp popupname.cpp kgpgfast.cpp keygener.cpp kgpgoption.ui kgpginterface.cpp 
kgpg_LDADD   =  $(LIB_KIO)

KDE_ICON = kgpg_gen kgpg kgpg2 kgpg_edit kgpg_sign kgpg_info kgpg_import kgpg_export kgpg_key2 kgpg_key1 kgpg_manage kgpg_photo kgpg_identity

servicedir = $(kde_datadir)/konqueror/servicemenus
service_DATA = encryptfile.desktop

applnkdir = $(kde_appsdir)/Applications
applnk_DATA = kgpg.desktop keymanager.desktop

# set the include path for X, qt and KDE
INCLUDES= $(all_includes)

METASOURCES = AUTO

# the library search path. 
kgpg_LDFLAGS = $(all_libraries) $(KDE_RPATH)

appsdir = $(kde_datadir)/kgpg
apps_DATA = kgpg.rc listkeys.rc tips

messages: rc.cpp
	$(PREPARETIPS) > tips.cpp
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
	 $(XGETTEXT) $$LIST -o $(podir)/kgpg.pot; \
	fi

