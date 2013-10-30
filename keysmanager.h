/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2007,2008,2009,2010,2011,2012,2013
 *               Rolf Eike Beer <kde@opensource.sf-tec.de>
 */
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KEYSMANAGER_H
#define KEYSMANAGER_H

#include "ui_adduid.h"

#include "core/kgpgkey.h"
#include "model/kgpgitemmodel.h"
#include "model/kgpgitemnode.h"

#include <KDialog>
#include <KToggleAction>
#include <KUrl>
#include <KXmlGuiWindow>
#include <QClipboard>
#include <QSet>
#include <solid/networking.h>

class KJob;
class QEvent;

class KSelectAction;
class KStatusBar;
class KMenu;
class KLineEdit;
class KAction;
class KJob;
class KShortcut;

class KeyServer;
class KgpgEditor;
class KeyServer;
class KeyListProxyModel;
class KeyTreeView;
class KGpgAddUid;
class KGpgDelKey;
class KGpgImport;
class KGpgTransactionJob;

class KStatusNotifierItem;


class AddUid : public QWidget, public Ui::AddUid
{
public:
  explicit AddUid( QWidget *parent )
    : QWidget( parent )
  {
    setupUi( this );
  }
};

class KeysManager : public KXmlGuiWindow
{
    Q_OBJECT

public:
    explicit KeysManager(QWidget *parent = 0);
    ~KeysManager();

    KGpgItemModel *getModel();

    KgpgEditor *s_kgpgEditor;

    void saveToggleOpts(void);
    void showTrayMessage(const QString &message);

    /**
     * @brief returns the shortcut to go to the default key in a key selection
     */
    KShortcut goDefaultShortcut() const;

private:
    KToggleAction *sTrust;
    KToggleAction *sCreat;
    KToggleAction *sExpi;
    KToggleAction *sSize;
    KToggleAction *hPublic;
    KToggleAction *longId;
    KSelectAction *photoProps;
    KSelectAction *trustProps;

signals:
    void readAgainOptions();
    void certificate(QString);
    void closeAsked();
    void fontChanged(QFont);

public slots:
    void slotGenerateKey();
    void refreshkey();
    void readAllOptions();
    void showKeyInfo(const QString &keyID);
    void slotSetDefaultKey(const QString &newID);
    void showKeyManager();
    bool importRemoteKey(const QString &keyIDs);
    bool importRemoteKeys(const QStringList &keyIDs, const bool dialog = true);
    void showKeyServer();
    void showOptions();
    void slotOpenEditor();
    void slotImport(const QString &text);
    void slotImport(const KUrl::List &files);
    /**
     * When you click on "encrypt the clipboard" in the systray,
     * this slot will open the dialog to choose a key and encrypt the
     * clipboard.
     */
    void clipEncrypt();
    void clipDecrypt();
    void clipSign();
    void slotImportDone(int ret);
    void refreshKeys(const QStringList &ids);

protected:
    bool eventFilter(QObject *, QEvent *e);
    void removeFromGroups(KGpgKeyNode *nd);
    void setDefaultKeyNode(KGpgKeyNode *key);

private slots:
    void slotGenerateKeyDone(KJob *job);

    void slotShowTrust();
    void slotShowSize();
    void slotShowCreation();
    void slotShowExpiration();

    void slotAddUidFin(int res);
    void slotDelPhotoFinished(int res);
    void quitApp();
    void slotShowLongId(bool);
    void slotSetTrustFilter(int);
    void slotGotoDefaultKey();
    void slotDelUid();
    void slotDelUidDone(int);
    void slotPrimUid();
    void slotPrimUidDone(int result);
    void slotAddUid();
    void slotAddUidEnable(const QString &name);
    void slotUpdatePhoto();
    void slotDeletePhoto();
    void slotAddPhoto();
    void slotAddPhotoFinished(int res);
    void slotSetPhotoSize(int size);
    void slotShowPhoto();
    void revokeWidget();
    void slotRevokeDialogFinished(int result);
    void slotRevokeGenerated(int result);
    void doPrint(const QString &txt);
    void checkList();
    void slotManpage();
    void slotTip();
    void slotExportFinished(int result);
    void slotProcessExportMail(int result);
    void slotProcessExportClip(int result);
    void readOptions();
    void slotSetDefKey();
    void confirmdeletekey();
    void deleteseckey();
    void signkey();
    void signuid();
    void caff();
    void slotCaffDone();
    void delsignkey();
    void preimportsignkey();
    void importallsignkey();
    void signatureResult(int success);
    void delsignatureResult(int success);
    void defaultAction(const QModelIndex &);
    void defaultAction(KGpgNode *);
    void slotDefaultAction();
    void showProperties(KGpgNode *);
    void keyproperties();
    void slotexport();
    void slotexportsec();
    void slotExportSecFinished(int result);

