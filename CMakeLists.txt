project(kgpg)

find_package( KdepimLibs REQUIRED )


add_subdirectory( icons )
add_subdirectory( core )
include_directories( ${KDEPIMLIBS_INCLUDE_DIR} core editor model transactions )

set(core_SRCS
   core/convert.cpp
   core/images.cpp
   core/emailvalidator.cpp
   core/kgpgkey.cpp )

set(kgpg_editor_SRCS
   editor/kgpgmd5widget.cpp
   editor/kgpgeditor.cpp
   editor/kgpgview.cpp
)

set(kgpg_model_SRCS
   model/kgpgitemmodel.cpp
   model/kgpgitemnode.cpp
   model/groupeditproxymodel.cpp
   model/keylistproxymodel.cpp
   model/kgpgsearchresultmodel.cpp
   model/selectkeyproxymodel.cpp
)

set(kgpg_transaction_SRCS
   transactions/kgpgtransaction.cpp
   transactions/kgpguidtransaction.cpp
   transactions/kgpgchangedisable.cpp
   transactions/kgpgchangeexpire.cpp
   transactions/kgpgchangepass.cpp
   transactions/kgpgchangetrust.cpp
   transactions/kgpgdeluid.cpp
   transactions/kgpgexport.cpp
   transactions/kgpgaddphoto.cpp
   transactions/kgpgadduid.cpp
   transactions/kgpggeneratekey.cpp
   transactions/kgpgdelkey.cpp
   transactions/kgpgimport.cpp
   transactions/kgpgkeyservertransaction.cpp
   transactions/kgpgkeyservergettransaction.cpp
   transactions/kgpgsendkeys.cpp
)

set(kgpg_SRCS ${core_SRCS} ${kgpg_editor_SRCS} ${kgpg_model_SRCS} ${kgpg_transaction_SRCS}
   selectpublickeydialog.cpp
   selectsecretkey.cpp
   kgpgoptions.cpp
   keysmanager.cpp
   kgpg.cpp
   main.cpp
   kgpgkeygenerate.cpp
   kgpginterface.cpp
   kgpgtextinterface.cpp
   gpgproc.cpp
   klinebufferedprocess.cpp
   keyservers.cpp
   detailedconsole.cpp
   kgpglibrary.cpp
   keyinfodialog.cpp
   newkey.cpp
   keyexport.cpp
   conf_encryption.cpp
   kgpgrevokewidget.cpp
   keytreeview.cpp
   groupedit.cpp
   sourceselect.cpp
   kgpgchangekey.cpp
   kgpgfirstassistant.cpp
   kgpgexternalactions.cpp
)

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
    conf_decryption.ui
    sourceselect.ui
)

kde4_add_app_icon(kgpg_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/hi*-app*-kgpg.png")

kde4_add_kcfg_files(kgpg_SRCS kgpgsettings.kcfgc )

kde4_add_executable(kgpg ${kgpg_SRCS})

target_link_libraries(kgpg
	${KDE4_KIO_LIBS}
	${KDE4_KABC_LIBS}
	${KDE4_KUTILS_LIBS}
	${KDE4_KPIMUTILS_LIBRARY}
	${KDE4_SOLID_LIBS}
)

install(TARGETS kgpg  ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

install( FILES kgpg.rc keysmanager.rc tips  DESTINATION ${DATA_INSTALL_DIR}/kgpg)
install( FILES kgpg.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR})
install( FILES kgpg.kcfg  DESTINATION ${KCFG_INSTALL_DIR})
install( FILES kgpg.desktop  DESTINATION ${AUTOSTART_INSTALL_DIR} )
install( FILES kgpg_anim.gif  DESTINATION ${DATA_INSTALL_DIR}/kgpg/pics)
install( FILES encryptfile.desktop encryptfolder.desktop viewdecrypted.desktop DESTINATION
${SERVICES_INSTALL_DIR}/ServiceMenus)
install( FILES org.kde.kgpg.Key.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})

kde4_install_icons( ${ICON_INSTALL_DIR}   )
