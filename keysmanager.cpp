/***************************************************************************
                          keysmanager.cpp  -  description
                             -------------------
    begin                : Thu Jul 4 2002
    copyright          : (C) 2002 by Jean-Baptiste Mardelle
    email                : bj@altern.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "keysmanager.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include <QApplication>
#include <QDragMoveEvent>
#include <QDesktopWidget>
#include <QRadioButton>
#include <QKeySequence>
#include <QTextStream>
#include <QVBoxLayout>
#include <QCloseEvent>
#include <QToolButton>
#include <QClipboard>
#include <QTextCodec>
#include <QTabWidget>
#include <QDropEvent>
#include <QCheckBox>
#include <QComboBox>
#include <QVariant>
#include <QPainter>
#include <QtGui/QPrinter>
#include <QtGui/QPrintDialog>
#include <QPixmap>
#include <QLayout>
#include <QRegExp>
#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QEvent>
#include <QFlags>
#include <QMovie>
#include <QList>
#include <QFile>
#include <QDir>
#include <QtDBus>
#include <Q3TextDrag>
#include <QProcess>

#include <kabc/addresseedialog.h>
#include <kabc/stdaddressbook.h>
#include <KPasswordDialog>
#include <KToolInvocation>
#include <KUrlRequester>
#include <kio/netaccess.h>
#include <KStandardDirs>
#include <KPassivePopup>
#include <KDesktopFile>
#include <KInputDialog>
#include <KFileDialog>
#include <KMessageBox>
#include <KFindDialog>
#include <KStatusBar>
#include <KService>
#include <KServiceTypeTrader>
#include <KLineEdit>
#include <KMimeType>
#include <KShortcut>
#include <KStandardShortcut>
#include <KLocale>
#include <KProcess>
#include <KAction>
#include <KDebug>
#include <KFind>
#include <KMenu>
#include <KUrl>
#include <ktip.h>
#include <KRun>
#include <KToolBar>
#include <KActionCollection>
#include <KStandardAction>
#include <KSelectAction>
#include <KIcon>
#include <KVBox>
#include <KToggleAction>

#include "kgpgkey.h"
#include "selectsecretkey.h"
#include "newkey.h"
#include "kgpg.h"
#include "kgpgeditor.h"
#include "kgpgview.h"
#include "keyexport.h"
#include "kgpgrevokewidget.h"
#include "keyservers.h"
#include "kgpginterface.h"
#include "kgpgsettings.h"
#include "kgpgkeygenerate.h"
#include "kgpgoptions.h"
#include "keyinfodialog.h"
#include "kgpglibrary.h"
#include "keylistview.h"
#include "keyadaptor.h"
#include "images.h"
#include "sourceselect.h"
#include "keylistproxymodel.h"
#include "keytreeview.h"
#include "groupedit.h"

using namespace KgpgCore;

KeysManager::KeysManager(QWidget *parent)
           : KXmlGuiWindow(parent)
{
    new KeyAdaptor(this);
    QDBusConnection::sessionBus().registerObject("/KeyInterface", this);

    setWindowTitle(i18n("Key Management"));

    m_statusbartimer = new QTimer(this);
    keysList2 = new KeyListView(this);
    keysList2->photoKeysList = QString();
    keysList2->groupNb = 0;
    m_statusbar = 0;
    readOptions();

    if (showTipOfDay)
        installEventFilter(this);

    KStandardAction::quit(this, SLOT(quitApp()), actionCollection());
    KStandardAction::find(this, SLOT(findKey()), actionCollection());
    KStandardAction::findNext(this, SLOT(findNextKey()), actionCollection());
    actionCollection()->addAction(KStandardAction::Preferences, "options_configure", this, SLOT(showOptions()));

    QAction *action = 0;

    action = actionCollection()->addAction( "key_server" );
    action->setText( i18n("&Key Server Dialog") );
    action->setIcon( KIcon("network-server") );
    connect(action, SIGNAL(triggered(bool)), SLOT(showKeyServer()));

    action =  actionCollection()->addAction( "help_tipofday");
    action->setIcon( KIcon("help-hint") );
    action->setText( i18n("Tip of the &Day") );
    connect(action, SIGNAL(triggered(bool)), SLOT(slotTip()));

    action = actionCollection()->addAction( "gpg_man");
    action->setText( i18n("View GnuPG Manual") );
    action->setIcon( KIcon("help-contents") );
    connect(action, SIGNAL(triggered(bool)), SLOT(slotManpage()));

    action = actionCollection()->addAction("kgpg_editor");
    action->setIcon(KIcon("document-properties"));
    action->setText(i18n("&Open Editor"));
    connect(action, SIGNAL(triggered(bool)), SLOT(slotOpenEditor()));

    action = actionCollection()->addAction("go_default_key");
    action->setIcon(KIcon("go-home"));
    action->setText(i18n("&Go to Default Key"));
    connect(action, SIGNAL(triggered(bool)), SLOT(slotGotoDefaultKey()));
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Home));

    action = actionCollection()->addAction("key_refresh");
    action->setIcon(KIcon("view-refresh"));
    action->setText(i18n("&Refresh List"));
    connect(action, SIGNAL(triggered(bool)), SLOT(refreshkey()));
    action->setShortcuts(KStandardShortcut::reload());

    hPublic = actionCollection()->add<KToggleAction>("show_secret");
    hPublic->setIcon(KIcon("kgpg_show"));
    hPublic->setText(i18n("&Show only Secret Keys"));
    hPublic->setChecked(KGpgSettings::showSecret());
    connect(hPublic, SIGNAL(triggered(bool)), SLOT(slotToggleSecret(bool)));

    hExRev = actionCollection()->add<KToggleAction>("hide_disabled");
    hExRev->setText(i18n("&Hide Expired/Disabled Keys"));
    hExRev->setChecked(KGpgSettings::hideExRev());
    connect(hExRev, SIGNAL(triggered(bool)), SLOT(slotToggleDisabled(bool)));

    QAction *infoKey = actionCollection()->addAction("key_info");
    infoKey->setIcon(KIcon("kgpg-info-kgpg"));
    infoKey->setText(i18n("K&ey properties"));
    connect(infoKey, SIGNAL(triggered(bool)), SLOT(keyproperties()));
    infoKey->setShortcut(QKeySequence(Qt::Key_Return));

    QAction *editKey = actionCollection()->addAction("key_edit");
    editKey->setIcon(KIcon("kgpg-term-kgpg"));
    editKey->setText(i18n("Edit Key in &Terminal"));
    connect(editKey, SIGNAL(triggered(bool)), SLOT(slotedit()));
    editKey->setShortcut(QKeySequence(Qt::ALT+Qt::Key_Return));

    QAction *generateKey = actionCollection()->addAction("key_gener");
    generateKey->setIcon(KIcon("kgpg_gen"));
    generateKey->setText(i18n("&Generate Key Pair..."));
    connect(generateKey, SIGNAL(triggered(bool)), SLOT(slotGenerateKey()));
    generateKey->setShortcuts(KStandardShortcut::shortcut(KStandardShortcut::New));

    QAction *exportPublicKey = actionCollection()->addAction("key_export");
    exportPublicKey->setIcon(KIcon("kgpg_export"));
    exportPublicKey->setText(i18n("E&xport Public Keys..."));
    connect(exportPublicKey, SIGNAL(triggered(bool)), SLOT(slotexport()));
    exportPublicKey->setShortcuts(KStandardShortcut::shortcut(KStandardShortcut::Copy));

    QAction *importKey = actionCollection()->addAction("key_import");
    importKey->setIcon(KIcon("kgpg-import-kgpg"));
    importKey->setText(i18n("&Import Key..."));
    connect(importKey, SIGNAL(triggered(bool)), SLOT(slotPreImportKey()));
    importKey->setShortcuts(KStandardShortcut::shortcut(KStandardShortcut::Paste));

    QAction *newContact = actionCollection()->addAction("add_kab");
    newContact->setIcon(KIcon("office-addressbook"));
    newContact->setText(i18n("&Create New Contact in Address Book"));
    connect(newContact, SIGNAL(triggered(bool)), SLOT(addToKAB()));

    QAction *createGroup = actionCollection()->addAction("create_group");
    createGroup->setText(i18n("&Create Group with Selected Keys..."));
    connect(createGroup, SIGNAL(triggered(bool)), SLOT(createNewGroup()));

    editCurrentGroup = actionCollection()->addAction("edit_group");
    editCurrentGroup->setText(i18n("&Edit Group"));
    connect(editCurrentGroup, SIGNAL(triggered(bool)), SLOT(editGroup()));

    delGroup = actionCollection()->addAction("delete_group");
    delGroup->setText(i18n("&Delete Group"));
    connect(delGroup, SIGNAL(triggered(bool)), SLOT(deleteGroup()));

    deleteKey = actionCollection()->addAction("key_delete");
    deleteKey->setIcon(KIcon("edit-delete"));
    deleteKey->setText(i18n("&Delete Keys"));
    connect(deleteKey, SIGNAL(triggered(bool)), SLOT(confirmdeletekey()));
    deleteKey->setShortcut(QKeySequence(Qt::Key_Delete));

    setDefaultKey = actionCollection()->addAction("key_default");
    setDefaultKey->setText(i18n("Set as De&fault Key"));
    connect(setDefaultKey, SIGNAL(triggered(bool)), SLOT(slotSetDefKey()));
    QAction *addPhoto = actionCollection()->addAction("add_photo");
    addPhoto->setText(i18n("&Add Photo"));
    connect(addPhoto, SIGNAL(triggered(bool)), SLOT(slotAddPhoto()));
    QAction *addUid = actionCollection()->addAction("add_uid");
    addUid->setText(i18n("&Add User Id"));
    connect(addUid, SIGNAL(triggered(bool)), SLOT(slotAddUid()));
    QAction *exportSecretKey = actionCollection()->addAction("key_sexport");
    exportSecretKey->setText(i18n("Export Secret Key..."));
    connect(exportSecretKey, SIGNAL(triggered(bool)), SLOT(slotexportsec()));
    QAction *deleteKeyPair = actionCollection()->addAction("key_pdelete");
    deleteKeyPair->setText(i18n("Delete Key Pair"));
    connect(deleteKeyPair, SIGNAL(triggered(bool)), SLOT(deleteseckey()));
    QAction *revokeKey = actionCollection()->addAction("key_revoke");
    revokeKey->setText(i18n("Revoke Key..."));
    connect(revokeKey, SIGNAL(triggered(bool)), SLOT(revokeWidget()));
    QAction *regeneratePublic = actionCollection()->addAction("key_regener");
    regeneratePublic->setText(i18n("&Regenerate Public Key"));
    connect(regeneratePublic, SIGNAL(triggered(bool)), SLOT(slotregenerate()));
    QAction *delUid = actionCollection()->addAction("del_uid");
    delUid->setText(i18n("&Delete User Id"));
    connect(delUid, SIGNAL(triggered(bool)), SLOT(slotDelUid()));
    setPrimUid = actionCollection()->addAction("prim_uid");
    setPrimUid->setText(i18n("Set User Id as &primary"));
    connect(setPrimUid, SIGNAL(triggered(bool)), SLOT(slotPrimUid()));
    QAction *openPhoto = actionCollection()->addAction("key_photo");
    openPhoto->setIcon(KIcon("image-x-generic"));
    openPhoto->setText(i18n("&Open Photo"));
    connect(openPhoto, SIGNAL(triggered(bool)), SLOT(slotShowPhoto()));
    QAction *deletePhoto = actionCollection()->addAction("delete_photo");
    deletePhoto->setIcon(KIcon("edit-delete"));
    deletePhoto->setText(i18n("&Delete Photo"));
    connect(deletePhoto, SIGNAL(triggered(bool)), SLOT(slotDeletePhoto()));
    delSignKey = actionCollection()->addAction("key_delsign");
    delSignKey->setIcon(KIcon("edit-delete"));
    delSignKey->setText(i18n("Delete sign&ature(s)"));
    connect(delSignKey, SIGNAL(triggered(bool)), SLOT(delsignkey()));

    importAllSignKeys = actionCollection()->addAction("key_importallsign");
    importAllSignKeys->setIcon(KIcon("document-import"));
    importAllSignKeys->setText(i18n("Import &Missing Signatures From Keyserver"));
    connect(importAllSignKeys, SIGNAL(triggered(bool)), SLOT(importallsignkey()));
    refreshKey = actionCollection()->addAction("key_server_refresh");
    refreshKey->setIcon(KIcon("view-refresh"));
    refreshKey->setText(i18n("&Refresh Keys From Keyserver"));
    connect(refreshKey, SIGNAL(triggered(bool)), SLOT(refreshKeyFromServer()));
    signKey = actionCollection()->addAction("key_sign");
    signKey->setIcon(KIcon("kgpg-sign-kgpg"));
    signKey->setText(i18n("&Sign Keys..."));
    connect(signKey, SIGNAL(triggered(bool)), SLOT(signkey()));
    importSignatureKey = actionCollection()->addAction("key_importsign");
    importSignatureKey->setIcon(KIcon("document-import"));
    importSignatureKey->setText(i18n("Import key(s) from keyserver"));
    connect(importSignatureKey, SIGNAL(triggered(bool)), SLOT(preimportsignkey()));

    sTrust = actionCollection()->add<KToggleAction>("show_trust");
    sTrust->setText(i18n("Trust"));
    connect(sTrust, SIGNAL(triggered(bool) ), SLOT(slotShowTrust()));
    sSize = actionCollection()->add<KToggleAction>("show_size");
    sSize->setText(i18n("Size"));
    connect(sSize, SIGNAL(triggered(bool) ), SLOT(slotShowSize()));
    sCreat = actionCollection()->add<KToggleAction>("show_creat");
    sCreat->setText(i18n("Creation"));
    connect(sCreat, SIGNAL(triggered(bool) ), SLOT(slotShowCreation()));
    sExpi = actionCollection()->add<KToggleAction>("show_expi");
    sExpi->setText(i18n("Expiration"));
    connect(sExpi, SIGNAL(triggered(bool) ), SLOT(slotShowExpiration()));

    photoProps = actionCollection()->add<KSelectAction>("photo_settings");
    photoProps->setIcon(KIcon("kgpg_photo"));
    photoProps->setText(i18n("&Photo ID's"));

    // Keep the list in kgpg.kcfg in sync with this one!
    QStringList list;
    list.append(i18n("Disable"));
    list.append(i18nc("small picture", "Small"));
    list.append(i18nc("medium picture", "Medium"));
    list.append(i18nc("large picture", "Large"));
    photoProps->setItems(list);

    imodel = new KGpgItemModel(this);

    iproxy = new KeyListProxyModel(this);
    iproxy->setKeyModel(imodel);

    iview = new KeyTreeView(this, iproxy);
    connect(iview, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(defaultAction(const QModelIndex &)));
    iview->setSelectionMode(QAbstractItemView::ExtendedSelection);
    setCentralWidget(iview);
    for (int i = 0; i < 7; i++)
       iview->resizeColumnToContents(i);
    iview->setAlternatingRowColors(true);
    iview->setSortingEnabled(true);
    connect(iview, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(slotMenu(const QPoint &)));
    iview->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(iview->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), this, SLOT(checkList()));

    int psize = KGpgSettings::photoProperties();
    photoProps->setCurrentItem(psize);
    slotSetPhotoSize(psize);

    m_popuppub = new KMenu();
    m_popuppub->addAction(exportPublicKey);
    m_popuppub->addAction(deleteKey);
    m_popuppub->addAction(signKey);
    m_popuppub->addAction(infoKey);
    m_popuppub->addAction(editKey);
    m_popuppub->addAction(refreshKey);
    m_popuppub->addAction(createGroup);
    m_popuppub->addAction(setDefaultKey);
    m_popuppub->addSeparator();
    m_popuppub->addAction(importAllSignKeys);

    m_popupsec = new KMenu();
    m_popupsec->addAction(exportPublicKey);
    m_popupsec->addAction(signKey);
    m_popupsec->addAction(infoKey);
    m_popupsec->addAction(editKey);
    m_popupsec->addAction(refreshKey);
    m_popupsec->addAction(setDefaultKey);
    m_popupsec->addSeparator();
    m_popupsec->addAction(importAllSignKeys);
    m_popupsec->addSeparator();
    m_popupsec->addAction(addPhoto);
    m_popupsec->addAction(addUid);
    m_popupsec->addAction(exportSecretKey);
    m_popupsec->addAction(deleteKeyPair);
    m_popupsec->addAction(revokeKey);

    m_popupgroup = new KMenu();
    m_popupgroup->addAction(editCurrentGroup);
    m_popupgroup->addAction(delGroup);
    m_popupgroup->addAction(refreshKey);

    m_popupout = new KMenu();
    m_popupout->addAction(importKey);

    m_popupsig = new KMenu();
    m_popupsig->addAction(importSignatureKey);
    m_popupsig->addAction(delSignKey);

    m_popupphoto = new KMenu();
    m_popupphoto->addAction(openPhoto);
    m_popupphoto->addAction(deletePhoto);

    m_popupuid = new KMenu();
    m_popupuid->addAction(delUid);
    m_popupuid->addAction(setPrimUid);

    m_popuporphan = new KMenu();
    m_popuporphan->addAction(regeneratePublic);
    m_popuporphan->addAction(deleteKeyPair);

    editCurrentGroup->setEnabled(false);
    delGroup->setEnabled(false);
    createGroup->setEnabled(false);
    infoKey->setEnabled(false);
    editKey->setEnabled(false);
    signKey->setEnabled(false);
    refreshKey->setEnabled(false);
    exportPublicKey->setEnabled(false);
    newContact->setEnabled(false);

//     setCentralWidget(keysList2);
    keysList2->restoreLayout(KGlobal::config().data(), "KeyView");

    connect(keysList2, SIGNAL(returnPressed(Q3ListViewItem *)), this, SLOT(defaultAction()));
    connect(keysList2, SIGNAL(doubleClicked(Q3ListViewItem *, const QPoint &, int)), this, SLOT(defaultAction()));
    connect(keysList2, SIGNAL(destroyed()), this, SLOT(annule()));
    connect(photoProps, SIGNAL(activated(int)), this, SLOT(slotSetPhotoSize(int)));

    // get all keys data
    setupGUI(KXmlGuiWindow::Create | Save | ToolBar | StatusBar | Keys, "keysmanager.rc");
    toolBar()->addSeparator();

    QLabel *searchLabel = new QLabel(i18n("Search: "), this);
    m_listviewsearch = new KLineEdit(this);
    m_listviewsearch->setClearButtonShown(true);

    QWidget *searchWidget = new QWidget(this);
    QHBoxLayout *searchLayout = new QHBoxLayout(searchWidget);
    searchLayout->addWidget(searchLabel);
    searchLayout->addWidget(m_listviewsearch);
    searchLayout->addStretch();

    KAction *serchLineAction = new KAction(i18n("Search Line"), this);
    actionCollection()->addAction("search_line", serchLineAction);
    serchLineAction->setDefaultWidget(searchWidget);

    toolBar()->addAction(actionCollection()->action("search_line"));

    action = actionCollection()->addAction("search_focus");
    action->setText(i18n("Filter Search"));
    connect(action, SIGNAL(triggered(bool) ), m_listviewsearch, SLOT(setFocus()));
    action->setShortcut(QKeySequence(Qt::Key_F6));
    connect(m_listviewsearch, SIGNAL(textChanged(const QString &)), iproxy, SLOT(setFilterFixedString(const QString &)));

    sTrust->setChecked(KGpgSettings::showTrust());
    iview->setColumnHidden(2, !KGpgSettings::showTrust());
    sSize->setChecked(KGpgSettings::showSize());
    iview->setColumnHidden(3, !KGpgSettings::showSize());
    sCreat->setChecked(KGpgSettings::showCreat());
    iview->setColumnHidden(4, !KGpgSettings::showCreat());
    sExpi->setChecked(KGpgSettings::showExpi());
    iview->setColumnHidden(5, !KGpgSettings::showExpi());
    iproxy->setOnlySecret(KGpgSettings::showSecret());
    iproxy->setShowExpired(!KGpgSettings::hideExRev());

    m_statusbar = statusBar();
    m_statusbar->insertItem("", 0, 1);
    m_statusbar->insertPermanentFixedItem(i18n("00000 Keys, 000 Groups"), 1);
    m_statusbar->setItemAlignment(0, Qt::AlignLeft);
    m_statusbar->changeItem("", 1);

    connect(keysList2, SIGNAL(statusMessage(QString, int, bool)), this, SLOT(changeMessage(QString, int, bool)));
    connect(m_statusbartimer, SIGNAL(timeout()), this, SLOT(statusBarTimeout()));

    s_kgpgEditor = new KgpgEditor(parent, Qt::WType_Dialog, qobject_cast<KAction *>(actionCollection()->action("go_default_key"))->shortcut(), true);
    connect(s_kgpgEditor, SIGNAL(refreshImported(QStringList)), keysList2, SLOT(slotReloadKeys(QStringList)));
    connect(this, SIGNAL(fontChanged(QFont)), s_kgpgEditor, SLOT(slotSetFont(QFont)));
}

KeysManager::~KeysManager()
{
    delete imodel;
    delete iview;
}

void KeysManager::slotGenerateKey()
{
    KgpgKeyGenerate *kg = new KgpgKeyGenerate(this);
    if (kg->exec() == QDialog::Accepted)
    {
        if (!kg->isExpertMode())
        {
            KgpgInterface *interface = new KgpgInterface();
            connect(interface, SIGNAL(generateKeyStarted(KgpgInterface*)), this, SLOT(slotGenerateKeyProcess(KgpgInterface*)));
            connect(interface, SIGNAL(generateKeyFinished(int, KgpgInterface*, QString, QString, QString, QString)), this, SLOT(slotGenerateKeyDone(int, KgpgInterface*, QString, QString, QString, QString)));
            interface->generateKey(kg->name(), kg->email(), kg->comment(), kg->algo(), kg->size(), kg->expiration(), kg->days());
        }
        else
        {
            KConfigGroup config(KGlobal::config(), "General");

            QString terminalApp = config.readPathEntry("TerminalApplication", "konsole");
            QStringList args;
            args << "-e" << KGpgSettings::gpgBinaryPath() << "--gen-key";

            QProcess *genKeyProc = new QProcess(this);
            genKeyProc->start(terminalApp, args);
            genKeyProc->waitForFinished();
            refreshkey();
        }
    }

    delete kg;
}

void KeysManager::showKeyManager()
{
    show();
}

void KeysManager::slotOpenEditor()
{
    KgpgEditor *kgpgtxtedit = new KgpgEditor(this, Qt::Window, qobject_cast<KAction *>(actionCollection()->action("go_default_key"))->shortcut());
    kgpgtxtedit->setAttribute(Qt::WA_DeleteOnClose);

    connect(kgpgtxtedit, SIGNAL(refreshImported(QStringList)), keysList2, SLOT(slotReloadKeys(QStringList)));
    connect(kgpgtxtedit, SIGNAL(encryptFiles(KUrl::List)), this, SIGNAL(encryptFiles(KUrl::List)));
    connect(this, SIGNAL(fontChanged(QFont)), kgpgtxtedit, SLOT(slotSetFont(QFont)));

    kgpgtxtedit->show();
}

void KeysManager::statusBarTimeout()
{
    if (m_statusbar)
        m_statusbar->changeItem("", 0);
}

void KeysManager::changeMessage(const QString &msg, const int nb, const bool keep)
{
    m_statusbartimer->stop();
    if (m_statusbar)
    {
        if ((nb == 0) && (!keep))
            m_statusbartimer->start(10000);
        m_statusbar->changeItem(' ' + msg + ' ', nb);
    }
}

void KeysManager::slotGenerateKeyProcess(KgpgInterface *)
{
    pop = new KPassivePopup(this);
    pop->setTimeout(0);

    KVBox *passiveBox = pop->standardView(i18n("Generating new key pair."), QString(), Images::kgpg(), 0);

    QMovie anim(KStandardDirs::locate("appdata", "pics/kgpg_anim.gif"));
    QLabel *text1 = new QLabel(passiveBox);
    text1->setAlignment(Qt::AlignHCenter);
    text1->setMovie(&anim);

    QLabel *text2 = new QLabel(passiveBox);
    text2->setText(i18n("\nPlease wait..."));

    pop->setView(passiveBox);
    pop->show();

    QRect qRect(QApplication::desktop()->screenGeometry());
    int Xpos = qRect.width() / 2 - pop->width() / 2;
    int Ypos = qRect.height() / 2 - pop->height() / 2;
    pop->move(Xpos, Ypos);

    pop->setAutoDelete(false);
    changeMessage(i18n("Generating New Key..."), 0, true);
}

void KeysManager::slotGenerateKeyDone(int res, KgpgInterface *interface, const QString &name, const QString &email, const QString &id, const QString &fingerprint)
{
    delete interface;
    changeMessage(i18nc("Application ready for user input", "Ready"), 0);

    if (res == 1)
    {
        QString infomessage = i18n("Generating new key pair");
        QString infotext = i18n("Bad passphrase. Cannot generate a new key pair.");
        KMessageBox::error(this, infotext, infomessage);
    }
    else
    if (res == 3)
    {
        QString infomessage = i18n("Generating new key pair");
        QString infotext = i18n("Aborted by the user. Cannot generate a new key pair.");
        KMessageBox::error(this, infotext, infomessage);
    }
    else
    if (res == 4)
    {
        QString infomessage = i18n("Generating new key pair");
        QString infotext = i18n("The email address is not valid. Cannot generate a new key pair.");
        KMessageBox::error(this, infotext, infomessage);
    }
    else
    if (res == 10)
    {
        QString infomessage = i18n("Generating new key pair");
        QString infotext = i18n("The name is not accepted by gpg. Cannot generate a new key pair.");
        KMessageBox::error(this, infotext, infomessage);
    }
    else
    if (res != 2)
    {
        QString infomessage = i18n("Generating new key pair");
        QString infotext = i18n("gpg process did not finish. Cannot generate a new key pair.");
        KMessageBox::error(this, infotext, infomessage);
    }
    else
    {
        changeMessage(keysList2->statusCountMessage(), 1);

        KDialog *keyCreated = new KDialog(this);
        keyCreated->setCaption(i18n("New Key Pair Created"));
        keyCreated->setButtons(KDialog::Ok);
        keyCreated->setDefaultButton(KDialog::Ok);
        keyCreated->setModal(true);

        newKey *page = new newKey(keyCreated);
        page->TLname->setText("<b>" + name + "</b>");
        page->TLemail->setText("<b>" + email + "</b>");

	QString revurl;
        QString gpgPath = KGpgSettings::gpgConfigPath();
        if (!gpgPath.isEmpty())
            revurl = KUrl::fromPath(gpgPath).directory(KUrl::AppendTrailingSlash);
	else
            revurl = QDir::homePath() + '/';

        if (!email.isEmpty())
            page->kURLRequester1->setUrl(revurl + email.section("@", 0, 0) + ".revoke");
        else
            page->kURLRequester1->setUrl(revurl + email.section(" ", 0, 0) + ".revoke");

        page->TLid->setText("<b>" + id + "</b>");
        page->LEfinger->setText(fingerprint);
        page->CBdefault->setChecked(true);
        page->show();
        keyCreated->setMainWidget(page);

        delete pop;
        pop = 0;

        keyCreated->exec();

            if (page->CBdefault->isChecked())
                imodel->setDefaultKey(fingerprint);

            iview->selectNode(imodel->getRootNode()->findKey(fingerprint));

        if (page->CBsave->isChecked())
        {
            slotrevoke(id, page->kURLRequester1->url().path(), 0, i18n("backup copy"));
            if (page->CBprint->isChecked())
                connect(revKeyProcess, SIGNAL(revokeurl(QString)), this, SLOT(doFilePrint(QString)));
        }
        else
        if (page->CBprint->isChecked())
        {
            slotrevoke(id, QString(), 0, i18n("backup copy"));
            connect(revKeyProcess, SIGNAL(revokecertificate(QString)), this, SLOT(doPrint(QString)));
        }
        keysList2->refreshKeys(QStringList(id));
    }
}

void KeysManager::slotShowTrust()
{
    iview->setColumnHidden(KEYCOLUMN_TRUST, !sTrust->isChecked());
}

void KeysManager::slotShowExpiration()
{
    iview->setColumnHidden(KEYCOLUMN_EXPIR, !sExpi->isChecked());
}

void KeysManager::slotShowSize()
{
    iview->setColumnHidden(KEYCOLUMN_SIZE, !sSize->isChecked());
}

void KeysManager::slotShowCreation()
{
    iview->setColumnHidden(KEYCOLUMN_CREAT, !sCreat->isChecked());
}

void KeysManager::slotToggleSecret(bool b)
{
    iproxy->setOnlySecret(b);
}

void KeysManager::slotToggleDisabled(bool b)
{
    iproxy->setShowExpired(!b);
}

bool KeysManager::eventFilter(QObject *, QEvent *e)
{
    if ((e->type() == QEvent::Show) && (showTipOfDay))
    {
        KTipDialog::showTip(this, QString("kgpg/tips"), false);
        showTipOfDay = false;
    }

    return false;
}

void KeysManager::slotGotoDefaultKey()
{
    iview->selectNode(imodel->getRootNode()->findKey(KGpgSettings::defaultKey()));
}

void KeysManager::refreshKeyFromServer()
{
    QList<KGpgNode *> keysList = iview->selectedNodes();
    if (keysList.isEmpty())
        return;

    QStringList keyIDS;

    for (int i = 0; i < keysList.count(); ++i)
    {
        KGpgNode *item = keysList.at(i);

            if (item->getType() == ITYPE_GROUP)
            {
                for (int j = 0; j < item->getChildCount(); j++)
                    keyIDS << item->getChild(j)->getId();

                continue;
            }

            if (item->getType() & ITYPE_PAIR)
                keyIDS << item->getId();
            else
            {
                KMessageBox::sorry(this, i18n("You can only refresh primary keys. Please check your selection."));
                return;
            }
    }

    kServer = new KeyServer(this, false);
    connect(kServer, SIGNAL(importFinished(QStringList)), this, SLOT(refreshFinished(QStringList)));
    kServer->refreshKeys(keyIDS);
}

void KeysManager::refreshFinished(const QStringList &ids)
{
    if (kServer)
        kServer = 0L;

    for (int i = 0; i < ids.count(); ++i)
        keysList2->refreshcurrentkey(ids.at(i));
}

void KeysManager::slotDelUid()
{
    KeyListViewItem *uitem = keysList2->currentItem();
    KeyListViewItem *item = uitem;
    while (item->depth()>0)
        item = item->parent();

    QProcess *process = new QProcess(this);
    KConfigGroup config(KGlobal::config(), "General");
    QString terminalApp = config.readPathEntry("TerminalApplication", "konsole");
    QStringList args;
    args << "-e" << KGpgSettings::gpgBinaryPath();
    args << "--edit-key" << item->keyId() << "uid" << uitem->text(6) << "deluid";
    process->start(terminalApp, args);
    process->waitForFinished();
    keysList2->refreshcurrentkey(item);
}

void KeysManager::slotPrimUid()
{
    KeyListViewItem *uitem = keysList2->currentItem();
    KeyListViewItem *item = uitem;
    while (item->depth()>0)
        item = item->parent();

    QProcess *process = new QProcess(this);
    KConfigGroup config(KGlobal::config(), "General");
    QString terminalApp = config.readPathEntry("TerminalApplication", "konsole");
    QStringList args;
    args << "-e" << KGpgSettings::gpgBinaryPath();
    args << "--edit-key" << item->keyId() << "uid" << uitem->text(6) << "primary" << "save";
    process->start(terminalApp, args);
    process->waitForFinished();
    keysList2->refreshcurrentkey(item);
}

void KeysManager::slotregenerate()
{
	QString regID = keysList2->currentItem()->keyId();
	KProcess *p1, *p2, *p3;

	p1 = new KProcess(this);
	*p1 << KGpgSettings::gpgBinaryPath() << "--no-secmem-warning" << "--export-secret-key" << regID;
	p1->setOutputChannelMode(KProcess::OnlyStdoutChannel);

	p2 = new KProcess(this);
	*p2 << "gpgsplit" << "--no-split" << "--secret-to-public";
	p2->setOutputChannelMode(KProcess::OnlyStdoutChannel);

	p3 = new KProcess(this);
	*p3 << KGpgSettings::gpgBinaryPath() << "--import";

	p1->setStandardOutputProcess(p2);
	p2->setStandardOutputProcess(p3);

	p1->start();
	p2->start();
	p3->start();

	p1->waitForFinished();
	p2->waitForFinished();
	p3->waitForFinished();

	delete p1;
	delete p2;
	delete p3;

	keysList2->takeItem(keysList2->currentItem());
	keysList2->refreshKeys(QStringList(regID));
}

void KeysManager::slotAddUid()
{
    addUidWidget = new KDialog(this );
    addUidWidget->setCaption( i18n("Add New User Id") );
    addUidWidget->setButtons(  KDialog::Ok | KDialog::Cancel );
    addUidWidget->setDefaultButton(  KDialog::Ok );
    addUidWidget->setModal( true );
    addUidWidget->enableButtonOk(false);
    AddUid *keyUid = new AddUid(0);
    addUidWidget->setMainWidget(keyUid);
    //keyUid->setMinimumSize(keyUid->sizeHint());
    keyUid->setMinimumWidth(300);

    connect(keyUid->kLineEdit1, SIGNAL(textChanged(const QString & )), this, SLOT(slotAddUidEnable(const QString & )));
    if (addUidWidget->exec() != QDialog::Accepted)
        return;

    KgpgInterface *addUidProcess = new KgpgInterface();
    connect(addUidProcess,SIGNAL(addUidFinished(int, KgpgInterface*)), this ,SLOT(slotAddUidFin(int, KgpgInterface*)));
    addUidProcess->addUid(iview->selectedNode()->getId(), keyUid->kLineEdit1->text(), keyUid->kLineEdit2->text(), keyUid->kLineEdit3->text());
}

void KeysManager::slotAddUidFin(int res, KgpgInterface *interface)
{
    // TODO tester les res
    kDebug(2100) << "Resultat : " << res ;
    delete interface;
    keysList2->refreshselfkey();
}

void KeysManager::slotAddUidEnable(const QString & name)
{
    addUidWidget->enableButtonOk(name.length() > 4);
}

void KeysManager::slotAddPhoto()
{
    KgpgLibrary *lib = new KgpgLibrary();
    connect (lib, SIGNAL(photoAdded()), this, SLOT(slotUpdatePhoto()));
    lib->addPhoto(iview->selectedNode()->getId());
}

void KeysManager::slotDeletePhoto()
{
    KGpgNode *nd = iview->selectedNode();
    Q_ASSERT(nd->getType() == ITYPE_UAT);
    KGpgUatNode *und = static_cast<KGpgUatNode *>(nd);
    KGpgKeyNode *parent = und->getParentKeyNode();

    QString mess = i18n("<qt>Are you sure you want to delete Photo id <b>%1</b><br/>from key <b>%2 &lt;%3&gt;</b>?</qt>",
                        und->getId(), parent->getName(), parent->getEmail());

    /*
    if (KMessageBox::warningContinueCancel(this, mess, QString(), KGuiItem(i18n("Delete"), "edit-delete")) != KMessageBox::Continue)
        return;
    */

    KgpgInterface *interface = new KgpgInterface();
    connect(interface, SIGNAL(delPhotoFinished(int, KgpgInterface*)), this, SLOT(slotDelPhotoFinished(int, KgpgInterface*)));
    interface->deletePhoto(parent->getId(), und->getId());
}

