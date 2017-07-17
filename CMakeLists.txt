cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

# KDE Application Version, managed by release script
set (KDE_APPLICATIONS_VERSION_MAJOR "17")
set (KDE_APPLICATIONS_VERSION_MINOR "11")
set (KDE_APPLICATIONS_VERSION_MICRO "70")
set (KDE_APPLICATIONS_VERSION "${KDE_APPLICATIONS_VERSION_MAJOR}.${KDE_APPLICATIONS_VERSION_MINOR}.${KDE_APPLICATIONS_VERSION_MICRO}")

project(kgpg)
set(QT_MIN_VERSION "5.3.0")

find_package(ECM 5.21 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

include(ECMAddAppIcon)
include(ECMInstallIcons)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)
include(ECMSetupVersion)
include(ECMPackageConfigHelpers)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
  Core
  DBus
  Gui
  PrintSupport
  Widgets
)

# Load the frameworks we need
find_package(KF5 5.28 REQUIRED
  Archive
  DocTools
  Codecs
  CoreAddons
  Crash
  DBusAddons
  I18n
  IconThemes
  JobWidgets
  KIO
  Notifications
  Service
  TextWidgets
  XmlGui
  WidgetsAddons
  WindowSystem
)

# kdepimlibs
find_package(KF5 REQUIRED
  AkonadiContact
  Contacts
)

# only headers are used
find_package(Gpgme REQUIRED)

add_definitions(-DQT_NO_URL_CAST_FROM_STRING)

add_subdirectory( icons )
add_subdirectory( doc )
include_directories(  ${KDEPIMLIBS_INCLUDE_DIR} ${GPGME_INCLUDES} )

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
   transactions/kgpgtransactionprivate.cpp
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
   kgpg_debug.cpp
   main.cpp
   kgpgkeygenerate.cpp
   kgpginterface.cpp
   kgpgtextinterface.cpp
   gpgproc.cpp
   klinebufferedprocess.cpp
   klinebufferedprocessprivate.cpp
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

set(kgpg_HDRS
   caff.h
   caff_p.h
   conf_encryption.h
   detailedconsole.h
   foldercompressjob.h
   gpgproc.h
   groupedit.h
   keyexport.h
   keyinfodialog.h
   keyservers.h
   keysmanager.h
   keytreeview.h
   kgpg.h
   kgpgchangekey.h
   kgpgexternalactions.h
   kgpgfirstassistant.h
   kgpginterface.h
   kgpgkeygenerate.h
   kgpgoptions.h
   kgpgrevokewidget.h
   kgpgtextinterface.h
   kgpg_debug.h
   klinebufferedprocess.h
   newkey.h
   selectexpirydate.h
   selectpublickeydialog.h
   selectsecretkey.h
   sourceselect.h
)

qt5_add_dbus_adaptor( kgpg_SRCS org.kde.kgpg.Key.xml keysmanager.h KeysManager)

qt5_add_dbus_interface(kgpg_SRCS org.kde.kgpg.Key.xml kgpg_interface )

ki18n_wrap_ui(kgpg_SRCS
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

file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/*-apps-kgpg.png")
ecm_add_app_icon(kgpg_SRCS ICONS ${ICONS_SRCS})

kconfig_add_kcfg_files(kgpg_SRCS kgpgsettings.kcfgc )

add_executable(kgpg ${kgpg_SRCS} ${kgpg_HDRS})

if (KGPG_DEBUG_TRANSACTIONS)
	set_property(SOURCE ${kgpg_transaction_SRCS} APPEND PROPERTY COMPILE_DEFINITIONS "KGPG_DEBUG_TRANSACTIONS")
endif (KGPG_DEBUG_TRANSACTIONS)

set_property(SOURCE main.cpp APPEND PROPERTY COMPILE_DEFINITIONS "KGPG_VERSION=\"${KDE_APPLICATIONS_VERSION}\"")

target_link_libraries(kgpg
    KF5::AkonadiContact
    KF5::Archive
    KF5::Codecs
    KF5::Contacts
    KF5::CoreAddons
    KF5::Crash
    KF5::DBusAddons
    KF5::I18n
    KF5::IconThemes
    KF5::JobWidgets
    KF5::KIOCore
    KF5::KIOFileWidgets
    KF5::KIOWidgets
    KF5::Notifications
    KF5::Service
    KF5::TextWidgets
    KF5::XmlGui
    KF5::WidgetsAddons
    KF5::WindowSystem
    Qt5::PrintSupport
)

install(TARGETS kgpg  ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

install( FILES editor/kgpgeditor.rc keysmanager.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/kgpg)
install( FILES tips  DESTINATION ${DATA_INSTALL_DIR}/kgpg)
install( PROGRAMS org.kde.kgpg.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR})
install( FILES org.kde.kgpg.appdata.xml  DESTINATION ${KDE_INSTALL_METAINFODIR})
install( FILES kgpg.kcfg  DESTINATION ${KCFG_INSTALL_DIR})
install( FILES org.kde.kgpg.desktop  DESTINATION ${AUTOSTART_INSTALL_DIR} )
install( FILES encryptfile.desktop encryptfolder.desktop viewdecrypted.desktop DESTINATION
${SERVICES_INSTALL_DIR}/ServiceMenus)
install( FILES org.kde.kgpg.Key.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})

ecm_install_icons(ICONS
    16-apps-kgpg.png
    22-apps-kgpg.png
    32-apps-kgpg.png
    48-apps-kgpg.png
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor
)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