    void slotMenu(const QPoint &);

    void slotPreImportKey();
    void slotSendEmail();
    void slotedit();

    /**
     * @brief start an "add to addressbook" operation
     *
     * This searches if given id already exists in the addressbook.
     * The search result is handled in slotAddressbookSearchResult()
     */
    void addToKAB();

    /**
     * @brief add or change the addressbook entry
     * @param job the search job
     *
     * This handles the result of the search started in addToKAB().
     */
    void slotAddressbookSearchResult(KJob *job);

    void editGroup();
    void createNewGroup();
    void deleteGroup();
    void renameGroup();
    void slotImportRevokeTxt(const QString &revokeText);
    void refreshKeyFromServer();
    void slotKeyRefreshDone(int result);
    void slotregenerate();
    void secretKeyDeleted(int);
    void getMissingSigs(QSet<QString> &missingKeys, const KGpgExpandableNode *nd);
    void slotEditDone(int exitcode);
    void importRemoteFinished(int result);
    void slotDelKeyDone(int ret);
    void slotSetClip(int result);
    void slotOpenKeyUrl();

    void slotNetworkUp();
    void slotNetworkDown();

private:
    KGpgItemModel *imodel;
    KeyListProxyModel *iproxy;
    KeyTreeView *iview;

    KGpgAddUid *m_adduid;
    KGpgTransactionJob *m_genkey;
    KGpgDelKey *m_delkey;

    QString globalkeyID;
    QString searchString;

    QList<KGpgSignableNode *> signList;
    QList<KGpgKeyNode *> refreshList;
    QHash<KJob *, KGpgNode *> m_addIds;	///< user ids to add to addressbook

    QClipboard::Mode m_clipboardmode;

    KMenu *m_popuppub;          // popup on a public key
    KMenu *m_popupsec;          // popup on a secret key
    KMenu *m_popupgroup;        // popup on a group
    KMenu *m_popupout;          // popup there is no key or when the user don't right-click on a key
    KMenu *m_popupsig;          // popup on a signature
    KMenu *m_popupphoto;        // popup on a photo
    KMenu *m_popupuid;          // popup on an user id
    KMenu *m_popuporphan;       // popup on an orphan key

    KLineEdit *m_listviewsearch;
    KDialog *addUidWidget;

    KAction *importSignatureKey;
    KAction *importAllSignKeys;
    KAction *signKey;
    KAction *signUid;
    KAction *signMailUid;
    KAction *refreshKey;
    KAction *editKey;
    KAction *setPrimUid;
    KAction *delUid;
    KAction *delSignKey;
    KAction *deleteKey;
    KAction *editCurrentGroup;
    KAction *delGroup;
    KAction *setDefaultKey;
    KAction *kserver;
    KAction *openEditor;
    KAction *goToDefaultKey;
    KAction *exportPublicKey;
    KAction *m_sendEmail;
    KAction *createGroup;
    KAction *m_groupRename;
    KAction *m_revokeKey;

    bool showTipOfDay;
    bool m_signuids;

    int keyCount;

    KGpgKeyNode *terminalkey; // the key currently edited in a terminal

    void startImport(KGpgImport *import);

    // react to network status changes
    bool m_online;
    Solid::Networking::Notifier *m_netnote;
    void toggleNetworkActions(bool online);

    KStatusNotifierItem *m_trayicon;
    void setupTrayIcon();

    void setActionDescriptions(int cnt);
    /**
     * @brief show a message in the status bar
     * @param msg the text to show
     * @param keep if the text should stay visible or may be hidden after a while
     */
    void changeMessage(const QString &msg, const bool keep = false);
    /**
     * @brief update the key and group counter in the status bar
     */
    void updateStatusCounter();
    /**
     * @brief sign the next key from signList
     * @param localsign if signature should be a local (not exportable) one
     * @param checklevel how careful the identity of the key was checked
     */
    void signLoop(const bool localsign, const int checklevel);
    /**
     * @brief Opens the console when the user want to sign a key manually.
     * @param signer key to sign with
     * @param keyid key to sign
     * @param checking how carefule the identify was checked
     * @param local if signature should be local (not exportable)
     */
    void signKeyOpenConsole(const QString &signer, const QString &keyid, const int checking, const bool local);
};

#endif // KEYSMANAGER_H