void KeysManager::slotDelPhotoFinished(int res, KgpgInterface *interface)
{
    delete interface;

    // TODO : add res == 3 (bad passphrase)

    if (res == 2)
        slotUpdatePhoto();

}

void KeysManager::slotUpdatePhoto()
{
    keysList2->refreshselfkey();
}

void KeysManager::slotSetPhotoSize(int size)
{
    switch(size)
    {
        case 1:
            showPhoto = true;
            keysList2->setPreviewSize(22);
            imodel->setPreviewSize(22);
            break;
        case 2:
            showPhoto = true;
            keysList2->setPreviewSize(42);
            imodel->setPreviewSize(42);
            break;
        case 3:
            showPhoto = true;
            keysList2->setPreviewSize(65);
            imodel->setPreviewSize(65);
            break;
        default:
            showPhoto = false;
            imodel->setPreviewSize(0);
            break;
    }
    keysList2->setDisplayPhoto(showPhoto);

    // refresh keys with photo id
    KeyListViewItem *newdef = keysList2->firstChild();
    while (newdef)
    {
        //if ((keysList2->photoKeysList.find(newdef->text(6))!=-1) && (newdef->childCount ()>0))
        if (newdef->childCount() > 0)
        {
            bool hasphoto = false;
            KeyListViewItem *newdefChild = newdef->firstChild();
            while (newdefChild)
            {
                if (newdefChild->itemType() == KeyListViewItem::Uat)
                {
                    hasphoto = true;
                    break;
                }
                newdefChild = newdefChild->nextSibling();
            }

            if (hasphoto)
            {
                while (newdef->firstChild())
                    delete newdef->firstChild();
                keysList2->expandKey(newdef);
            }
        }
        newdef = newdef->nextSibling();
    }
}

