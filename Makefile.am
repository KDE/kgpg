####### kdevelop will overwrite this part!!! (begin)##########
bin_PROGRAMS = kgpg

## INCLUDES were found outside kdevelop specific part

kgpg_SOURCES = kgpgoptions.cpp listkeys.cpp popuppublic.cpp kgpgview.cpp kgpg.cpp main.cpp popupname.cpp popupname.h hi16-app-kgpg.png hi32-app-kgpg.png hi48-app-kgpg.png kgpgfast.cpp keygener.cpp keygener.h tips
kgpg_LDADD   =  $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_QT) $(LIBSOCKET)


EXTRA_DIST = main.cpp kgpg.cpp kgpg.h kgpgview.cpp kgpgview.h kgpg.desktop popuppublic.cpp popuppublic.h listkeys.h listkeys.cpp kgpgoptions.h kgpgoptions.cpp 

install-data-local:
	$(mkinstalldirs) $(kde_appsdir)/Applications/
	$(INSTALL_DATA) $(srcdir)/kgpg.desktop $(kde_appsdir)/Applications/kgpg.desktop
	$(mkinstalldirs) $(kde_icondir)/hicolor/48x48/apps/
	$(INSTALL_DATA) $(srcdir)/hi48-app-kgpg.png $(kde_icondir)/hicolor/48x48/apps/kgpg.png
	$(mkinstalldirs) $(kde_icondir)/hicolor/32x32/apps/
	$(INSTALL_DATA) $(srcdir)/hi32-app-kgpg.png $(kde_icondir)/hicolor/32x32/apps/kgpg.png
	$(mkinstalldirs) $(kde_icondir)/hicolor/16x16/apps/
	$(INSTALL_DATA) $(srcdir)/hi16-app-kgpg.png $(kde_icondir)/hicolor/16x16/apps/kgpg.png
	$(INSTALL_DATA) $(srcdir)/hi16-app-kgpg2.png $(kde_icondir)/hicolor/16x16/apps/kgpg2.png
	$(mkinstalldirs) $(DESTDIR)$(kde_icondir)/hicolor/22x22/actions
	$(INSTALL_DATA) $(srcdir)/hi22-action-kgpg_gen.png $(kde_icondir)/hicolor/22x22/actions/kgpg_gen.png
	$(INSTALL_DATA) $(srcdir)/hi22-action-kgpg_edit.png $(kde_icondir)/hicolor/22x22/actions/kgpg_edit.png
	$(INSTALL_DATA) $(srcdir)/hi22-action-kgpg_sign.png $(kde_icondir)/hicolor/22x22/actions/kgpg_sign.png
	$(INSTALL_DATA) $(srcdir)/hi22-action-kgpg_info.png $(kde_icondir)/hicolor/22x22/actions/kgpg_info.png
	$(INSTALL_DATA) $(srcdir)/hi22-action-kgpg_import.png $(kde_icondir)/hicolor/22x22/actions/kgpg_import.png
	$(INSTALL_DATA) $(srcdir)/hi22-action-kgpg_export.png $(kde_icondir)/hicolor/22x22/actions/kgpg_export.png
	$(mkinstalldirs) $(DESTDIR)$(kde_icondir)/hicolor/16x16/actions
	$(INSTALL_DATA) $(srcdir)/hi16-action-kgpg_key2.png $(kde_icondir)/hicolor/16x16/actions/kgpg_key2.png
	$(INSTALL_DATA) $(srcdir)/hi16-action-kgpg_key1.png $(kde_icondir)/hicolor/16x16/actions/kgpg_key1.png
	$(INSTALL_DATA) $(srcdir)/hi16-action-kgpg_dkey2.png $(kde_icondir)/hicolor/16x16/actions/kgpg_dkey2.png
	$(INSTALL_DATA) $(srcdir)/hi16-action-kgpg_dkey1.png $(kde_icondir)/hicolor/16x16/actions/kgpg_dkey1.png

	$(INSTALL_DATA) $(srcdir)/hi22-action-kgpg_manage.png $(kde_icondir)/hicolor/22x22/actions/kgpg_manage.png
	
	$(mkinstalldirs) $(kde_datadir)/konqueror/servicemenus/

