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

kde4_automoc(kgpg ${kgpg_SRCS})

kde4_add_ui_files(kgpg_SRCS adduid.ui conf_gpg.ui searchres.ui)
kde4_add_ui3_files(kgpg_SRCS
   keyserver.ui
   kgpgwizard.ui
   keyproperties.ui
   conf_encryption.ui
   conf_decryption.ui
   groupedit.ui
   kgpgrevokewidget.ui
   newkey.ui
   keyexport.ui
   sourceselect.ui
   conf_servers.ui
   conf_ui2.ui
   conf_misc.ui )

kde4_add_kcfg_files(kgpg kgpg_SRCS kgpgsettings.kcfgc )

kde4_add_executable(kgpg ${kgpg_SRCS})

target_link_libraries(kgpg  ${KDE4_KIO_LIBS} ${KDE4_KABC_LIBS} ${KDE4_KUTILS_LIBS} ${KDE4_KDEPRINT_LIBS} )

install(TARGETS kgpg  DESTINATION ${BIN_INSTALL_DIR} )


########### install files ###############

install( FILES kgpg.rc keysmanager.rc tips  DESTINATION ${DATA_INSTALL_DIR}/kgpg)
install( FILES kgpg.desktop  DESTINATION ${XDG_APPS_DIR})
install( FILES kgpg.kcfg  DESTINATION ${KCFG_INSTALL_DIR})
install( FILES kgpg.desktop  DESTINATION ${AUTOSTART_INSTALL_DIR} )
install( FILES kgpg_anim.gif kgpg_docked.png  kgpg_docked.gif kgpg_blank.png  kgpg_fill.png  DESTINATION ${DATA_INSTALL_DIR}/kgpg/pics)
install( FILES encryptfile.desktop encryptfolder.desktop  DESTINATION ${DATA_INSTALL_DIR}/konqueror/servicemenus)

kde4_install_icons( ${ICON_INSTALL_DIR}   )