void KeysManager::findKey()
{
    KFindDialog fd(this);
    if (fd.exec() != QDialog::Accepted)
        return;

    searchString = fd.pattern();
    searchOptions = fd.options();
    findFirstKey();
}

void KeysManager::findFirstKey()
{
    if (searchString.isEmpty())
        return;

    bool foundItem = true;
    KeyListViewItem *item = keysList2->firstChild();
    if (!item)
        return;
    QString searchText = item->text(0) + ' ' + item->text(1) + ' ' + item->text(6);

    KFind *m_find = new KFind(searchString, searchOptions, this);
    m_find->setData(searchText);
    while (m_find->find() == KFind::NoMatch)
    {
        if (!item->nextSibling())
        {
            foundItem = false;
            break;
        }
        else
        {
            item = item->nextSibling();
            searchText = item->text(0) + ' ' + item->text(1) + ' ' + item->text(6);
            m_find->setData(searchText);
        }
    }
    delete m_find;

    if (foundItem)
    {
        keysList2->clearSelection();
        keysList2->setCurrentItem(item);
        keysList2->setSelected(item, true);
        keysList2->ensureItemVisible(item);
    }
    else
        KMessageBox::sorry(this, i18n("<qt>Search string '<b>%1</b>' not found.</qt>", searchString));
}