#	$(INSTALL_DATA) $(srcdir)/decryptfile.desktop $(kde_datadir)/konqueror/servicemenus/decryptfile.desktop
	-rm -f $(kde_datadir)/konqueror/servicemenus/decryptfile.desktop
	
	$(INSTALL_DATA) $(srcdir)/encryptfile.desktop $(kde_datadir)/konqueror/servicemenus/encryptfile.desktop
	$(mkinstalldirs) $(kde_mimedir)/application
	$(INSTALL_DATA) $(srcdir)/pgp-encrypted.desktop $(kde_mimedir)/application/pgp-encrypted.desktop

	$(mkinstalldirs) $(kde_datadir)/kgpg/
	$(INSTALL_DATA) $(srcdir)/tips $(kde_datadir)/kgpg/tips

	
uninstall-local:
	-rm -f $(kde_appsdir)/Applications/kgpg.desktop
	-rm -f $(kde_icondir)/hicolor/48x48/apps/kgpg.png
	-rm -f $(kde_icondir)/hicolor/32x32/apps/kgpg.png
	-rm -f $(kde_icondir)/hicolor/16x16/apps/kgpg.png
	
	-rm -f $(kde_icondir)/hicolor/22x22/actions/kgpg_gen.png
	-rm -f $(kde_icondir)/hicolor/22x22/actions/kgpg_edit.png
	-rm -f $(kde_icondir)/hicolor/22x22/actions/kgpg_sign.png
	-rm -f $(kde_icondir)/hicolor/22x22/actions/kgpg_info.png
	-rm -f $(kde_icondir)/hicolor/22x22/actions/kgpg_import.png
	-rm -f $(kde_icondir)/hicolor/22x22/actions/kgpg_export.png
	-rm -f $(kde_icondir)/hicolor/16x16/actions/kgpg_key2.png
	-rm -f $(kde_icondir)/hicolor/16x16/actions/kgpg_key1.png
	-rm -f $(kde_icondir)/hicolor/16x16/actions/kgpg_dkey2.png
	-rm -f $(kde_icondir)/hicolor/16x16/actions/kgpg_dkey1.png
	-rm -f $(kde_icondir)/hicolor/22x22/actions/kgpg_manage.png
	-rm -f $(kde_icondir)/hicolor/16x16/apps/kgpg2.png
	
#	-rm -f $(kde_datadir)/konqueror/servicemenus/decryptfile.desktop
	-rm -f $(kde_datadir)/konqueror/servicemenus/encryptfile.desktop
	-rm -f $(kde_datadir)/kgpg/tips

	
####### kdevelop will overwrite this part!!! (end)############
# These paths are KDE specific. Use them:
# kde_appsdir         Where your application's menu entry (.desktop) should go to.
# kde_icondir         Where your icon should go to - better use KDE_ICON.
# kde_sounddir        Where your sounds should go to.
# kde_htmldir         Where your docs should go to. (contains lang subdirs)
# kde_datadir         Where you install application data. (Use a subdir)
# kde_locale          Where translation files should go to. (contains lang subdirs)
# kde_cgidir          Where cgi-bin executables should go to.
# kde_confdir         Where config files should go to (system-wide ones with default values).
# kde_mimedir         Where mimetypes .desktop files should go to.
# kde_servicesdir     Where services .desktop files should go to.
# kde_servicetypesdir Where servicetypes .desktop files should go to.
# kde_toolbardir      Where general toolbar icons should go to (deprecated, use KDE_ICON).
# kde_wallpaperdir    Where general wallpapers should go to.
# kde_templatesdir    Where templates for the "New" menu (Konqueror/KDesktop) should go to.
# kde_bindir          Where executables should go to. Use bin_PROGRAMS or bin_SCRIPTS.
# kde_libdir          Where shared libraries should go to. Use lib_LTLIBRARIES.
# kde_moduledir       Where modules (e.g. parts) should go to. Use kde_module_LTLIBRARIES.
# kde_styledir        Where Qt/KDE widget styles should go to (new in KDE 3).
# kde_designerdir     Where Qt Designer plugins should go to (new in KDE 3).

# set the include path for X, qt and KDE
INCLUDES= $(all_includes)

METASOURCES = AUTO

# the library search path. 
kgpg_LDFLAGS = $(all_libraries) $(KDE_RPATH)

rcdir = $(kde_datadir)/kgpg
rc_DATA = kgpg.rc

messages: rc.cpp
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
	 $(XGETTEXT) $$LIST -o $(podir)/kgpg.pot; \
	fi

