project(kgpg)

add_subdirectory( icons )
add_subdirectory( core )
include_directories( core  )

add_definitions (-DQT3_SUPPORT -DQT3_SUPPORT_WARNINGS)

########### next target ###############

set(core_SRCS
   core/convert.cpp
   core/emailvalidator.cpp
   core/images.cpp
   core/kgpgkey.cpp )

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
   gpgproc.cpp
   keyservers.cpp
   kgpgeditor.cpp
   detailedconsole.cpp
   kgpglibrary.cpp
   keyinfodialog.cpp
   keylistview.cpp
   newkey.cpp
   keyexport.cpp
   conf_encryption.cpp
   kgpgrevokewidget.cpp
   sourceselect.cpp)

qt4_add_dbus_adaptor( kgpg_SRCS org.kde.kgpg.Key.xml keysmanager.h KeysManager)

QT4_ADD_DBUS_INTERFACE(kgpg_SRCS org.kde.kgpg.Key.xml kgpg_interface )

kde4_add_ui_files(kgpg_SRCS
    adduid.ui
    conf_gpg.ui
    searchres.ui
    groupedit.ui
    conf_ui2.ui
    conf_servers.ui
    conf_misc.ui
    newkey.ui
    keyexport.ui
    conf_encryption.ui
    kgpgrevokewidget.ui
    keyserver.ui
    kgpgwizard.ui
    conf_decryption.ui
    sourceselect.ui
)

kde4_add_kcfg_files(kgpg_SRCS kgpgsettings.kcfgc )

kde4_add_executable(kgpg ${kgpg_SRCS})

target_link_libraries(kgpg   ${KDE4_KABC_LIBS} ${KDE4_KUTILS_LIBS} ${KDE4_KDE3SUPPORT_LIBS})

install(TARGETS kgpg  DESTINATION ${BIN_INSTALL_DIR} )

########### install files ###############

install( FILES kgpg.rc keysmanager.rc tips  DESTINATION ${DATA_INSTALL_DIR}/kgpg)
install( FILES kgpg.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR})
install( FILES kgpg.kcfg  DESTINATION ${KCFG_INSTALL_DIR})
install( FILES kgpg.desktop  DESTINATION ${AUTOSTART_INSTALL_DIR} )
install( FILES kgpg_anim.gif kgpg_docked.png  kgpg_docked.gif kgpg_blank.png  kgpg_fill.png  DESTINATION ${DATA_INSTALL_DIR}/kgpg/pics)
install( FILES encryptfile.desktop encryptfolder.desktop  DESTINATION ${DATA_INSTALL_DIR}/konqueror/servicemenus)
install( FILES org.kde.kgpg.Key.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})

kde4_install_icons( ${ICON_INSTALL_DIR}   )