void KeysManager::findNextKey()
{
    //kDebug(2100)<<"find next";
    if (searchString.isEmpty())
    {
        findKey();
        return;
    }

    bool foundItem = true;
    KeyListViewItem *item = keysList2->currentItem();
    if (!item)
        return;

    while(item->depth() > 0)
        item = item->parent();
    item = item->nextSibling();

    QString searchText = item->text(0) + ' ' + item->text(1) + ' ' + item->text(6);
    //kDebug(2100) << "Next string:" << searchText ;
    //kDebug(2100) << "Search:" << searchString ;
    //kDebug(2100) << "OPts:" << searchOptions ;
    KFind *m_find = new KFind(searchString, searchOptions, this);
    m_find->setData(searchText);
    while (m_find->find() == KFind::NoMatch)
    {
        if (!item->nextSibling())
        {
            foundItem = false;
            break;
        }
        else
        {
            item = item->nextSibling();
            searchText = item->text(0) + ' ' + item->text(1) + ' ' + item->text(6);
            m_find->setData(searchText);
            //kDebug(2100) << "Next string:" << searchText ;
        }
    }
    delete m_find;

    if (foundItem)
    {
        keysList2->clearSelection();
        keysList2->setCurrentItem(item);
        keysList2->setSelected(item,true);
        keysList2->ensureItemVisible(item);
    }
    else
        findFirstKey();
}

void KeysManager::addToKAB()
{
    KABC::Key key;
    KGpgNode *nd = iview->selectedNode();
    if (nd == NULL)
        return;

    QString email = nd->getEmail();

    KABC::AddressBook *ab = KABC::StdAddressBook::self();
    if (!ab->load())
    {
        KMessageBox::sorry(this, i18n("Unable to contact the address book. Please check your installation."));
        return;
    }

    KABC::Addressee::List addresseeList = ab->findByEmail(email);
    KToolInvocation::startServiceByDesktopName("kaddressbook");
    QDBusInterface kaddressbook("org.kde.kaddressbook", "/KAddressBook", "org.kde.KAddressbook.Core");
    if(!addresseeList.isEmpty())
        kaddressbook.call( "showContactEditor", addresseeList.first().uid());
    else
        kaddressbook.call( "addEmail", nd->getName() + " <" + email + '>');
}

/*
void KeysManager::allToKAB()
{
        KABC::Key key;
        QString email;
        QStringList keylist;
        KABC::Addressee a;

        KABC::AddressBook *ab = KABC::StdAddressBook::self();
        if ( !ab->load() ) {
                KMessageBox::sorry(this,i18n("Unable to contact the address book. Please check your installation."));
                return;
        }

        KeyListViewItem * myChild = keysList2->firstChild();
        while( myChild ) {
                //email=extractKeyMail(myChild).simplified();
                email=myChild->text(1);
                KABC::Addressee::List addressees = ab->findByEmail( email );
                if (addressees.count()==1) {
                        a=addressees.first();
                        KgpgInterface *ks=new KgpgInterface();
                        key.setTextData(ks->getKey(myChild->text(6),true));
                        a.insertKey(key);
                        ab->insertAddressee(a);
                        keylist<<myChild->text(6)+": "+email;
                }
                //            doSomething( myChild );
                myChild = myChild->nextSibling();
        }
        KABC::StdAddressBook::save();
        if (!keylist.isEmpty())
                KMessageBox::informationList(this,i18n("The following keys were exported to the address book:"),keylist);
        else
                KMessageBox::sorry(this,i18n("No entry matching your keys were found in the address book."));
}
*/

