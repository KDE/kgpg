project(kgpg)

add_subdirectory( icons )
add_subdirectory( core )
include_directories( ${CMAKE_SOURCE_DIR}/kgpg/core  )



########### next target ###############

set(core_SRCS
   ${CMAKE_SOURCE_DIR}/kgpg/core/convert.cpp
   ${CMAKE_SOURCE_DIR}/kgpg/core/emailvalidator.cpp
   ${CMAKE_SOURCE_DIR}/kgpg/core/images.cpp
   ${CMAKE_SOURCE_DIR}/kgpg/core/kgpgkey.cpp )

set(kgpg_SRCS ${core_SRCS}
   kgpgmd5widget.cpp
   selectpublickeydialog.cpp
   selectsecretkey.cpp
   kgpgoptions.cpp
   keysmanager.cpp
   kgpgview.cpp
   kgpg.cpp
   main.cpp
   kgpgkeygenerate.cpp
   kgpginterface.cpp
   keyservers.cpp
   kgpgeditor.cpp
   detailedconsole.cpp
   kgpglibrary.cpp
   keyinfodialog.cpp
   keylistview.cpp )

qt4_add_dbus_adaptor( kgpg_SRCS org.kde.kgpg.Key.xml keysmanager.h KeysManager)

kde4_automoc(${kgpg_SRCS})

kde4_add_ui3_files(kgpg_SRCS
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

kde4_add_kcfg_files(kgpg_SRCS kgpgsettings.kcfgc )

kde4_add_executable(kgpg ${kgpg_SRCS})

target_link_libraries(kgpg  ${KDE4_KIO_LIBS} ${KDE4_KABC_LIBS} kutils ${KDE4_KDEPRINT_LIBS} )

install(TARGETS kgpg  DESTINATION ${BIN_INSTALL_DIR} )


########### install files ###############

install( FILES kgpg.rc keysmanager.rc tips  DESTINATION ${DATA_INSTALL_DIR}/kgpg)
install( FILES kgpg.desktop  DESTINATION ${XDG_APPS_DIR})
install( FILES kgpg.kcfg  DESTINATION ${KCFG_INSTALL_DIR})
install( FILES kgpg.desktop  DESTINATION ${AUTOSTART_INSTALL_DIR} )
install( FILES kgpg_anim.gif kgpg_docked.png  kgpg_docked.gif kgpg_blank.png  kgpg_fill.png  DESTINATION ${DATA_INSTALL_DIR}/kgpg/pics)
install( FILES encryptfile.desktop encryptfolder.desktop  DESTINATION ${DATA_INSTALL_DIR}/konqueror/servicemenus)

kde4_install_icons( ${ICON_INSTALL_DIR}   )




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
