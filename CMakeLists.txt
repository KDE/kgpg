cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

# KDE Application Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "25")
set (RELEASE_SERVICE_VERSION_MINOR "11")
set (RELEASE_SERVICE_VERSION_MICRO "70")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(kgpg VERSION ${RELEASE_SERVICE_VERSION})
set(QT_MIN_VERSION "6.4.0")
set(KF_MIN_VERSION "5.240.0")

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(CheckIncludeFileCXX)
include(ECMAddAppIcon)
include(ECMInstallIcons)
include(FeatureSummary)
include(ECMSetupVersion)
include(CMakeFindDependencyMacro)
include(ECMQtDeclareLoggingCategory)
include(ECMDeprecationSettings)

find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
  Core
  DBus
  Gui
  PrintSupport
  Widgets
)


# Load the frameworks we need
find_package(KF6 ${KF_MIN_VERSION} REQUIRED
  Archive
  Codecs
  CoreAddons
  Crash
  DBusAddons
  I18n
  JobWidgets
  KIO
  Notifications
  Service
  TextWidgets
  XmlGui
  WidgetsAddons
  WindowSystem
  Contacts
  StatusNotifierItem
)

find_package(KPim6AkonadiContactWidgets 5.24.40 REQUIRED)

find_package(KF6DocTools ${KF_MIN_VERSION})
set_package_properties(KF6DocTools PROPERTIES
  DESCRIPTION "Tools to generate documentation"
  TYPE OPTIONAL
)

# only headers are used
find_package(Gpgme REQUIRED)

ecm_set_disabled_deprecation_versions(
     KF 5.80.0
    )

add_subdirectory( icons )
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
   transactions/customstyleditemdelegate.cpp
)

set(kgpg_SRCS ${core_SRCS} ${kgpg_editor_SRCS} ${kgpg_model_SRCS} ${kgpg_transaction_SRCS}
   selectpublickeydialog.cpp
   selectsecretkey.cpp
   kgpgoptions.cpp
   keysmanager.cpp
   kgpg.cpp
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

ecm_qt_declare_logging_category(kgpg_SRCS HEADER kgpg_general_debug.h IDENTIFIER KGPG_LOG_GENERAL CATEGORY_NAME org.kde.kgpg_general DESCRIPTION "kgpg (general)" EXPORT KGPG)
ecm_qt_declare_logging_category(kgpg_SRCS HEADER kgpg_transactions_debug.h IDENTIFIER KGPG_LOG_TRANSACTIONS CATEGORY_NAME org.kde.kgpg_transactions DESCRIPTION "kgpg (transactions)" EXPORT KGPG)

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
   klinebufferedprocess.h
   newkey.h
   selectexpirydate.h
   selectpublickeydialog.h
   selectsecretkey.h
   sourceselect.h
   transactions/customstyleditemdelegate.h
)

qt_add_dbus_adaptor( kgpg_SRCS org.kde.kgpg.Key.xml keysmanager.h KeysManager)

qt_add_dbus_interface(kgpg_SRCS org.kde.kgpg.Key.xml kgpg_interface )

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

add_library(kgpg_lib STATIC ${kgpg_SRCS} ${kgpg_HDRS})
add_executable(kgpg
    kgpg.qrc
    main.cpp
)
target_link_libraries(kgpg kgpg_lib)

if (KGPG_DEBUG_TRANSACTIONS)
	set_property(SOURCE ${kgpg_transaction_SRCS} APPEND PROPERTY COMPILE_DEFINITIONS "KGPG_DEBUG_TRANSACTIONS")
endif ()

set_property(SOURCE main.cpp APPEND PROPERTY COMPILE_DEFINITIONS "KGPG_VERSION=\"${RELEASE_SERVICE_VERSION}\"")

target_link_libraries(kgpg_lib
    KF6::Archive
    KF6::Codecs
    KF6::Contacts
    KF6::CoreAddons
    KF6::Crash
    KF6::DBusAddons
    KF6::I18n
    KF6::JobWidgets
    KF6::KIOCore
    KF6::KIOFileWidgets
    KF6::KIOWidgets
    KF6::Notifications
    KF6::Service
    KF6::TextWidgets
    KF6::XmlGui
    KF6::WidgetsAddons
    KF6::WindowSystem
    KF6::StatusNotifierItem
    KPim6::AkonadiContactWidgets
    Qt6::PrintSupport
)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

install(TARGETS kgpg  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

install( PROGRAMS org.kde.kgpg.desktop  DESTINATION ${KDE_INSTALL_APPDIR})
install( FILES org.kde.kgpg.appdata.xml  DESTINATION ${KDE_INSTALL_METAINFODIR})
install( FILES kgpg.kcfg  DESTINATION ${KDE_INSTALL_KCFGDIR})
install( FILES org.kde.kgpg.desktop  DESTINATION ${KDE_INSTALL_AUTOSTARTDIR} )

install( PROGRAMS kgpg_encryptfile.desktop kgpg_encryptfolder.desktop kgpg_viewdecrypted.desktop  DESTINATION ${KDE_INSTALL_DATADIR}/kio/servicemenus )
install( FILES org.kde.kgpg.Key.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR})

ecm_install_icons(ICONS
    16-apps-kgpg.png
    22-apps-kgpg.png
    32-apps-kgpg.png
    48-apps-kgpg.png
    64-apps-kgpg.png
    128-apps-kgpg.png
    sc-apps-kgpg.svg
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor
)

ecm_qt_install_logging_categories(EXPORT KGPG FILE kgpg.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

if (KF6DocTools_FOUND)
    add_subdirectory(doc)
    kdoctools_install(po)
endif()

ki18n_install(po)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