void KeysManager::slotManpage()
{
    KToolInvocation::startServiceByDesktopName("khelpcenter", QString("man:/gpg"), 0, 0, 0, "", true);
}

void KeysManager::slotTip()
{
    KTipDialog::showTip(this, QString("kgpg/tips"), true);
}

void KeysManager::closeEvent (QCloseEvent *e)
{
    // kapp->ref(); // prevent KXmlGuiWindow from closing the app
    // KXmlGuiWindow::closeEvent(e);
    e->accept();
    // hide();
    // e->ignore();
}

void KeysManager::showKeyServer()
{
    KeyServer *ks = new KeyServer(this);
    connect(ks, SIGNAL(importFinished(QStringList)), keysList2, SLOT(refreshKeys(QStringList)));
    ks->exec();
    delete ks;
    refreshkey();
}

void KeysManager::checkList()
{
    QList<KGpgNode *> exportList = iview->selectedNodes();
    if (exportList.count() > 1)
    {
        stateChanged("multi_selected");
    }
    else
    {
        if (exportList.at(0)->getType() == ITYPE_GROUP)
            stateChanged("group_selected");
        else
            stateChanged("single_selected");

    }

    switch (exportList.at(0)->getType()) {
    case ITYPE_PUBLIC:
		changeMessage(i18n("Public Key"), 0);
		break;
    case ITYPE_SUB:
		changeMessage(i18n("Sub Key"), 0);
		break;
    case ITYPE_PAIR:
		changeMessage(i18n("Secret Key Pair"), 0);
		break;
    case ITYPE_GROUP:
		changeMessage(i18n("Key Group"), 0);
		break;
    case ITYPE_SIGN:
		changeMessage(i18n("Signature"), 0);
		break;
    case ITYPE_UID:
		changeMessage(i18n("User ID"), 0);
		break;
    case ITYPE_REVSIGN:
		changeMessage(i18n("Revocation Signature"), 0);
		break;
    case ITYPE_UAT:
		changeMessage(i18n("Photo ID"), 0);
		break;
    case ITYPE_SECRET:
		changeMessage(i18n("Orphaned Secret Key"), 0);
		break;
    case ITYPE_GPUBLIC:
    case ITYPE_GSECRET:
    case ITYPE_GPAIR:
		changeMessage(i18n("Group member"), 0);
		break;
    default:
kDebug(3125) << "Oops, unmatched type value" << exportList.at(0)->getType();
    }
}

void KeysManager::annule()
{
    // close window
    close();
}

void KeysManager::quitApp()
{
    // close window
    saveToggleOpts();
    qApp->quit();
}

void KeysManager::saveToggleOpts(void)
{
    keysList2->saveLayout(KGlobal::config().data(), "KeyView");
    KGpgSettings::setPhotoProperties(photoProps->currentItem());
    KGpgSettings::setShowTrust(sTrust->isChecked());
    KGpgSettings::setShowExpi(sExpi->isChecked());
    KGpgSettings::setShowCreat(sCreat->isChecked());
    KGpgSettings::setShowSize(sSize->isChecked());
    KGpgSettings::setHideExRev(hExRev->isChecked());
    KGpgSettings::setShowSecret(hPublic->isChecked());
    KGpgSettings::self()->writeConfig();
}

void KeysManager::readOptions()
{
    m_clipboardmode = QClipboard::Clipboard;
    if (KGpgSettings::useMouseSelection() && (kapp->clipboard()->supportsSelection()))
        m_clipboardmode = QClipboard::Selection;

    // re-read groups in case the config file location was changed
    QStringList groups = KgpgInterface::getGpgGroupNames(KGpgSettings::gpgConfigPath());
    KGpgSettings::setGroups(groups.join(","));
    keysList2->groupNb = groups.count();
    changeMessage(keysList2->statusCountMessage(), 1);

    showTipOfDay = KGpgSettings::showTipOfDay();
}

void KeysManager::showOptions()
{
    if (KConfigDialog::showDialog("settings"))
        return;

    kgpgOptions *optionsDialog = new kgpgOptions(this, "settings");
    connect(optionsDialog, SIGNAL(settingsUpdated()), this, SLOT(readAllOptions()));
    connect(optionsDialog, SIGNAL(homeChanged()), this, SLOT(refreshkey()));
    connect(optionsDialog, SIGNAL(refreshTrust(int, QColor)), keysList2, SLOT(refreshTrust(int, QColor)));
    connect(optionsDialog, SIGNAL(changeFont(QFont)), this, SIGNAL(fontChanged(QFont)));
    optionsDialog->exec();
    delete optionsDialog;
}

void KeysManager::readAllOptions()
{
    readOptions();
    emit readAgainOptions();
}

void KeysManager::slotSetDefKey()
{
    slotSetDefaultKey(iview->selectedNode()->getId());
}

void KeysManager::slotSetDefaultKey(const QString &newID)
{
    if (newID == KGpgSettings::defaultKey())
      return;

    KeyListViewItem *newdef = keysList2->findItemByKeyId(newID);
    if (newdef == NULL) {
      kDebug(3125) << "key with id" << newID << "not found in keys list";
      return;
    }

    return slotSetDefaultKey(newdef);
}

void KeysManager::slotSetDefaultKey(KeyListViewItem *newdef)
{
    if ((newdef->trust() != TRUST_FULL) &&
        (newdef->trust() != TRUST_ULTIMATE))
    {
        KMessageBox::sorry(this, i18n("<qt>Sorry, the key <b>%1</b> is not valid for encryption or not trusted.</qt>", newdef->keyId()));
        return;
    }

    KeyListViewItem *olddef = keysList2->findItemByKeyId(KGpgSettings::defaultKey());

    if (olddef == newdef)
        return;

    KGpgSettings::setDefaultKey(newdef->keyId());
    KGpgSettings::self()->writeConfig();
    if (olddef)
        keysList2->refreshcurrentkey(olddef);
    keysList2->refreshcurrentkey(newdef);
    keysList2->ensureItemVisible(keysList2->currentItem());
}

bool KeysManager::isSignature(KeyListViewItem *item)
{
	return (item->itemType() & KeyListViewItem::Sign);
}

bool KeysManager::isSignatureUnknown(KeyListViewItem *item)
{
	if (!isSignature(item))
		return false;
	// ugly hack to detect unknown keys
	return (item->text(0).startsWith('[') && item->text(0).endsWith(']'));
}

void
KeysManager::slotMenu(const QPoint &pos)
{
	QPoint globpos = iview->mapToGlobal(pos);
	bool sametype;
	KgpgItemType itype;
	QList<KGpgNode *> ndlist = iview->selectedNodes(&sametype, &itype);
	bool unksig = false;
	QStringList l;
	int cnt = ndlist.count();

	// find out if an item has unknown signatures. Only check if the item has been
	// expanded before as expansion is very expensive and can take several seconds
	// that will freeze the UI meanwhile.
	for (int i = 0; i < cnt; i++) {
		KGpgNode *nd = ndlist.at(i);

		if (!nd->hasChildren())
			continue;

		KGpgExpandableNode *exnd = static_cast<KGpgExpandableNode *>(nd);
		if (!exnd->wasExpanded()) {
			unksig = true;
			break;
		}
		getMissingSigs(&l, exnd);
		if (!l.isEmpty()) {
			unksig = true;
			break;
		}
	}
	importAllSignKeys->setEnabled(unksig);

	if (itype == ITYPE_SIGN) {
#warning port me
// 		importSignatureKey->setEnabled(allunksig);
		delSignKey->setEnabled( (cnt == 1) );
		m_popupsig->exec(globpos);
	} else if ((itype == ITYPE_UID) && (cnt == 1)) {
		KGpgKeyNode *knd = static_cast<KGpgUidNode *>(ndlist.at(0))->getParentKeyNode();
		setPrimUid->setVisible(knd->getType() & ITYPE_SECRET);
		m_popupuid->exec(globpos);
	} else if ((itype == ITYPE_UAT) && (cnt == 1)) {
		m_popupphoto->exec(globpos);
	} else if ((itype == ITYPE_PAIR) && (cnt == 1)) {
		m_popupsec->exec(globpos);
	} else if ((itype == ITYPE_SECRET) && (cnt == 1)) {
		m_popuporphan->exec(globpos);
	} else if (itype == ITYPE_GROUP) {
		delGroup->setEnabled( (cnt == 1) );
		editCurrentGroup->setEnabled( (cnt == 1) );
		m_popupgroup->exec(globpos);
	} else if (!(itype & ~(ITYPE_PAIR | ITYPE_GROUP))) {
		signKey->setEnabled(!(itype & ITYPE_GROUP));
		deleteKey->setEnabled(!(itype & ITYPE_GROUP));
		setDefaultKey->setEnabled( (cnt == 1) );
		m_popuppub->exec(globpos);
	} else {
		m_popupout->exec(globpos);
	}
}

void KeysManager::slotrevoke(const QString &keyID, const QString &revokeUrl, const int reason, const QString &description)
{
    revKeyProcess = new KgpgInterface();
    revKeyProcess->KgpgRevokeKey(keyID, revokeUrl, reason, description);
}

void KeysManager::revokeWidget()
{
    KDialog *keyRevokeWidget = new KDialog(this);
    KGpgNode *nd = iview->selectedNode();

    keyRevokeWidget->setCaption(  i18n("Create Revocation Certificate") );
    keyRevokeWidget->setButtons(  KDialog::Ok | KDialog::Cancel );
    keyRevokeWidget->setDefaultButton(  KDialog::Ok );
    keyRevokeWidget->setModal( true );
    KgpgRevokeWidget *keyRevoke = new KgpgRevokeWidget();

    keyRevoke->keyID->setText(i18nc("<Name> (<Email>) ID: <KeyId>", "%1 (%2) ID: %3", nd->getName(), nd->getEmail(), nd->getId()));
    keyRevoke->kURLRequester1->setUrl(QDir::homePath() + '/' + nd->getEmail().section('@', 0, 0) + ".revoke");
    keyRevoke->kURLRequester1->setMode(KFile::File);

    keyRevoke->setMinimumSize(keyRevoke->sizeHint());
    keyRevoke->show();
    keyRevokeWidget->setMainWidget(keyRevoke);

    if (keyRevokeWidget->exec() != QDialog::Accepted)
        return;

    if (keyRevoke->cbSave->isChecked())
    {
        slotrevoke(nd->getId(), keyRevoke->kURLRequester1->url().path(), keyRevoke->comboBox1->currentIndex(), keyRevoke->textDescription->toPlainText());
        if (keyRevoke->cbPrint->isChecked())
            connect(revKeyProcess, SIGNAL(revokeurl(QString)), this, SLOT(doFilePrint(QString)));
        if (keyRevoke->cbImport->isChecked())
            connect(revKeyProcess, SIGNAL(revokeurl(QString)), this, SLOT(slotImportRevoke(QString)));
    }
    else
    {
        slotrevoke(nd->getId(), QString(), keyRevoke->comboBox1->currentIndex(), keyRevoke->textDescription->toPlainText());
        if (keyRevoke->cbPrint->isChecked())
            connect(revKeyProcess, SIGNAL(revokecertificate(QString)), this, SLOT(doPrint(QString)));
        if (keyRevoke->cbImport->isChecked())
            connect(revKeyProcess, SIGNAL(revokecertificate(QString)), this, SLOT(slotImportRevokeTxt(QString)));
    }
}

