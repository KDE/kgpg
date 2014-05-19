project(kgpg)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
    find_package(KDE4)
    include( KDE4Defaults )

    add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
    add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)

    # If definitions like -D_GNU_SOURCE are needed for these checks they
    # should be added to _KDE4_PLATFORM_DEFINITIONS when it is originally
    # defined outside this file.  Here we include these definitions in
    # CMAKE_REQUIRED_DEFINITIONS so they will be included in the build of
    # checks below.
    set( CMAKE_REQUIRED_DEFINITIONS ${_KDE4_PLATFORM_DEFINITIONS} )
endif()

find_package( KdepimLibs REQUIRED )
# only headers are used
find_package( Gpgme REQUIRED )

add_subdirectory( icons )
add_subdirectory( doc )
include_directories( ${KDE4_INCLUDES} ${KDEPIMLIBS_INCLUDE_DIR} ${GPGME_INCLUDES} )

option(KGPG_DEBUG_TRANSACTIONS "show commands and results of gpg calls in debug log" Off)

set(core_SRCS
   core/convert.cpp
   core/images.cpp
   core/emailvalidator.cpp
   core/kgpgkey.cpp
   core/KGpgExpandableNode.cpp
   core/KGpgKeyNode.cpp
   core/KGpgGroupMemberNode.cpp
   core/KGpgGroupNode.cpp
   core/KGpgNode.cpp
   core/KGpgOrphanNode.cpp
   core/KGpgRefNode.cpp
   core/KGpgRootNode.cpp
   core/KGpgSignableNode.cpp
   core/KGpgSignNode.cpp
   core/KGpgSubkeyNode.cpp
   core/KGpgUatNode.cpp
   core/KGpgUidNode.cpp
)

set(kgpg_editor_SRCS
   editor/kgpgmd5widget.cpp
   editor/kgpgeditor.cpp
   editor/kgpgtextedit.cpp
)

set(kgpg_model_SRCS
   model/gpgservermodel.cpp
   model/kgpgitemmodel.cpp
   model/groupeditproxymodel.cpp
   model/keylistproxymodel.cpp
   model/kgpgsearchresultmodel.cpp
   model/selectkeyproxymodel.cpp
)

set(kgpg_transaction_SRCS
   transactions/kgpgtransaction.cpp
   transactions/kgpgtransactionjob.cpp
   transactions/kgpguidtransaction.cpp
   transactions/kgpgeditkeytransaction.cpp
   transactions/kgpgsigntransactionhelper.cpp
   transactions/kgpgchangedisable.cpp
   transactions/kgpgchangeexpire.cpp
   transactions/kgpgchangepass.cpp
   transactions/kgpgchangetrust.cpp
   transactions/kgpgdelsign.cpp
   transactions/kgpgdeluid.cpp
   transactions/kgpgencrypt.cpp
   transactions/kgpgexport.cpp
   transactions/kgpgaddphoto.cpp
   transactions/kgpgadduid.cpp
   transactions/kgpggeneratekey.cpp
   transactions/kgpggeneraterevoke.cpp
   transactions/kgpgdecrypt.cpp
   transactions/kgpgdelkey.cpp
   transactions/kgpgimport.cpp
   transactions/kgpgkeyservertransaction.cpp
   transactions/kgpgkeyservergettransaction.cpp
   transactions/kgpgkeyserversearchtransaction.cpp
   transactions/kgpgprimaryuid.cpp
   transactions/kgpgsendkeys.cpp
   transactions/kgpgsignkey.cpp
   transactions/kgpgsigntext.cpp
   transactions/kgpgsignuid.cpp
   transactions/kgpgtextorfiletransaction.cpp
   transactions/kgpgverify.cpp
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
   selectexpirydate.cpp
   caff.cpp
   foldercompressjob.cpp
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
    kgpgKeyInfo.ui
    kgpgkeygenerate.ui
    kgpgrevokewidget.ui
    keyserver.ui
    conf_decryption.ui
    sourceselect.ui
)

kde4_add_app_icon(kgpg_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/hi*-app*-kgpg.png")

kde4_add_kcfg_files(kgpg_SRCS kgpgsettings.kcfgc )

kde4_add_executable(kgpg ${kgpg_SRCS})

if (KGPG_DEBUG_TRANSACTIONS)
	get_target_property(KGPG_FLAGS kgpg COMPILE_DEFINITIONS)
	if (NOT KGPG_FLAGS)
		# get rid of the NOTFOUND
		set(KGPG_FLAGS)
	endif (NOT KGPG_FLAGS)
	list(APPEND KGPG_FLAGS KGPG_DEBUG_TRANSACTIONS)
	set_target_properties(kgpg PROPERTIES COMPILE_DEFINITIONS "${KGPG_FLAGS}")
endif (KGPG_DEBUG_TRANSACTIONS)

target_link_libraries(kgpg
	${KDE4_KIO_LIBS}
	${KDE4_KABC_LIBS}
	${KDE4_KUTILS_LIBS}
	${KDE4_KPIMUTILS_LIBRARY}
	${KDE4_SOLID_LIBS}
	${KDEPIMLIBS_AKONADI_CONTACT_LIBS}
)

install(TARGETS kgpg  ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

install( FILES editor/kgpgeditor.rc keysmanager.rc tips  DESTINATION ${DATA_INSTALL_DIR}/kgpg)
install( PROGRAMS kgpg.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR})
install( FILES kgpg.appdata.xml  DESTINATION share/appdata/ )
install( FILES kgpg.kcfg  DESTINATION ${KCFG_INSTALL_DIR})
install( FILES kgpg.desktop  DESTINATION ${AUTOSTART_INSTALL_DIR} )
install( FILES encryptfile.desktop encryptfolder.desktop viewdecrypted.desktop DESTINATION
${SERVICES_INSTALL_DIR}/ServiceMenus)
install( FILES org.kde.kgpg.Key.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})

kde4_install_icons( ${ICON_INSTALL_DIR}   )
