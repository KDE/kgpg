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

#include <Q3TextDrag>

#include <kabc/addresseedialog.h>
#include <kabc/stdaddressbook.h>
#include <kpassworddialog.h>
#include <ktoolinvocation.h>
#include <kurlrequester.h>
#include <kio/netaccess.h>
#include <kstandarddirs.h>
#include <kpassivepopup.h>
#include <kdesktopfile.h>
#include <kinputdialog.h>
#include <kfiledialog.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kfinddialog.h>
#include <kstatusbar.h>
#include <kservice.h>
#include <kservicetypetrader.h>
#include <klineedit.h>
#include <kmimetype.h>
#include <kshortcut.h>
#include <kstandardshortcut.h>
#include <k3process.h>
#include <kprinter.h>
#include <klocale.h>
#include <k3procio.h>
#include <kaction.h>
#include <kdebug.h>
#include <kfind.h>
#include <kmenu.h>
#include <kurl.h>
#include <ktip.h>
#include <krun.h>
#include <ktoolbar.h>
#include <kactioncollection.h>
#include <kstandardaction.h>
#include <kselectaction.h>
#include <kicon.h>
#include <kvbox.h>

#include "selectsecretkey.h"
#include "newkey.h"
#include "kgpg.h"
#include "kgpgeditor.h"
#include "kgpgview.h"
#include "keysmanager.h"
#include "keyexport.h"
#include "sourceselect.h"
#include "kgpgrevokewidget.h"
#include "keyservers.h"
#include "keyserver.h"
#include "kgpginterface.h"
#include "kgpgsettings.h"
#include "kgpgkeygenerate.h"
#include "kgpgoptions.h"
#include "keyinfodialog.h"
#include "kgpglibrary.h"
#include "keylistview.h"
#include "keyadaptor.h"
#include <QtDBus>
#include <ktoggleaction.h>
#include "images.h"

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
    action->setIcon( KIcon("network-wired") );
    connect(action, SIGNAL(triggered(bool)), SLOT(showKeyServer()));

    action =  actionCollection()->addAction( "help_tipofday");
    action->setIcon( KIcon("idea") );
    action->setText( i18n("Tip of the &Day") );
    connect(action, SIGNAL(triggered(bool)), SLOT(slotTip()));

    action = actionCollection()->addAction( "gpg_man");
    action->setText( i18n("View GnuPG Manual") );
    action->setIcon( KIcon("help-contents") );
    connect(action, SIGNAL(triggered(bool)), SLOT(slotManpage()));

    action = actionCollection()->addAction("kgpg_editor");
    action->setIcon(KIcon("edit"));
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

    action = actionCollection()->add<KToggleAction>("show_secret");
    action->setIcon(KIcon("kgpg_show"));
    action->setText(i18n("&Show only Secret Keys"));
    connect(action, SIGNAL(triggered(bool)), SLOT(slotToggleSecret()));

    action = actionCollection()->add<KToggleAction>("hide_disabled");
    action->setText(i18n("&Hide Expired/Disabled Keys"));
    connect(action, SIGNAL(triggered(bool)), SLOT(slotToggleDisabled()));

    QAction *infoKey = actionCollection()->addAction("key_info");
    infoKey->setIcon(KIcon("kgpg-info-kgpg"));
    infoKey->setText(i18n("&Edit Key"));
    connect(infoKey, SIGNAL(triggered(bool)), SLOT(listsigns()));
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
    newContact->setIcon(KIcon("kaddressbook"));
    newContact->setText(i18n("&Create New Contact in Address Book"));
    connect(newContact, SIGNAL(triggered(bool)), SLOT(addToKAB()));

    QAction *createGroup = actionCollection()->addAction("create_group");
    createGroup->setText(i18n("&Create Group with Selected Keys..."));
    connect(createGroup, SIGNAL(triggered(bool)), SLOT(createNewGroup()));

    QAction *editCurrentGroup = actionCollection()->addAction("edit_group");
    editCurrentGroup->setText(i18n("&Edit Group"));
    connect(editCurrentGroup, SIGNAL(triggered(bool)), SLOT(editGroup()));

    QAction *delGroup = actionCollection()->addAction("delete_group");
    delGroup->setText(i18n("&Delete Group"));
    connect(delGroup, SIGNAL(triggered(bool)), SLOT(deleteGroup()));

    QAction *deleteKey = actionCollection()->addAction("key_delete");
    deleteKey->setIcon(KIcon("edit-delete"));
    deleteKey->setText(i18n("&Delete Keys"));
    connect(deleteKey, SIGNAL(triggered(bool)), SLOT(confirmdeletekey()));
    deleteKey->setShortcut(QKeySequence(Qt::Key_Delete));

    QAction *setDefaultKey = actionCollection()->addAction("key_default");
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
    QAction *openPhoto = actionCollection()->addAction("key_photo");
    openPhoto->setIcon(KIcon("image"));
    openPhoto->setText(i18n("&Open Photo"));
    connect(openPhoto, SIGNAL(triggered(bool)), SLOT(slotShowPhoto()));
    QAction *deletePhoto = actionCollection()->addAction("delete_photo");
    deletePhoto->setIcon(KIcon("delete"));
    deletePhoto->setText(i18n("&Delete Photo"));
    connect(deletePhoto, SIGNAL(triggered(bool)), SLOT(slotDeletePhoto()));
    QAction *delSignKey = actionCollection()->addAction("key_delsign");
    delSignKey->setIcon(KIcon("edit-delete"));
    delSignKey->setText(i18n("Delete Sign&ature"));
    connect(delSignKey, SIGNAL(triggered(bool)), SLOT(delsignkey()));

    importAllSignKeys = actionCollection()->addAction("key_importallsign");
    importAllSignKeys->setIcon(KIcon("network-wired"));
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
    importSignatureKey->setIcon(KIcon("network-wired"));
    importSignatureKey->setText(i18n("Import Key From Keyserver"));
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
    list.append(i18n("Small"));
    list.append(i18n("Medium"));
    list.append(i18n("Large"));
    photoProps->setItems(list);

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

    m_popupout = new KMenu();
    m_popupout->addAction(importKey);
    m_popupout->addAction(generateKey);

    m_popupsig = new KMenu();
    m_popupsig->addAction(importSignatureKey);
    m_popupsig->addAction(delSignKey);

    m_popupphoto = new KMenu();
    m_popupphoto->addAction(openPhoto);
    m_popupphoto->addAction(deletePhoto);

    m_popupuid = new KMenu();
    m_popupuid->addAction(delUid);

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

    setCentralWidget(keysList2);
    keysList2->restoreLayout(KGlobal::config().data(), "KeyView");

    connect(keysList2, SIGNAL(returnPressed(KeyListViewItem *)), this, SLOT(listsigns()));
    connect(keysList2, SIGNAL(doubleClicked(KeyListViewItem *, const QPoint &, int)), this, SLOT(listsigns()));
    connect(keysList2, SIGNAL(selectionChanged ()), this, SLOT(checkList()));
    connect(keysList2, SIGNAL(contextMenuRequested(KeyListViewItem *, const QPoint &, int)), this, SLOT(slotMenu(KeyListViewItem *, const QPoint &, int)));
    connect(keysList2, SIGNAL(destroyed()), this, SLOT(annule()));
    connect(photoProps, SIGNAL(activated(int)), this, SLOT(slotSetPhotoSize(int)));

    // get all keys data
    setupGUI(KXmlGuiWindow::Create | Save | ToolBar | StatusBar | Keys, "keysmanager.rc");
    toolBar()->addSeparator();

    (void) new QLabel(i18n("Search: "), toolBar());
    m_listviewsearch = new KeyListViewSearchLine(toolBar(), keysList2);
    m_listviewsearch->setClearButtonShown(true);

    action = actionCollection()->addAction("search_focus");
    action->setText(i18n("Filter Search"));
    connect(action, SIGNAL(triggered(bool) ), m_listviewsearch, SLOT(setFocus()));
    action->setShortcut(QKeySequence(Qt::Key_F6));

    sTrust->setChecked(KGpgSettings::showTrust());
    sSize->setChecked(KGpgSettings::showSize());
    sCreat->setChecked(KGpgSettings::showCreat());
    sExpi->setChecked(KGpgSettings::showExpi());

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

            delete kg;
            return;
        }
        else
        {
            K3Process kp;
            KConfigGroup config(KGlobal::config(), "General");
            kp << config.readPathEntry("TerminalApplication", "konsole");
            kp << "-e" << "gpg" << "--gen-key";
            kp.start(K3Process::Block);
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

    KVBox *passiveBox = pop->standardView(i18n("Generating new key pair."), QString(), KIconLoader::global()->loadIcon("kgpg", K3Icon::Desktop), 0);

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
    changeMessage(i18n("Ready"), 0);

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
    {
        changeMessage(i18n("%1 Keys, %2 Groups", keysList2->childCount() - keysList2->groupNb, keysList2->groupNb), 1);

        KDialog *keyCreated = new KDialog(this);
        keyCreated->setCaption(i18n("New Key Pair Created"));
        keyCreated->setButtons(KDialog::Ok);
        keyCreated->setDefaultButton(KDialog::Ok);
        keyCreated->setModal(true);

        newKey *page = new newKey(keyCreated);
        page->TLname->setText("<b>" + name + "</b>");
        page->TLemail->setText("<b>" + email + "</b>");

        if (!email.isEmpty())
            page->kURLRequester1->setUrl(QDir::homePath() + '/' + email.section("@", 0, 0) + ".revoke");
        else
            page->kURLRequester1->setUrl(QDir::homePath() + '/' + email.section(" ", 0, 0) + ".revoke");

        page->TLid->setText("<b>" + id + "</b>");
        page->LEfinger->setText(fingerprint);
        page->CBdefault->setChecked(true);
        page->show();
        keyCreated->setMainWidget(page);

        delete pop;
        pop = 0;

        keyCreated->exec();

        KeyListViewItem *newdef = keysList2->findItem(id, 6);
        if (newdef)
        {
            if (page->CBdefault->isChecked())
                slotSetDefaultKey(newdef);
            else
            {
                keysList2->clearSelection();
                keysList2->setCurrentItem(newdef);
                keysList2->setSelected(newdef, true);
                keysList2->ensureItemVisible(newdef);
            }
        }

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
    if (sTrust->isChecked())
        keysList2->slotAddColumn(2);
    else
        keysList2->slotRemoveColumn(2);
}

void KeysManager::slotShowExpiration()
{
    if (sExpi->isChecked())
        keysList2->slotAddColumn(3);
    else
        keysList2->slotRemoveColumn(3);
}

void KeysManager::slotShowSize()
{
    if (sSize->isChecked())
        keysList2->slotAddColumn(4);
    else
        keysList2->slotRemoveColumn(4);
}

void KeysManager::slotShowCreation()
{
    if (sCreat->isChecked())
        keysList2->slotAddColumn(5);
    else
        keysList2->slotRemoveColumn(5);
}

void KeysManager::slotToggleSecret()
{
    KeyListViewItem *item = keysList2->firstChild();
    if (!item)
        return;

    m_listviewsearch->setHidePublic(!m_listviewsearch->hidePublic());
    m_listviewsearch->updateSearch(m_listviewsearch->text());
}

void KeysManager::slotToggleDisabled()
{
    KeyListViewItem *item = keysList2->firstChild();
    if (!item)
        return;

    m_listviewsearch->setHideDisabled(!m_listviewsearch->hideDisabled());
    m_listviewsearch->updateSearch(m_listviewsearch->text());
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
    KeyListViewItem *myDefaulKey = keysList2->findItem(KGpgSettings::defaultKey(), 6);
    keysList2->clearSelection();
    keysList2->setCurrentItem(myDefaulKey);
    keysList2->setSelected(myDefaulKey, true);
    keysList2->ensureItemVisible(myDefaulKey);
}

void KeysManager::refreshKeyFromServer()
{
    if (keysList2->currentItem() == NULL)
        return;

    QString keyIDS;
    keysList = keysList2->selectedItems();

    for (int i = 0; i < keysList.count(); ++i)
        if (keysList.at(i))
        {
            if ((keysList.at(i)->depth() != 0) || (keysList.at(i)->text(6).isEmpty())) {
			KMessageBox::sorry(this, i18n("You can only refresh primary keys. Please check your selection."));
			return;
            } else
                keyIDS += keysList.at(i)->text(6) + ' ';
        }

    kServer = new KeyServer(0, false);
    connect(kServer, SIGNAL(importFinished(QString)), this, SLOT(refreshFinished()));
    kServer->slotSetText(keyIDS);
    kServer->slotImport();
}

void KeysManager::refreshFinished()
{
    if (kServer)
        kServer = 0L;

    for (int i = 0; i < keysList.count(); ++i)
        if (keysList.at(i))
            keysList2->refreshcurrentkey(keysList.at(i));
}

void KeysManager::slotDelUid()
{
    KeyListViewItem *item = keysList2->currentItem();
    while (item->depth()>0)
        item = item->parent();

    K3Process *process = new K3Process();
    KConfigGroup config(KGlobal::config(), "General");
    *process << config.readPathEntry("TerminalApplication", "konsole");
    *process << "-e" << "gpg";
    *process << "--edit-key" << item->text(6) << "uid";
    process->start(K3Process::Block);
    keysList2->refreshselfkey();
}

void KeysManager::slotregenerate()
{
    FILE *fp;
    QString tst;
    char line[300];
    QString cmd = "gpg --no-secmem-warning --export-secret-key " + keysList2->currentItem()->text(6) + " | gpgsplit --no-split --secret-to-public | gpg --import";

    fp = popen(cmd.toAscii(), "r");
    while (fgets(line, sizeof(line), fp))
    {
        tst += line;
    }
    pclose(fp);
    QString regID = keysList2->currentItem()->text(6);
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
    addUidProcess->addUid(keysList2->currentItem()->text(6),keyUid->kLineEdit1->text(),keyUid->kLineEdit2->text(),keyUid->kLineEdit3->text());
}

void KeysManager::slotAddUidFin(int res, KgpgInterface *interface)
{
    // TODO tester les res
    kDebug(2100) << "Resultat : " << res << endl;
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
    lib->addPhoto(keysList2->currentItem()->text(6));
}

void KeysManager::slotDeletePhoto()
{
    QString mess = i18n("<qt>Are you sure you want to delete Photo id <b>%1</b><br>from key <b>%2 &lt;%3&gt;</b>?</qt>",
                        keysList2->currentItem()->text(6),
                        keysList2->currentItem()->parent()->text(0),
                        keysList2->currentItem()->parent()->text(1));

    /*
    if (KMessageBox::warningContinueCancel(this, mess, i18n("Warning"), KGuiItem(i18n("Delete"), "edit-delete")) != KMessageBox::Continue)
        return;
    */

    KgpgInterface *interface = new KgpgInterface();
    connect(interface, SIGNAL(delPhotoFinished(int, KgpgInterface*)), this, SLOT(slotDelPhotoFinished(int, KgpgInterface*)));
    interface->deletePhoto(keysList2->currentItem()->parent()->text(6), keysList2->currentItem()->text(6));
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
            break;
        case 2:
            showPhoto = true;
            keysList2->setPreviewSize(42);
            break;
        case 3:
            showPhoto = true;
            keysList2->setPreviewSize(65);
            break;
        default:
            showPhoto = false;
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
                if (newdefChild->text(0) == i18n("Photo id"))
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
        KMessageBox::sorry(this, i18n("<qt>Search string '<b>%1</b>' not found.", searchString));
}

void KeysManager::findNextKey()
{
    //kDebug(2100)<<"find next"<<endl;
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
    //kDebug(2100) << "Next string:" << searchText << endl;
    //kDebug(2100) << "Search:" << searchString << endl;
    //kDebug(2100) << "OPts:" << searchOptions << endl;
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
            //kDebug(2100) << "Next string:" << searchText << endl;
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
    if (!keysList2->currentItem())
        return;

    //QString email = extractKeyMail(keysList2->currentItem()).simplified();
    QString email = keysList2->currentItem()->text(1);

    KABC::AddressBook *ab = KABC::StdAddressBook::self();
    if (!ab->load())
    {
        KMessageBox::sorry(this, i18n("Unable to contact the address book. Please check your installation."));
        return;
    }

    KABC::Addressee::List addresseeList = ab->findByEmail(email);
    KToolInvocation::startServiceByDesktopName("kaddressbook");
    QDBusInterface kaddressbook("org.kde.kaddressbook", "/KAddressBook", "org.kde.kaddressbook.KAddressBook");
    if(!addresseeList.isEmpty())
        kaddressbook.call( "showContactEditor", addresseeList.first().uid());
    else
        kaddressbook.call( "addEmail", QString (keysList2->currentItem()->text(0)) + " <" + email + '>');
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
    connect(ks, SIGNAL(importFinished(QString)), keysList2, SLOT(refreshcurrentkey(QString)));
    ks->exec();
    delete ks;
    refreshkey();
}

void KeysManager::checkList()
{
    QList<KeyListViewItem*> exportList = keysList2->selectedItems();
    if (exportList.count() > 1)
    {
        stateChanged("multi_selected");
        for (int i = 0; i < exportList.count(); ++i)
            if (exportList.at(i) && !(exportList.at(i)->isVisible()))
                exportList.at(i)->setSelected(false);
    }
    else
    {
        if (keysList2->currentItem()->text(6).isEmpty())
            stateChanged("group_selected");
        else
            stateChanged("single_selected");

    }

    int serial = keysList2->currentItem()->pixmap(0)->serialNumber();
    if (serial == Images::single().serialNumber())
    {
        if (keysList2->currentItem()->depth() == 0)
            changeMessage(i18n("Public Key"), 0);
        else
            changeMessage(i18n("Sub Key"), 0);
    }
    else
    if (serial == Images::pair().serialNumber())
        changeMessage(i18n("Secret Key Pair"), 0);
    else
    if (serial == Images::group().serialNumber())
        changeMessage(i18n("Key Group"), 0);
    else
    if (serial == Images::signature().serialNumber())
        changeMessage(i18n("Signature"), 0);
    else
    if (serial == Images::userId().serialNumber())
        changeMessage(i18n("User ID"), 0);
    else
    if (keysList2->currentItem()->text(0) == i18n("Photo id"))
        changeMessage(i18n("Photo ID"), 0);
    else
    if (serial == Images::revoke().serialNumber())
        changeMessage(i18n("Revocation Signature"), 0);
    else
    if (serial == Images::orphan().serialNumber())
        changeMessage(i18n("Orphaned Secret Key"), 0);
}

void KeysManager::annule()
{
    // close window
    close();
}

void KeysManager::quitApp()
{
    // close window
    qApp->quit();
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
    changeMessage(i18n("%1 Keys, %2 Groups", keysList2->childCount() - keysList2->groupNb, keysList2->groupNb), 1);

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
    connect(optionsDialog, SIGNAL(installShredder()), this, SIGNAL(installShredder()));
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
    slotSetDefaultKey(keysList2->currentItem());
}

void KeysManager::slotSetDefaultKey(const QString &newID)
{
    KeyListViewItem *newdef = keysList2->findItem(newID, 6);
    if (newdef)
        slotSetDefaultKey(newdef);
}

void KeysManager::slotSetDefaultKey(KeyListViewItem *newdef)
{
    //kDebug(2100)<<"------------------start ------------"<<endl;
    if ((!newdef) || (newdef->pixmap(2)==NULL))
        return;
    //kDebug(2100)<<newdef->text(6)<<endl;
    //kDebug(2100)<<KGpgSettings::defaultKey()<<endl;
    if (newdef->text(6)==KGpgSettings::defaultKey())
        return;
    if (newdef->pixmap(2)->serialNumber()!=keysList2->trustgood.serialNumber())
    {
        KMessageBox::sorry(this,i18n("Sorry, this key is not valid for encryption or not trusted."));
        return;
    }

    KeyListViewItem *olddef = keysList2->findItem(KGpgSettings::defaultKey(), 6);

    KGpgSettings::setDefaultKey(newdef->text(6));
    KGpgSettings::writeConfig();
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
	Q_ASSERT(isSignature(item));

	// ugly hack to detect unknown keys
	return (item->text(0).startsWith("[") && item->text(0).endsWith("]"));
}

void KeysManager::slotMenu(KeyListViewItem *sel, const QPoint &pos, int)
{
    // popup a different menu depending on which key is selected
    if (sel != 0)
    {
        if (keysList2->selectedItems().count() > 1)
        {
            QList<KeyListViewItem*> exportList = keysList2->selectedItems();
            bool keyDepth = true;
            for (int i = 0; i < exportList.count(); ++i)
                if (exportList.at(i))
                    if (exportList.at(i)->depth() != 0)
                        keyDepth = false;

            if (!keyDepth)
            {
                signKey->setEnabled(false);
                refreshKey->setEnabled(false);
                m_popupout->exec(pos);
                return;
            }
            else
            {
                signKey->setEnabled(true);
                refreshKey->setEnabled(true);
            }
        }

        if (sel->depth() != 0)
        {
            if (isSignature(sel))
            {
		// isn't this always true?
                if ((sel->text(2) == "-") || (sel->text(2) == i18n("Revoked")))
                {
                    if (isSignatureUnknown(sel))
                        importSignatureKey->setEnabled(true);
                    else
                        importSignatureKey->setEnabled(false);
                    m_popupsig->exec(pos);
                    return;
                }
            }
            else
            if (sel->text(0) == i18n("Photo id"))
                m_popupphoto->exec(pos);
            else
            if (sel->text(6) == ("-"))
                m_popupuid->exec(pos);
        }
        else
        {
            keysList2->setSelected(sel, true);
            if (keysList2->currentItem()->text(6).isEmpty())
                m_popupgroup->exec(pos);
            else
            {
                if (((sel->itemType() & KeyListViewItem::Pair) == KeyListViewItem::Pair) && (keysList2->selectedItems().count() == 1))
                    m_popupsec->exec(pos);
                else
                if ((sel->itemType() & KeyListViewItem::Secret) && (keysList2->selectedItems().count() == 1))
                    m_popuporphan->exec(pos);
                else
                    m_popuppub->exec(pos);
            }
            return;
        }
    }
    else
        m_popupout->exec(pos);
}

void KeysManager::slotrevoke(const QString &keyID, const QString &revokeUrl, const int reason, const QString &description)
{
    revKeyProcess = new KgpgInterface();
    revKeyProcess->KgpgRevokeKey(keyID, revokeUrl, reason, description);
}

void KeysManager::revokeWidget()
{
    KDialog *keyRevokeWidget = new KDialog(this );
    keyRevokeWidget->setCaption(  i18n("Create Revocation Certificate") );
    keyRevokeWidget->setButtons(  KDialog::Ok | KDialog::Cancel );
    keyRevokeWidget->setDefaultButton(  KDialog::Ok );
    keyRevokeWidget->setModal( true );
    KgpgRevokeWidget *keyRevoke = new KgpgRevokeWidget();

    keyRevoke->keyID->setText(keysList2->currentItem()->text(0) + " (" + keysList2->currentItem()->text(1) + ") " + i18n("ID: ") + keysList2->currentItem()->text(6));
    keyRevoke->kURLRequester1->setUrl(QDir::homePath() + '/' + keysList2->currentItem()->text(1).section('@', 0, 0) + ".revoke");
    keyRevoke->kURLRequester1->setMode(KFile::File);

    keyRevoke->setMinimumSize(keyRevoke->sizeHint());
    keyRevoke->show();
    keyRevokeWidget->setMainWidget(keyRevoke);

    if (keyRevokeWidget->exec() != QDialog::Accepted)
        return;
    if (keyRevoke->cbSave->isChecked())
    {
        slotrevoke(keysList2->currentItem()->text(6), keyRevoke->kURLRequester1->url().path(), keyRevoke->comboBox1->currentIndex(), keyRevoke->textDescription->text());
        if (keyRevoke->cbPrint->isChecked())
            connect(revKeyProcess, SIGNAL(revokeurl(QString)), this, SLOT(doFilePrint(QString)));
        if (keyRevoke->cbImport->isChecked())
            connect(revKeyProcess, SIGNAL(revokeurl(QString)), this, SLOT(slotImportRevoke(QString)));
    }
    else
    {
        slotrevoke(keysList2->currentItem()->text(6), QString(), keyRevoke->comboBox1->currentIndex(), keyRevoke->textDescription->text());
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
    QString warn = i18n("Secret keys SHOULD NOT be saved in an unsafe place.\n"
                        "If someone else can access this file, encryption with this key will be compromised!\nContinue key export?");
    int result = KMessageBox::questionYesNo(this, warn, i18n("Warning"), KGuiItem(i18n("Export")), KGuiItem(i18n("Do Not Export")));
    if (result != KMessageBox::Yes)
        return;

    QString sname = keysList2->currentItem()->text(1).section('@', 0, 0);
    sname = sname.section('.', 0, 0);
    if (sname.isEmpty())
        sname = keysList2->currentItem()->text(0).section(' ', 0, 0);
    sname.append(".asc");
    sname.prepend(QDir::homePath() + '/');
    KUrl url = KFileDialog::getSaveUrl(sname, "*.asc|*.asc Files", this, i18n("Export PRIVATE KEY As"));

    if(!url.isEmpty())
    {
        QFile fgpg(url.path());
        if (fgpg.exists())
            fgpg.remove();

        K3ProcIO *p = new K3ProcIO();
        *p << "gpg" << "--no-tty" << "--output" << QFile::encodeName(url.path()) << "--armor" << "--export-secret-keys" << keysList2->currentItem()->text(6);
        p->start(K3Process::Block);

        if (fgpg.exists())
            KMessageBox::information(this, i18n("Your PRIVATE key \"%1\" was successfully exported.\nDO NOT leave it in an insecure place.", url.path()));
        else
            KMessageBox::sorry(this, i18n("Your secret key could not be exported.\nCheck the key."));
    }
}

void KeysManager::slotexport()
{
    // export key
    if (keysList2->currentItem() == 0)
        return;
    if (keysList2->currentItem()->depth() != 0)
        return;

    QList<KeyListViewItem*> exportList = keysList2->selectedItems();
    if (exportList.count() == 0)
        return;

    QString sname;

    if (exportList.count() == 1)
    {
        sname = keysList2->currentItem()->text(1).section('@', 0, 0);
        sname = sname.section('.', 0, 0);
        if (sname.isEmpty())
            sname = keysList2->currentItem()->text(0).section(' ', 0, 0);
    }
    else
        sname = "keyring";

    sname.append(".asc");
    sname.prepend(QDir::homePath() + '/');

    KDialog *dial = new KDialog(this );
    dial->setCaption(  i18n("Public Key Export") );
    dial->setButtons( KDialog::Ok | KDialog::Cancel );
    dial->setDefaultButton( KDialog::Ok );
    dial->setModal( true );

    KeyExport *page = new KeyExport(dial);
    dial->setMainWidget(page);
    page->newFilename->setUrl(sname);
    page->newFilename->setWindowTitle(i18n("Save File"));
    page->newFilename->setMode(KFile::File);
    page->show();

    if (dial->exec() == QDialog::Accepted)
    {
        // export to file
        QString expname;
        bool exportAttr = page->exportAttributes->isChecked();
        if (page->checkServer->isChecked())
        {
            KeyServer *expServer = new KeyServer(0, false);
            expServer->slotSetExportAttribute(exportAttr);
            QString exportKeysList;
            for (int i = 0; i < exportList.count(); ++i)
                if (exportList.at(i))
                    exportKeysList.append(' ' + exportList.at(i)->text(6).simplified());

                expServer->slotExport(exportKeysList);
                return;
        }
        else
        if (page->checkFile->isChecked())
        {
            K3ProcIO *p = new K3ProcIO();
            *p << "gpg" << "--no-tty";

            expname = page->newFilename->url().url().simplified();
            if (!expname.isEmpty())
            {
                QFile fgpg(expname);
                if (fgpg.exists())
                    fgpg.remove();

                *p << "--output" << QFile::encodeName(expname) << "--export" << "--armor";
                if (!exportAttr)
                    *p << "--export-options" << "no-include-attributes";

                for (int i = 0; i < exportList.count(); ++i)
                    if (exportList.at(i))
                        *p << (exportList.at(i)->text(6)).simplified();

                p->start(K3Process::Block);

                if (fgpg.exists())
                    KMessageBox::information(this, i18n("Your public key \"%1\" was successfully exported\n", expname));
                else
                    KMessageBox::sorry(this, i18n("Your public key could not be exported\nCheck the key."));
            }
        }
        else
        {
            QStringList klist;
            for (int i = 0; i < exportList.count(); ++i)
                if (exportList.at(i))
                    klist.append(exportList.at(i)->text(6).simplified());

            KgpgInterface *kexp = new KgpgInterface();
            QString result = kexp->getKeys(true, exportAttr, klist);
            delete kexp;

            if (page->checkClipboard->isChecked())
                slotProcessExportClip(result);
            else
                slotProcessExportMail(result);
        }
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
    KService::Ptr ptr = list.first();
    //KMessageBox::sorry(0,ptr->desktopEntryName());
    K3ProcIO *p = new K3ProcIO();
    *p << "gpg" << "--no-tty" << "--photo-viewer" << QFile::encodeName(ptr->desktopEntryName() + " %i") << "--edit-key" << keysList2->currentItem()->parent()->text(6) << "uid" << keysList2->currentItem()->text(6) << "showphoto" << "quit";
    p->start(K3Process::DontCare, true);
}

void KeysManager::listsigns()
{
    // kDebug(2100) << "Edit -------------------------------" << endl;
    if (keysList2->currentItem() == 0)
        return;

    if (keysList2->currentItem()->depth() != 0)
    {
        if (keysList2->currentItem()->text(0) == i18n("Photo id"))
        {
            // display photo
            slotShowPhoto();
        }
        return;
    }

    if (keysList2->currentItem()->pixmap(0)->serialNumber() == Images::orphan().serialNumber())
    {
        if (KMessageBox::questionYesNo(this, i18n("This key is an orphaned secret key (secret key without public key.) It is currently not usable.\n\n"
                                               "Would you like to regenerate the public key?"), QString(), KGuiItem(i18n("Generate")), KGuiItem(i18n("Do Not Generate"))) == KMessageBox::Yes)
            slotregenerate();
            return;
    }

    QString key = keysList2->currentItem()->text(6);
    if (!key.isEmpty())
    {
        KgpgKeyInfo *opts = new KgpgKeyInfo(key, this);
        connect(opts, SIGNAL(keyNeedsRefresh()), keysList2, SLOT(refreshselfkey()));
        opts->exec();
        delete opts;
    }
    else
        editGroup();
}

void KeysManager::groupAdd()
{
    QList<Q3ListViewItem*> addList = gEdit->availableKeys->selectedItems();
    for (int i = 0; i < addList.count(); ++i)
        if (addList.at(i))
            gEdit->groupKeys->insertItem(addList.at(i));
}

void KeysManager::groupRemove()
{
    QList<Q3ListViewItem*> remList = gEdit->groupKeys->selectedItems();
    for (int i = 0; i < remList.count(); ++i)
        if (remList.at(i))
            gEdit->availableKeys->insertItem(remList.at(i));
}

void KeysManager::deleteGroup()
{
    if (!keysList2->currentItem() || !keysList2->currentItem()->text(6).isEmpty())
        return;

    int result = KMessageBox::warningContinueCancel(this, i18n("<qt>Are you sure you want to delete group <b>%1</b> ?</qt>", keysList2->currentItem()->text(0)), i18n("Warning"), KGuiItem(i18n("Delete"), "edit-delete"));
    if (result != KMessageBox::Continue)
        return;

    KgpgInterface::delGpgGroup(keysList2->currentItem()->text(0), KGpgSettings::gpgConfigPath());
    KeyListViewItem *item = keysList2->currentItem()->nextSibling();
    delete keysList2->currentItem();

    if (!item)
        item = keysList2->lastChild();

    keysList2->setCurrentItem(item);
    keysList2->setSelected(item,true);

    QStringList groups = KgpgInterface::getGpgGroupNames(KGpgSettings::gpgConfigPath());
    KGpgSettings::setGroups(groups.join(","));
    keysList2->groupNb = groups.count();
    changeMessage(i18n("%1 Keys, %2 Groups", keysList2->childCount()-keysList2->groupNb, keysList2->groupNb), 1);
}

void KeysManager::groupChange()
{
    QStringList selected;
    KeyListViewItem *item = static_cast<KeyListViewItem*>(gEdit->groupKeys->firstChild());
    while (item)
    {
        selected += item->text(2);
        item = item->nextSibling();
    }
    KgpgInterface::setGpgGroupSetting(keysList2->currentItem()->text(0), selected,KGpgSettings::gpgConfigPath());
}

void KeysManager::createNewGroup()
{
    QStringList badkeys;
    QStringList keysGroup;

    if (keysList2->selectedItems().count() > 0)
    {
        QList<KeyListViewItem*> groupList = keysList2->selectedItems();
        bool keyDepth = true;
        for (int i = 0; i < groupList.count(); ++i)
            if (groupList.at(i))
            {
                if (groupList.at(i)->depth() != 0)
                    keyDepth = false;
                else
                if (groupList.at(i)->text(6).isEmpty())
                    keyDepth = false;
                else
                if (groupList.at(i)->pixmap(2))
                {
                    if (groupList.at(i)->pixmap(2)->serialNumber() == keysList2->trustgood.serialNumber())
                        keysGroup += groupList.at(i)->text(6);
                    else
                        badkeys += groupList.at(i)->text(0) + " (" + groupList.at(i)->text(1) + ") " + groupList.at(i)->text(6);
                }
            }

        if (!keyDepth)
        {
            KMessageBox::sorry(this, i18n("<qt>You cannot create a group containing signatures, subkeys or other groups.</qt>"));
            return;
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
            KGpgSettings::setGroups(groups.join(","));
            keysList2->refreshGroups();
            KeyListViewItem *newgrp = keysList2->findItem(groupName, 0);

            keysList2->clearSelection();
            keysList2->setCurrentItem(newgrp);
            keysList2->setSelected(newgrp, true);
            keysList2->ensureItemVisible(newgrp);
            keysList2->groupNb = groups.count();
            changeMessage(i18n("%1 Keys, %2 Groups", keysList2->childCount() - keysList2->groupNb, keysList2->groupNb), 1);
        }
        else
            KMessageBox::sorry(this, i18n("<qt>No valid or trusted key was selected. The group <b>%1</b> will not be created.</qt>", groupName));
    }
}

void KeysManager::groupInit(const QStringList &keysGroup)
{
    kDebug(2100) << "preparing group" << endl;
    QStringList lostKeys;
    bool foundId;

    for (QStringList::ConstIterator it = keysGroup.begin(); it != keysGroup.end(); ++it)
    {
        KeyListViewItem *item = static_cast<KeyListViewItem*>(gEdit->availableKeys->firstChild());
        foundId = false;
        while (item)
        {
            kDebug(2100) << "Searching in key: " << item->text(0) << endl;
            if (QString(*it).right(8).toLower() == item->text(2).right(8).toLower())
            {
                gEdit->groupKeys->insertItem(item);
                foundId = true;
                break;
            }
            item = item->nextSibling();
        }
        if (!foundId)
            lostKeys += QString(*it);
    }

    if (!lostKeys.isEmpty())
        KMessageBox::informationList(this, i18n("Following keys are in the group but are not valid or not in your keyring. They will be removed from the group."), lostKeys);
}

void KeysManager::editGroup()
{
    if (!keysList2->currentItem() || !keysList2->currentItem()->text(6).isEmpty())
        return;
    QStringList keysGroup;
    //KDialogBase *dialogGroupEdit=new KDialogBase( this, "edit_group", true,i18n("Group Properties"),KDialogBase::Ok | KDialogBase::Cancel);
    KDialog *dialogGroupEdit = new KDialog(this );
    dialogGroupEdit->setCaption( i18n("Group Properties") );
    dialogGroupEdit->setButtons( KDialog::Ok | KDialog::Cancel );
    dialogGroupEdit->setDefaultButton(  KDialog::Ok );
    dialogGroupEdit->setModal( true );

    gEdit = new groupEdit();
    gEdit->buttonAdd->setIcon(KIconLoader::global()->loadIconSet("go-down", K3Icon::Small, 20));
    gEdit->buttonRemove->setIcon(KIconLoader::global()->loadIconSet("go-up", K3Icon::Small, 20));

    connect(gEdit->buttonAdd, SIGNAL(clicked()), this, SLOT(groupAdd()));
    connect(gEdit->buttonRemove, SIGNAL(clicked()), this, SLOT(groupRemove()));
    // connect(dialogGroupEdit->okClicked(),SIGNAL(clicked()),this,SLOT(groupChange()));
    connect(gEdit->availableKeys, SIGNAL(doubleClicked (Q3ListViewItem *, const QPoint &, int)), this, SLOT(groupAdd()));
    connect(gEdit->groupKeys, SIGNAL(doubleClicked (Q3ListViewItem *, const QPoint &, int)), this, SLOT(groupRemove()));
    KeyListViewItem *item = keysList2->firstChild();
    if (!item)
        return;

    if (item->pixmap(2))
        if (item->pixmap(2)->serialNumber() == keysList2->trustgood.serialNumber())
            (void) new K3ListViewItem(gEdit->availableKeys, item->text(0), item->text(1), item->text(6));

    while (item->nextSibling())
    {
        item = item->nextSibling();
        if (item->pixmap(2))
            if (item->pixmap(2)->serialNumber() == keysList2->trustgood.serialNumber())
                (void) new K3ListViewItem(gEdit->availableKeys, item->text(0), item->text(1), item->text(6));
    }

    keysGroup = KgpgInterface::getGpgGroupSetting(keysList2->currentItem()->text(0), KGpgSettings::gpgConfigPath());
    groupInit(keysGroup);
    dialogGroupEdit->setMainWidget(gEdit);
    gEdit->availableKeys->setColumnWidth(0, 200);
    gEdit->availableKeys->setColumnWidth(1, 200);
    gEdit->availableKeys->setColumnWidth(2, 100);
    gEdit->availableKeys->setColumnWidthMode(0, K3ListView::Manual);
    gEdit->availableKeys->setColumnWidthMode(1, K3ListView::Manual);
    gEdit->availableKeys->setColumnWidthMode(2, K3ListView::Manual);

    gEdit->groupKeys->setColumnWidth(0, 200);
    gEdit->groupKeys->setColumnWidth(1, 200);
    gEdit->groupKeys->setColumnWidth(2, 100);
    gEdit->groupKeys->setColumnWidthMode(0, K3ListView::Manual);
    gEdit->groupKeys->setColumnWidthMode(1, K3ListView::Manual);
    gEdit->groupKeys->setColumnWidthMode(2, K3ListView::Manual);

    gEdit->setMinimumSize(gEdit->sizeHint());
    gEdit->show();
    if (dialogGroupEdit->exec() == QDialog::Accepted)
        groupChange();
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
        QString fingervalue;
        QString opt;

        QStringList list(keysList2->currentItem()->text(6));
        KgpgInterface *interface = new KgpgInterface();
        KgpgKeyList listkeys = interface->readPublicKeys(true, list);
        delete interface;
        fingervalue = listkeys.at(0).fingerprint();

        opt = i18n("<qt>You are about to sign key:<br><br>%1<br>ID: %2<br>Fingerprint: <br><b>%3</b>.<br><br>"
                   "You should check the key fingerprint by phoning or meeting the key owner to be sure that someone "
                   "is not trying to intercept your communications</qt>", keysList2->currentItem()->text(0) + " (" + keysList2->currentItem()->text(1) + ')', keysList2->currentItem()->text(6), fingervalue);

        if (KMessageBox::warningContinueCancel(this, opt) != KMessageBox::Continue)
            return;
    }
    else
    {
        QStringList signKeyList;
        for (int i = 0; i < signList.count(); ++i)
            if (signList.at(i))
                signKeyList += signList.at(i)->text(0) + " (" + signList.at(i)->text(1) + "): " + signList.at(i)->text(6);

        if (KMessageBox::warningContinueCancelList(this, i18n("<qt>You are about to sign the following keys in one pass.<br><b>If you have not carefully checked all fingerprints, the security of your communications may be compromised.</b></qt>"), signKeyList) != KMessageBox::Continue)
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
        kDebug(2100) << "Sign process for key: " << keyCount + 1 << " on a total of " << signList.count() << endl;
        if (signList.at(keyCount))
        {
            KgpgInterface *interface = new KgpgInterface();
            interface->signKey(signList.at(keyCount)->text(6), globalkeyID, globalisLocal, globalChecked, m_isterminal);
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
        KMessageBox::sorry(this, i18n("<qt>Bad passphrase, key <b>%1</b> not signed.</qt>", signList.at(keyCount)->text(0) + i18n(" (") + signList.at(keyCount)->text(1) + i18n(")")));
    else
    if (success == 4)
        KMessageBox::sorry(this, i18n("<qt>The key <b>%1</b> is already signed.</qt>", signList.at(keyCount)->text(0) + i18n(" (") + signList.at(keyCount)->text(1) + i18n(")")));

    keyCount++;
    signLoop();
}

void KeysManager::importallsignkey()
{
    if (keysList2->currentItem() == 0)
        return;

    if (!keysList2->currentItem()->firstChild())
    {
        keysList2->currentItem()->setOpen(true);
        keysList2->currentItem()->setOpen(false);
    }

    QString missingKeysList;
    KeyListViewItem *current = keysList2->currentItem()->firstChild();
    while (current)
    {
        if (isSignatureUnknown(current))
            missingKeysList += current->text(6) + ' ';
        current = current->nextSibling();
    }

    if (!missingKeysList.isEmpty())
        importsignkey(missingKeysList);
    else
        KMessageBox::information(this, i18n("All signatures for this key are already in your keyring"));
}

void KeysManager::preimportsignkey()
{
    if (keysList2->currentItem() == NULL)
        return;
    else
        importsignkey(keysList2->currentItem()->text(6));
}

bool KeysManager::importRemoteKey(const QString &keyID)
{
    kServer = new KeyServer(0, false, true);
    kServer->slotSetText(keyID);
    //kServer->page->Buttonimport->setDefault(true);
    //kServer->page->tabWidget2->setTabEnabled(kServer->page->tabWidget2->page(1),false);
    kServer->show();
    kServer->raise();
    connect(kServer, SIGNAL(importFinished(QString)), this, SLOT( dcopImportFinished()));

    return true;
}

void KeysManager::dcopImportFinished()
{
    if (kServer)
        kServer = 0L;
#ifdef __GNUC__
#warning "kde4 dbus port it"
#endif
#if 0
    QByteArray params;
    QDataStream stream(&params, QIODevice::WriteOnly);

    stream.setVersion(QDataStream::Qt_3_1);
    stream << true;
    kapp->dcopClient()->emitDCOPSignal("keyImported(bool)", params);
#endif
    refreshkey();
}

void KeysManager::importsignkey(const QString &importKeyId)
{
    // sign a key
    kServer = new KeyServer(0, false);
    kServer->slotSetText(importKeyId);
    //kServer->Buttonimport->setDefault(true);
    kServer->slotImport();
    //kServer->show();
    connect(kServer, SIGNAL(importFinished(QString)), this, SLOT(importfinished()));
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

    if (keysList2->currentItem()->depth() > 1)
    {
        KMessageBox::sorry(this, i18n("Edit key manually to delete this signature."));
        return;
    }

    QString signID;
    QString parentKey;
    QString signMail;
    QString parentMail;

    // open a key selection dialog (KgpgSelectSecretKey, see beginning of this file)
    parentKey = keysList2->currentItem()->parent()->text(6);
    signID = keysList2->currentItem()->text(6);
    parentMail = keysList2->currentItem()->parent()->text(0) + " (" + keysList2->currentItem()->parent()->text(1) + ')';
    signMail = keysList2->currentItem()->text(0) + " (" + keysList2->currentItem()->text(1) + ')';

    if (parentKey == signID)
    {
        KMessageBox::sorry(this, i18n("Edit key manually to delete a self-signature."));
        return;
    }

    QString ask = i18n("<qt>Are you sure you want to delete signature<br><b>%1</b> from key:<br><b>%2</b>?</qt>", signMail, parentMail);

    if (KMessageBox::questionYesNo(this, ask, QString(), KStandardGuiItem::del(), KStandardGuiItem::cancel()) != KMessageBox::Yes)
        return;

    KgpgInterface *delSignKeyProcess = new KgpgInterface();
    connect(delSignKeyProcess, SIGNAL(delsigfinished(bool)), this, SLOT(delsignatureResult(bool)));
    delSignKeyProcess->KgpgDelSignature(parentKey, signID);
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
    if (!keysList2->currentItem())
        return;
    if (keysList2->currentItem()->depth() != 0)
        return;
    if (keysList2->currentItem()->text(6).isEmpty())
        return;

    K3Process kp;
    KConfigGroup config(KGlobal::config(), "General");
    kp << config.readPathEntry("TerminalApplication","konsole");
    kp << "-e" << "gpg" <<"--no-secmem-warning" <<"--edit-key" << keysList2->currentItem()->text(6) << "help";
    kp.start(K3Process::Block);
    keysList2->refreshcurrentkey(keysList2->currentItem());
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
    KPrinter prt;
    //kDebug(2100) << "Printing..." << endl;
    if (prt.setup(this))
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
                        i18n("<p>Delete <b>SECRET KEY</b> pair <b>%1</b>?</p>Deleting this key pair means you will never be able to decrypt files encrypted with this key again.", res),
                        i18n("Warning"),
                        KGuiItem(i18n("Delete"),"edit-delete"));
    if (result != KMessageBox::Continue)
        return;

    K3Process *conprocess = new K3Process();
    KConfigGroup config(KGlobal::config(), "General");
    *conprocess<< config.readPathEntry("TerminalApplication","konsole");
    *conprocess<<"-e"<<"gpg"
    <<"--no-secmem-warning"
    <<"--delete-secret-key"<<keysList2->currentItem()->text(6);
    connect(conprocess, SIGNAL(processExited(K3Process *)), this, SLOT(reloadSecretKeys()));
    conprocess->start(K3Process::NotifyOnExit, K3Process::AllOutput);
}

void KeysManager::reloadSecretKeys()
{
}

void KeysManager::confirmdeletekey()
{
    KeyListViewItem *ki = keysList2->currentItem();

    if (ki->depth() != 0)
    {
	// this is only a public key
        if ((ki->depth() == 1) && !(ki->itemType() & KeyListViewItem::Secret) && (ki->itemType() & KeyListViewItem::Public))
            delsignkey();
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
                    secList += ki->text(0) + " (" + ki->text(1) + ")<br>";
                    ki->setSelected(false);
                }
                else
                    keysToDelete += ki->text(0) + " (" + ki->text(1) + ')';
            }
	}

        if (secretKeyInside)
        {
            int result = KMessageBox::warningContinueCancel(this, i18n("<qt>The following are secret key pairs:<br><b>%1</b>They will not be deleted.<br></qt>", secList));
            if (result != KMessageBox::Continue)
                return;
        }

        if (keysToDelete.isEmpty())
            return;

        int result = KMessageBox::warningContinueCancelList(this, i18np("<qt><b>Delete the following public key?</b></qt>", "<qt><b>Delete the following %1 public keys?</b></qt>", keysToDelete.count()), keysToDelete, i18n("Warning"), KStandardGuiItem::del());
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

    K3Process gp;
    gp << "gpg"
    << "--no-tty"
    << "--no-secmem-warning"
    << "--batch"
    << "--yes"
    << "--delete-key";

    for (int i = 0; i < exportList.count(); ++i)
        if (exportList.at(i))
            gp << (exportList.at(i)->text(6)).simplified();

    gp.start(K3Process::Block);

    for (int i = 0; i < exportList.count(); ++i)
        if (exportList.at(i))
            keysList2->refreshcurrentkey(exportList.at(i));

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

    changeMessage(i18n("%1 Keys, %2 Groups", keysList2->childCount() - keysList2->groupNb, keysList2->groupNb), 1);
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
    page->resize(page->minimumSize());
    dial->resize(dial->minimumSize());

    if (dial->exec() == QDialog::Accepted)
    {
        changeMessage(i18n("Importing..."), 0, true);
        if (page->checkFile->isChecked())
        {
            QString impname = page->newFilename->url().url().simplified();
            if (!impname.isEmpty())
            {
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
    m_listviewsearch->updateSearch(m_listviewsearch->text());
}

#include "keysmanager.moc"