void KeysManager::slotImportRevoke(const QString &url)
{
    KgpgInterface *importKeyProcess = new KgpgInterface();
    connect(importKeyProcess, SIGNAL(importKeyFinished(QStringList)), keysList2, SLOT(refreshselfkey()));
    importKeyProcess->importKey(KUrl(url));
}

void KeysManager::slotImportRevokeTxt(const QString &revokeText)
{
    KgpgInterface *importKeyProcess = new KgpgInterface();
    connect(importKeyProcess, SIGNAL(importKeyFinished(QStringList)), keysList2, SLOT(refreshselfkey()));
    importKeyProcess->importKey(revokeText);
}

void KeysManager::slotexportsec()
{
    // export secret key
    QString warn = i18n("<qt>Secret keys <b>should not</b> be saved in an unsafe place.<br/>"
                        "If someone else can access this file, encryption with this key will be compromised!<br/>Continue key export?</qt>");
    int result = KMessageBox::warningContinueCancel(this, warn);
    if (result != KMessageBox::Continue)
        return;
    KGpgNode *nd = iview->selectedNode();

    QString sname = nd->getEmail().section('@', 0, 0).section('.', 0, 0);
    if (sname.isEmpty())
        sname = nd->getName().section(' ', 0, 0);
    sname.append(".asc");
    sname.prepend(QDir::homePath() + '/');
    KUrl url = KFileDialog::getSaveUrl(sname, "*.asc|*.asc Files", this, i18n("Export PRIVATE KEY As"));

    if(!url.isEmpty())
    {
        QFile fgpg(url.path());
        if (fgpg.exists())
            fgpg.remove();

        KProcess p;
        p << KGpgSettings::gpgBinaryPath() << "--no-tty" << "--output" << QFile::encodeName(url.path()) << "--armor" << "--export-secret-keys" << nd->getId();
        p.execute();

        if (fgpg.exists())
            KMessageBox::information(this, i18n("<qt>Your <b>private</b> key \"%1\" was successfully exported to<br/>%2 .<br/>"
                                                "<b>Do not</b> leave it in an insecure place.</qt>", nd->getId(), url.path()));
        else
            KMessageBox::sorry(this, i18n("Your secret key could not be exported.\nCheck the key."));
    }
}

void KeysManager::slotexport()
{
    bool same;
    KgpgItemType tp;

    QList<KGpgNode *> ndlist = iview->selectedNodes(&same, &tp);
    if (ndlist.isEmpty())
        return;
    if (!(tp & ITYPE_PUBLIC) || (tp & ~ITYPE_GPAIR))
        return;

    QString sname;

    if (ndlist.count() == 1)
    {
        sname = ndlist.at(0)->getEmail().section('@', 0, 0).section('.', 0, 0);
        if (sname.isEmpty())
            sname = ndlist.at(0)->getName().section(' ', 0, 0);
    }
    else
        sname = "keyring";

	QStringList klist;
	for (int i = 0; i < ndlist.count(); ++i) {
		klist << ndlist.at(i)->getId();
	}

    sname.append(".asc");
    sname.prepend(QDir::homePath() + '/');

    KDialog *dial = new KDialog(this );
    dial->setCaption(  i18n("Public Key Export") );
    dial->setButtons( KDialog::Ok | KDialog::Cancel );
    dial->setDefaultButton( KDialog::Ok );
    dial->setModal( true );

    KConfig *m_config = new KConfig("kgpgrc", KConfig::SimpleConfig);
    KConfigGroup gr = m_config->group("Servers");
    QString servers = gr.readEntry("Server_List");
    delete m_config;

    QStringList *serverList = new QStringList(servers.split(","));

    KeyExport *page = new KeyExport(dial, serverList);
    delete serverList;

    dial->setMainWidget(page);
    page->newFilename->setUrl(sname);
    page->newFilename->setWindowTitle(i18n("Save File"));
    page->newFilename->setMode(KFile::File);
    page->show();

    if (dial->exec() == QDialog::Accepted)
    {
        // export to file
        QString *exportAttr;

	if (page->checkAttrAll->isChecked())
		exportAttr = NULL;
	else if (page->checkAttrPhoto->isChecked())
		exportAttr = new QString("no-export-attributes");
	else
		exportAttr = new QString("export-minimal");
        if (page->checkServer->isChecked())
        {
            KeyServer *expServer = new KeyServer(0, false);
            expServer->slotSetExportAttribute(exportAttr);
            expServer->slotSetKeyserver(page->destServer->currentText());

            expServer->slotExport(klist.join(" "));
        }
        else
        if (page->checkFile->isChecked())
        {
            QString expname = page->newFilename->url().path().simplified();
            if (!expname.isEmpty())
            {
                QFile fgpg(expname);
                if (fgpg.exists())
                    fgpg.remove();

                KProcess p;
                p << KGpgSettings::gpgBinaryPath() << "--no-tty";

                p << "--output" << expname << "--export" << "--armor";
                if (exportAttr != NULL)
                    p << "--export-options" << *exportAttr;

                p << klist;

                p.execute();

                if (fgpg.exists())
                    KMessageBox::information(this, i18np("<qt>The public key was successfully exported to<br/>%2</qt>",
                                                         "<qt>The %1 public keys were successfully exported to<br/>%2</qt>",
                                                         klist.count(), expname));
                else
                    KMessageBox::sorry(this, i18n("Your public key could not be exported\nCheck the key."));
            }
        }
        else
        {
            KgpgInterface *kexp = new KgpgInterface();
            QString result = kexp->getKeys(true, exportAttr, klist);
            delete kexp;

            if (page->checkClipboard->isChecked())
                slotProcessExportClip(result);
            else
                slotProcessExportMail(result);
        }
        delete exportAttr;
    }

    delete dial;
}

void KeysManager::slotProcessExportMail(const QString &keys)
{
    // start default Mail application
    KToolInvocation::invokeMailer(QString(), QString(), QString(), QString(), keys);
}

void KeysManager::slotProcessExportClip(const QString &keys)
{
    kapp->clipboard()->setText(keys, m_clipboardmode);
}

void KeysManager::showKeyInfo(const QString &keyID)
{
    KgpgKeyInfo *opts = new KgpgKeyInfo(keyID, this);
    opts->show();
}

void KeysManager::slotShowPhoto()
{
    KService::List list = KServiceTypeTrader::self()->query("image/jpeg", "Type == 'Application'");
    KGpgNode *nd = iview->selectedNode();
    Q_ASSERT(nd->getType() == ITYPE_UAT);
    KGpgUatNode *und = static_cast<KGpgUatNode *>(nd);
    KGpgKeyNode *parent = und->getParentKeyNode();
    KService::Ptr ptr = list.first();
    //KMessageBox::sorry(0,ptr->desktopEntryName());
    KProcess p;
    p << KGpgSettings::gpgBinaryPath() << "--no-tty" << "--photo-viewer" << (ptr->desktopEntryName() + " %i") << "--edit-key" << parent->getId() << "uid" << und->getId() << "showphoto" << "quit";
    p.startDetached();
}

void KeysManager::defaultAction(const QModelIndex &index)
{
	KGpgNode *nd = iproxy->nodeForIndex(index);

	switch (nd->getType()) {
	case ITYPE_GROUP:
		editGroup();
		break;
	case ITYPE_UAT:
		slotShowPhoto();
		break;
	case ITYPE_SIGN:
	case ITYPE_GPUBLIC:
	case ITYPE_GSECRET:
	case ITYPE_GPAIR:
		iview->selectNode(imodel->getRootNode()->findKey(nd->getId()));
		break;
	case ITYPE_SECRET:
#warning FIXME: orphan
		break;
	case ITYPE_PAIR:
	case ITYPE_PUBLIC:
		showProperties(index);
		return;
        }
}

void
KeysManager::showProperties(const QModelIndex &index)
{
	KGpgNode *n = imodel->nodeForIndex(iproxy->mapToSource(index));

	switch (n->getType()) {
	case ITYPE_UAT:
		return;
	case ITYPE_PUBLIC:
	case ITYPE_PAIR:
		{
			KGpgKeyNode *k = static_cast<KGpgKeyNode *>(n);
			KgpgKeyInfo *opts = new KgpgKeyInfo(k->getKeyId(), this);
			connect(opts, SIGNAL(keyNeedsRefresh(const QString &)), keysList2, SLOT(refreshcurrentkey(const QString &)));
			opts->exec();
			delete opts;
		}
	default:
		return;
	}
}

void KeysManager::keyproperties()
{
    KeyListViewItem *cur = keysList2->currentItem();
    if (cur == NULL)
        return;

    if (cur->itemType() == KeyListViewItem::Secret)
    {
        if (KMessageBox::questionYesNo(this, i18n("This key is an orphaned secret key (secret key without public key.) It is currently not usable.\n\n"
                                               "Would you like to regenerate the public key?"), QString(), KGuiItem(i18n("Generate")), KGuiItem(i18n("Do Not Generate"))) == KMessageBox::Yes)
            slotregenerate();
            return;
    }

    QString key = cur->keyId();
    if (!key.isEmpty())
    {
        KgpgKeyInfo *opts = new KgpgKeyInfo(key, this);
        connect(opts, SIGNAL(keyNeedsRefresh(const QString &)), keysList2, SLOT(refreshcurrentkey(const QString &)));
        opts->exec();
        delete opts;
    }
}

void KeysManager::deleteGroup()
{
    KGpgNode *nd = iview->selectedNode();
    if (!nd || (nd->getType() != ITYPE_GROUP))
        return;

    int result = KMessageBox::warningContinueCancel(this, i18n("<qt>Are you sure you want to delete group <b>%1</b> ?</qt>", nd->getName()), QString(), KGuiItem(i18n("Delete"), "edit-delete"));
    if (result != KMessageBox::Continue)
        return;

    KgpgInterface::delGpgGroup(nd->getName(), KGpgSettings::gpgConfigPath());
    imodel->delGroup(nd);

    QStringList groups = KgpgInterface::getGpgGroupNames(KGpgSettings::gpgConfigPath());
    KGpgSettings::setGroups(groups.join(","));
    changeMessage(imodel->statusCountMessage(), 1);
}

