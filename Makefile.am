bin_PROGRAMS = kgpg

## INCLUDES were found outside kdevelop specific part

INCLUDES = $(all_includes)

kgpg_SOURCES = kgpgoptions.cpp listkeys.cpp popuppublic.cpp kgpgview.cpp kgpg.cpp main.cpp popupname.cpp kgpgfast.cpp keygener.cpp kgpginterface.cpp keyservers.cpp keyserver.ui searchres.ui popupimport.cpp kgpgeditor.cpp detailedconsole.cpp kgpglibrary.cpp kgpgwizard.ui kgpgshred.ui kgpgshredwidget.cpp keyproperties.ui conf_encryption.ui conf_decryption.ui conf_ui.ui conf_gpg.ui dcopiface.skel keyinfowidget.cpp
kgpg_LDADD   =  $(LIB_KIO) $(LIB_KABC)

KDE_ICON =kgpg

kgpg_ICON= kgpg_gen kgpg kgpg2 kgpg_edit kgpg_sign kgpg_info kgpg_import kgpg_export kgpg_key2 kgpg_key1 kgpg_manage kgpg_photo kgpg_identity

kgpgdir=$(kde_datadir)/kgpg/icons


pics_DATA=kgpg.png kgpg_unknown.png kgpg_good.png kgpg_bad.png
picsdir=$(kde_datadir)/kgpg/pics

servicedir = $(kde_datadir)/konqueror/servicemenus
service_DATA = encryptfile.desktop encryptfolder.desktop

autostartdir = $(prefix)/share/autostart
autostart_DATA = kgpg.desktop

applnkdir = $(kde_appsdir)/Utilities
applnk_DATA = kgpg.desktop

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
