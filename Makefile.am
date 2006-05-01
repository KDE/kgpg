SUBDIRS = icons
bin_PROGRAMS = kgpg

## INCLUDES were found outside kdevelop specific part

INCLUDES = $(all_includes)

kgpg_SOURCES = kgpgoptions.cpp listkeys.cpp popuppublic.cpp kgpgview.cpp selectsecretkey.cpp \
        kgpg.cpp main.cpp kgpgkeygenerate.cpp kgpginterface.cpp keyservers.cpp keyserver.ui \
        searchres.ui kgpgeditor.cpp detailedconsole.cpp kgpglibrary.cpp kgpgwizard.ui \
        keyproperties.ui conf_encryption.ui conf_decryption.ui conf_gpg.ui dcopiface.skel \
        keyinfowidget.cpp groupedit.ui kgpgrevokewidget.ui newkey.ui adduid.ui  keyexport.ui \
        sourceselect.ui kgpgsettings.kcfgc conf_servers.ui conf_ui2.ui conf_misc.ui kgpgkey.cpp \
        kgpgmd5widget.cpp keylistview.cpp
kgpg_LDADD   =  $(LIB_KIO) $(LIB_KABC) -lkutils -lkdeprint

pics_DATA= kgpg_anim.gif kgpg_docked.png  kgpg_docked.gif kgpg_blank.png  kgpg_fill.png
picsdir=$(kde_datadir)/kgpg/pics

servicedir = $(kde_datadir)/konqueror/servicemenus
service_DATA = encryptfile.desktop encryptfolder.desktop

autostartdir = $(datadir)/autostart
autostart_DATA = kgpg.desktop

xdg_apps_DATA = kgpg.desktop
kde_kcfg_DATA = kgpg.kcfg

METASOURCES = AUTO

# the library search path.
kgpg_LDFLAGS = $(all_libraries) $(KDE_RPATH)

appsdir = $(kde_datadir)/kgpg
apps_DATA = kgpg.rc listkeys.rc tips

KDE_ICON = kgpg