void KeysManager::createNewGroup()
{
    QStringList badkeys;
    QStringList keysGroup;
    KGpgKeyNodeList keysList;
    KgpgItemType tp;
    QList<KGpgNode *> ndlist = iview->selectedNodes(NULL, &tp);

    if (ndlist.isEmpty())
       return;
    if (tp & ~ITYPE_PAIR) {
       KMessageBox::sorry(this, i18n("<qt>You cannot create a group containing signatures, subkeys or other groups.</qt>"));
       return;
    }

        for (int i = 0; i < ndlist.count(); ++i)
            {
                KGpgNode *nd = ndlist.at(i);
                    if ((nd->getTrust() == TRUST_FULL) ||
                        (nd->getTrust() == TRUST_ULTIMATE)) {
                        keysGroup += nd->getId();
                        keysList.append(static_cast<KGpgKeyNode *>(nd));
                    } else
                        badkeys += i18nc("<Name> (<Email>) ID: <KeyId>", "%1 (%2) ID: %3", nd->getName(), nd->getEmail(), nd->getId());
            }

        QString groupName = KInputDialog::getText(i18n("Create New Group"), i18n("Enter new group name:"), QString(), 0, this);
        if (groupName.isEmpty())
            return;
        if (!keysGroup.isEmpty())
        {
            if (!badkeys.isEmpty())
                KMessageBox::informationList(this, i18n("Following keys are not valid or not trusted and will not be added to the group:"), badkeys);

            KgpgInterface::setGpgGroupSetting(groupName, keysGroup, KGpgSettings::gpgConfigPath());
            QStringList groups = KgpgInterface::getGpgGroupNames(KGpgSettings::gpgConfigPath());

            iview->selectNode(imodel->addGroup(groupName, keysList));
            changeMessage(imodel->statusCountMessage(), 1);
        }
        else
            KMessageBox::sorry(this, i18n("<qt>No valid or trusted key was selected. The group <b>%1</b> will not be created.</qt>", groupName));
}

void KeysManager::editGroup()
{
    KGpgNode *nd = iview->selectedNode();
    if (!nd || (nd->getType() != ITYPE_GROUP))
        return;
    KDialog *dialogGroupEdit = new KDialog(this );
    dialogGroupEdit->setCaption( i18n("Group Properties") );
    dialogGroupEdit->setButtons( KDialog::Ok | KDialog::Cancel );
    dialogGroupEdit->setDefaultButton(  KDialog::Ok );
    dialogGroupEdit->setModal( true );

    QList<KGpgNode *> members;
    for (int i = 0; i < nd->getChildCount(); i++)
        members << nd->getChild(i);

    groupEdit *gEdit = new groupEdit(this, &members);
    gEdit->setModel(imodel);

    dialogGroupEdit->setMainWidget(gEdit);

    gEdit->show();
    if (dialogGroupEdit->exec() == QDialog::Accepted)
        imodel->changeGroup(static_cast<KGpgGroupNode *>(nd), members);
    delete dialogGroupEdit;
}

void KeysManager::signkey()
{
    if (keysList2->currentItem() == 0)
        return;
    if (keysList2->currentItem()->depth() != 0)
        return;

    signList = keysList2->selectedItems();
    bool keyDepth = true;
    for (int i = 0; i < signList.count(); ++i)
        if (signList.at(i))
            if (signList.at(i)->depth() != 0)
                keyDepth = false;

    if (!keyDepth)
    {
        KMessageBox::sorry(this, i18n("You can only sign primary keys. Please check your selection."));
        return;
    }

    if (signList.count() == 1)
    {
        KeyListViewItem *item = keysList2->currentItem();
        QString fingervalue;
        QString opt;

        KgpgInterface *interface = new KgpgInterface();
        KgpgKeyList listkeys = interface->readPublicKeys(true, item->keyId());
        delete interface;
        fingervalue = listkeys.at(0).fingerprintBeautified();

        opt = i18n("<qt>You are about to sign key:<br /><br />%1<br />ID: %2<br />Fingerprint: <br /><b>%3</b>.<br /><br />"
                   "You should check the key fingerprint by phoning or meeting the key owner to be sure that someone "
                   "is not trying to intercept your communications</qt>", item->text(0) + " (" + item->text(1) + ')', item->text(6), fingervalue);

        if (KMessageBox::warningContinueCancel(this, opt) != KMessageBox::Continue)
            return;
    }
    else
    {
        QStringList signKeyList;
		for (int i = 0; i < signList.count(); ++i) {
			KeyListViewItem *item = static_cast<KeyListViewItem *>(signList.at(i));

			if (item)
				signKeyList += item->text(0) + " (" + item->text(1) + "): " + item->keyId();
		}

        if (KMessageBox::warningContinueCancelList(this, i18n("<qt>You are about to sign the following keys in one pass.<br/><b>If you have not carefully checked all fingerprints, the security of your communications may be compromised.</b></qt>"), signKeyList) != KMessageBox::Continue)
            return;
    }

    KgpgSelectSecretKey *opts = new KgpgSelectSecretKey(this, true, signList.count());
    if (opts->exec() != QDialog::Accepted)
    {
        delete opts;
        return;
    }

    globalkeyID = QString(opts->getKeyID());
    globalkeyMail = QString(opts->getKeyMail());
    globalisLocal = opts->isLocalSign();
    globalChecked = opts->getSignTrust();
    globalCount = signList.count();
    m_isterminal = opts->isTerminalSign();
    keyCount = 0;
    delete opts;

    signLoop();
}

void KeysManager::signLoop()
{
    if (keyCount < globalCount)
    {
        kDebug(2100) << "Sign process for key: " << keyCount + 1 << " on a total of " << signList.count() ;
        if (signList.at(keyCount))
        {
            KgpgInterface *interface = new KgpgInterface();
            interface->signKey(signList.at(keyCount)->keyId(), globalkeyID, globalisLocal, globalChecked, m_isterminal);
            connect(interface, SIGNAL(signKeyFinished(int, KgpgInterface*)), this, SLOT(signatureResult(int, KgpgInterface*)));
        }
    }
}

void KeysManager::signatureResult(int success, KgpgInterface *interface)
{
    delete interface;
    if (success == 2)
        keysList2->refreshcurrentkey(signList.at(keyCount));
    else
    if (success == 1)
        KMessageBox::sorry(this, i18n("<qt>Bad passphrase, key <b>%1 (%2)</b> not signed.</qt>", signList.at(keyCount)->text(0), signList.at(keyCount)->text(1)));
    else
    if (success == 4)
        KMessageBox::sorry(this, i18n("<qt>The key <b>%1 (%2)</b> is already signed.</qt>", signList.at(keyCount)->text(0), signList.at(keyCount)->text(1)));

    keyCount++;
    signLoop();
}

void KeysManager::getMissingSigs(QStringList *missingKeys, KeyListViewItem *item)
{
	while (item) {
		if (isSignatureUnknown(item))
			*missingKeys << item->keyId();
		if (item->firstChild() != NULL)
			getMissingSigs(missingKeys, item->firstChild());
		item = item->nextSibling();
	}
}

void KeysManager::getMissingSigs(QStringList *missingKeys, KGpgExpandableNode *nd)
{
	for (int i = nd->getChildCount() - 1; i >= 0; i--) {
		KGpgNode *ch = nd->getChild(i);
		if (ch->hasChildren()) {
			getMissingSigs(missingKeys, static_cast<KGpgExpandableNode *>(ch));
			continue;
		}

		if (ch->getType() == ITYPE_SIGN) {
			if (static_cast<KGpgSignNode *>(ch)->isUnknown())
				*missingKeys << ch->getId();
		}
	}
}

void KeysManager::importallsignkey()
{
	QList<KeyListViewItem *> sel = keysList2->selectedItems();
	QStringList missingKeys;
	int i;

	if (sel.isEmpty())
		return;

	for (i = 0; i < sel.count(); i++) {
		KeyListViewItem *cur = sel.at(i);
		KeyListViewItem *item = cur->firstChild();

		if (item == NULL) {
			cur->setOpen(true);
			cur->setOpen(false);
			item = cur->firstChild();
		}

		getMissingSigs(&missingKeys, item);
	}

	if (missingKeys.isEmpty()) {
		KMessageBox::information(this,
			i18np("All signatures for this key are already in your keyring",
			"All signatures for this keys are already in your keyring", sel.count()));
		return;
	}

	// remove duplicate entries. TODO: Is there a standard function for this?
	missingKeys.sort();
	i = 0;
	while (i < missingKeys.count() - 1) {
		if (missingKeys[i] == missingKeys[i + 1])
			missingKeys.removeAt(i + 1);
		else
			i++;
	}

	importsignkey(missingKeys);
}

void KeysManager::preimportsignkey()
{
    QList<KeyListViewItem*> exportList = keysList2->selectedItems();
    QStringList idlist;

    if (exportList.empty())
      return;

    for (int i = 0; i < exportList.count(); ++i)
      idlist << exportList.at(i)->keyId();

    importsignkey(idlist);
}

bool KeysManager::importRemoteKey(const QString &keyIDs)
{
	KGpgTransaction *xact = new KGpgTransaction();

	QStringList kservers = KeyServer::getServerList();
	if (kservers.isEmpty())
		return false;
	connect(xact, SIGNAL(receiveComplete(KGpgTransaction *)), this, SLOT(importRemoteFinished(KGpgTransaction *)));
	xact->iface->downloadKeys(keyIDs.split(' '), kservers.first(), false, getenv("http_proxy"));

	return true;
}

void KeysManager::importRemoteFinished(KGpgTransaction *t)
{
	delete t;
	keysList2->refreshAll();
}

void KeysManager::importsignkey(const QStringList &importKeyId)
{
    // sign a key
    kServer = new KeyServer(0, false);
    kServer->slotSetText(importKeyId.join(" "));
    //kServer->Buttonimport->setDefault(true);
    kServer->slotImport();
    //kServer->show();
    connect(kServer, SIGNAL(importFinished(QStringList)), this, SLOT(importfinished()));
}

void KeysManager::importfinished()
{
    if (kServer)
        kServer = 0L;
    refreshkey();
}

