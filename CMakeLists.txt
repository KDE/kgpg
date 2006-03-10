kde4_header()

add_subdirectory( icons ) 

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(kgpg_SRCS 
   kgpgoptions.cpp 
   listkeys.cpp 
   popuppublic.cpp 
   kgpgview.cpp 
   selectsecretkey.cpp 
   kgpg.cpp 
   main.cpp 
   kgpgkeygenerate.cpp 
   kgpginterface.cpp 
   keyservers.cpp 
   kgpgeditor.cpp 
   detailedconsole.cpp 
   kgpglibrary.cpp 
   keyinfowidget.cpp 
   kgpgkey.cpp 
   kgpgmd5widget.cpp 
   keylistview.cpp )

kde4_automoc(${kgpg_SRCS})

kde4_add_ui_files(kgpg_SRCS 
   keyserver.ui 
   searchres.ui 
   kgpgwizard.ui 
   keyproperties.ui 
   conf_encryption.ui 
   conf_decryption.ui 
   conf_gpg.ui 
   groupedit.ui 
   kgpgrevokewidget.ui 
   newkey.ui 
   adduid.ui 
   keyexport.ui 
   sourceselect.ui 
   conf_servers.ui 
   conf_ui2.ui 
   conf_misc.ui )

kde4_add_dcop_skels(kgpg_SRCS dcopiface.h )

kde4_add_kcfg_files(kgpg_SRCS kgpgsettings.kcfgc )

kde4_add_executable(kgpg ${kgpg_SRCS})

target_link_libraries(kgpg  ${QT_AND_KDECORE_LIBS} kio kutils kdeprint )

install_targets(/bin kgpg )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/kgpg FILES kgpg.rc listkeys.rc tips )
install_files( ${XDG_APPS_DIR} FILES kgpg.desktop )
install_files( ${KCFG_INSTALL_DIR} FILES kgpg.kcfg )
install_files( /share/autostart FILES kgpg.desktop )
install_files( ${DATA_INSTALL_DIR}/kgpg/pics FILES kgpg_anim.gif kgpg_docked.png  kgpg_docked.gif kgpg_blank.png  kgpg_fill.png )
install_files( ${DATA_INSTALL_DIR}/konqueror/servicemenus FILES encryptfile.desktop encryptfolder.desktop )

kde4_install_icons( ${ICON_INSTALL_DIR}  hicolor )

kde4_footer()



#original Makefile.am contents follow:

#SUBDIRS = icons
#bin_PROGRAMS = kgpg
#
### INCLUDES were found outside kdevelop specific part
#
#INCLUDES = $(all_includes)
#
#kgpg_SOURCES = kgpgoptions.cpp listkeys.cpp popuppublic.cpp kgpgview.cpp selectsecretkey.cpp \
#        kgpg.cpp main.cpp kgpgkeygenerate.cpp kgpginterface.cpp keyservers.cpp keyserver.ui \
#        searchres.ui kgpgeditor.cpp detailedconsole.cpp kgpglibrary.cpp kgpgwizard.ui \
#        keyproperties.ui conf_encryption.ui conf_decryption.ui conf_gpg.ui dcopiface.skel \
#        keyinfowidget.cpp groupedit.ui kgpgrevokewidget.ui newkey.ui adduid.ui  keyexport.ui \
#        sourceselect.ui kgpgsettings.kcfgc conf_servers.ui conf_ui2.ui conf_misc.ui kgpgkey.cpp \
#        kgpgmd5widget.cpp keylistview.cpp
#kgpg_LDADD   =  $(LIB_KIO) $(LIB_KABC) -lkutils -lkdeprint
#
#pics_DATA= kgpg_anim.gif kgpg_docked.png  kgpg_docked.gif kgpg_blank.png  kgpg_fill.png
#picsdir=$(kde_datadir)/kgpg/pics
#
#servicedir = $(kde_datadir)/konqueror/servicemenus
#service_DATA = encryptfile.desktop encryptfolder.desktop
#
#autostartdir = $(datadir)/autostart
#autostart_DATA = kgpg.desktop
#
#xdg_apps_DATA = kgpg.desktop
#kde_kcfg_DATA = kgpg.kcfg
#
#METASOURCES = AUTO
#
## the library search path.
#kgpg_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#
#appsdir = $(kde_datadir)/kgpg
#apps_DATA = kgpg.rc listkeys.rc tips
#
#KDE_ICON = kgpg
#
#messages: rc.cpp
#	$(PREPARETIPS) > tips.cpp
#	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
#		 if test -n "$$LIST"; then \
#		 $(XGETTEXT) $$LIST -o $(podir)/kgpg.pot; \
#         fi
#	rm -f tips.cpp
#
#