void KeysManager::delsignkey()
{
    // sign a key
    if (keysList2->currentItem() == 0)
        return;

    QString uid;
    KeyListViewItem *parent = keysList2->currentItem()->parent();
    if (parent->itemType() == KeyListViewItem::Public)
        uid = '1';
    else {
        Q_ASSERT(parent->itemType() == KeyListViewItem::Uid);
        uid = parent->text(6);
    }

    QString signID;
    QString parentKey;
    QString signMail;
    QString parentMail;
    KeyListViewItem *sitem = keysList2->currentItem();

    if (parent->depth() == 1)
        parentKey = parent->parent()->keyId();
    else
        parentKey = parent->keyId();
    signID = sitem->keyId();
    parentMail = keysList2->currentItem()->parent()->text(0) + " (" + keysList2->currentItem()->parent()->text(1) + ')';
    signMail = keysList2->currentItem()->text(0) + " (" + keysList2->currentItem()->text(1) + ')';

    parentMail = parent->text(0);
    if (!parent->text(1).isEmpty())
       parentMail += " &lt;" + parent->text(1) + "&gt;";
    signMail = keysList2->currentItem()->text(0);
    if (!keysList2->currentItem()->text(1).isEmpty())
       signMail += " &lt;" + keysList2->currentItem()->text(1) + "&gt;";

    if (parentKey == signID)
    {
        KMessageBox::sorry(this, i18n("Edit key manually to delete a self-signature."));
        return;
    }

    QString ask = i18n("<qt>Are you sure you want to delete signature<br /><b>%1</b><br />from user id <b>%2</b><br />of key: <b>%3</b>?</qt>", signMail, parentMail, parentKey);

    if (KMessageBox::questionYesNo(this, ask, QString(), KStandardGuiItem::del(), KStandardGuiItem::cancel()) != KMessageBox::Yes)
        return;

    KgpgInterface *delSignKeyProcess = new KgpgInterface();
    connect(delSignKeyProcess, SIGNAL(delsigfinished(bool)), this, SLOT(delsignatureResult(bool)));
    delSignKeyProcess->KgpgDelSignature(parentKey, uid, signID);
}

void KeysManager::delsignatureResult(bool success)
{
    if (success)
    {
        KeyListViewItem *top = keysList2->currentItem();
        while (top->depth() != 0)
            top = top->parent();
        while (top->firstChild() != 0)
            delete top->firstChild();
        keysList2->refreshcurrentkey(top);
    }
    else
        KMessageBox::sorry(this, i18n("Requested operation was unsuccessful, please edit the key manually."));
}

void KeysManager::slotedit()
{
    KeyListViewItem *item = keysList2->currentItem();

    if (item == NULL)
        return;
    if (!(item->itemType() & KeyListViewItem::Pair))
        return;
    if (!terminalkey.isEmpty())
        return;

    KProcess *kp = new KProcess(this);
    KConfigGroup config(KGlobal::config(), "General");
    *kp << config.readPathEntry("TerminalApplication","konsole");
    *kp << "-e" << KGpgSettings::gpgBinaryPath() <<"--no-secmem-warning" <<"--edit-key" << keysList2->currentItem()->keyId() << "help";
    terminalkey = keysList2->currentItem()->keyId();

    connect(kp, SIGNAL(finished(int)), SLOT(slotEditDone(int)));
    kp->start();
}

void KeysManager::slotEditDone(int exitcode)
{
    if (exitcode == 0)
        keysList2->refreshcurrentkey(terminalkey);

    terminalkey.clear();
}

void KeysManager::doFilePrint(const QString &url)
{
    QFile qfile(url);
    if (qfile.open(QIODevice::ReadOnly))
    {
        QTextStream t(&qfile);
        doPrint(t.readAll());
    }
    else
        KMessageBox::sorry(this, i18n("<qt>Cannot open file <b>%1</b> for printing...</qt>", url));
}

void KeysManager::doPrint(const QString &txt)
{
    QPrinter prt;
    //kDebug(2100) << "Printing..." ;
    QPrintDialog printDialog(&prt, this);
    if (printDialog.exec())
    {
        QPainter painter(&prt);
        int width = painter.device()->width();
        int height = painter.device()->height();
        painter.drawText(0, 0, width, height, Qt::AlignLeft|Qt::AlignTop|Qt::TextDontClip, txt);
    }
}

void KeysManager::deleteseckey()
{
    // delete a key
    QString res = keysList2->currentItem()->text(0) + " (" + keysList2->currentItem()->text(1) + ')';
    int result = KMessageBox::warningContinueCancel(this,
                        i18n("<p>Delete <b>secret</b> key pair <b>%1</b>?</p>Deleting this key pair means you will never be able to decrypt files encrypted with this key again.", res),
                        QString(),
                        KGuiItem(i18n("Delete"),"edit-delete"));
    if (result != KMessageBox::Continue)
        return;

    KProcess *conprocess = new KProcess();
    KConfigGroup config(KGlobal::config(), "General");
    *conprocess << config.readPathEntry("TerminalApplication","konsole");
    *conprocess << "-e" << KGpgSettings::gpgBinaryPath() <<"--no-secmem-warning" << "--delete-secret-and-public-key" << keysList2->currentItem()->keyId();
    connect(conprocess, SIGNAL(finished(int)), this, SLOT(reloadSecretKeys()));
    conprocess->start();
}

void KeysManager::reloadSecretKeys()
{
	refreshkey();
}

void KeysManager::confirmdeletekey()
{
    KeyListViewItem *ki = keysList2->currentItem();

    // do not delete a key currently edited in terminal
    if ((ki->keyId() == terminalkey) && (keysList2->selectedItems().count() == 1)) {
        KMessageBox::error(this, i18n("Can not delete key <b>%1</b> while it is edited in terminal.", terminalkey), i18n("Delete key"));
        return;
    }

    if (ki->itemType() & KeyListViewItem::Group)
    {
        deleteGroup();
        return;
    }

    if ((ki->itemType() & KeyListViewItem::Secret) && (keysList2->selectedItems().count() == 1))
        deleteseckey();
    else
    {
        QStringList keysToDelete;
        QString secList;
        QList<KeyListViewItem*> exportList = keysList2->selectedItems();
        bool secretKeyInside = false;
        for (int i = 0; i < exportList.count(); ++i) {
	    KeyListViewItem *ki = exportList.at(i);

            if (ki)
            {
                if (ki->itemType() & KeyListViewItem::Secret)
                {
                    secretKeyInside = true;
                    secList += ki->text(0) + " (" + ki->text(1) + ")<br/>";
                    ki->setSelected(false);
                }
                else if (ki->keyId() != terminalkey)
                    keysToDelete += ki->text(0) + " (" + ki->text(1) + ')';
            }
	}

        if (secretKeyInside)
        {
            int result = KMessageBox::warningContinueCancel(this, i18n("<qt>The following are secret key pairs:<br/><b>%1</b>They will not be deleted.</qt>", secList));
            if (result != KMessageBox::Continue)
                return;
        }

        if (keysToDelete.isEmpty())
            return;

        int result = KMessageBox::warningContinueCancelList(this, i18np("<qt><b>Delete the following public key?</b></qt>", "<qt><b>Delete the following %1 public keys?</b></qt>", keysToDelete.count()), keysToDelete, QString(), KStandardGuiItem::del());
        if (result != KMessageBox::Continue)
            return;
        else
            deletekey();
    }
}

void KeysManager::deletekey()
{
    QList<KeyListViewItem*> exportList = keysList2->selectedItems();
    if (exportList.count() == 0)
        return;

    KProcess gp;
    gp << KGpgSettings::gpgBinaryPath()
    << "--no-tty"
    << "--no-secmem-warning"
    << "--batch"
    << "--yes"
    << "--delete-key";

    for (int i = 0; i < exportList.count(); ++i) {
	KeyListViewItem *item = exportList.at(i);
	if (!item)
		continue;
	gp << item->keyId();
    }

    gp.execute();

    keysList2->refreshAll();

    if (keysList2->currentItem())
    {
        KeyListViewItem * myChild = keysList2->currentItem();
        while(!myChild->isVisible())
        {
            myChild = myChild->nextSibling();
            if (!myChild)
                break;
        }

        if (!myChild)
        {
            KeyListViewItem * myChild = keysList2->firstChild();
            while(!myChild->isVisible())
            {
                myChild = myChild->nextSibling();
                if (!myChild)
                    break;
            }
        }

        if (myChild)
        {
            myChild->setSelected(true);
            keysList2->setCurrentItem(myChild);
        }
    }
    else
        stateChanged("empty_list");

    changeMessage(keysList2->statusCountMessage(), 1);
}

void KeysManager::slotPreImportKey()
{
    KDialog *dial = new KDialog(this );
    dial->setCaption(  i18n("Key Import") );
    dial->setButtons( KDialog::Ok | KDialog::Cancel );
    dial->setDefaultButton( KDialog::Ok );
    dial->setModal( true );

    SrcSelect *page = new SrcSelect();
    dial->setMainWidget(page);
    page->newFilename->setWindowTitle(i18n("Open File"));
    page->newFilename->setMode(KFile::File);
//     page->resize(page->minimumSize());
//     dial->resize(dial->minimumSize());

    if (dial->exec() == QDialog::Accepted)
    {
        if (page->checkFile->isChecked())
        {
            QString impname = page->newFilename->url().url().simplified();
            if (!impname.isEmpty())
            {
                changeMessage(i18n("Importing..."), 0, true);
                // import from file
                KgpgInterface *importKeyProcess = new KgpgInterface();
                connect(importKeyProcess, SIGNAL(importKeyFinished(QStringList)), keysList2, SLOT(slotReloadKeys(QStringList)));
                connect(importKeyProcess, SIGNAL(importKeyOrphaned()), keysList2, SLOT(slotReloadOrphaned()));
                importKeyProcess->importKey(KUrl(impname));
            }
        }
        else
        {
            QString keystr = kapp->clipboard()->text(m_clipboardmode);
            if (!keystr.isEmpty())
            {
                changeMessage(i18n("Importing..."), 0, true);
                KgpgInterface *importKeyProcess = new KgpgInterface();
                connect(importKeyProcess,SIGNAL(importKeyFinished(QStringList)),keysList2,SLOT(slotReloadKeys(QStringList)));
                connect(importKeyProcess,SIGNAL(importKeyOrphaned()),keysList2,SLOT(slotReloadOrphaned()));
                importKeyProcess->importKey(keystr);
            }
        }
    }
    delete dial;
}

void KeysManager::refreshkey()
{
    keysList2->refreshAll();
#warning FIXME: refresh model items
}

KGpgTransaction::KGpgTransaction()
	: iface(new KgpgInterface())
{
	connect(iface, SIGNAL(downloadKeysFinished(QList<int>, QStringList, bool, QString, KgpgInterface*)), this, SLOT(slotDownloadKeysFinished(QList<int>, QStringList, bool, QString, KgpgInterface*)));
}

void KGpgTransaction::slotDownloadKeysFinished(QList<int>, QStringList, bool, QString, KgpgInterface*)
{
	emit receiveComplete(this);
}

#include "keysmanager.moc"
