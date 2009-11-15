/***************************************************************************
                          listkeys.h  -  description
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

#ifndef KEYSMANAGER_H
#define KEYSMANAGER_H

#include <QClipboard>
#include <QSet>

#include <KToggleAction>
#include <KXmlGuiWindow>
#include <KDialog>
#include <KUrl>

#include <solid/networking.h>

#include "kgpgkey.h"
#include "ui_adduid.h"
#include "kgpginterface.h"
#include "kgpgitemmodel.h"
#include "kgpgitemnode.h"

class QEvent;

class KSelectAction;
class KStatusBar;
class KMenu;
class KLineEdit;
class KAction;
class KJob;

class KgpgInterface;
class KeyServer;
class KgpgEditor;
class KeyServer;
class KeyListProxyModel;
class KeyTreeView;
class KGpgAddUid;
class KGpgDelKey;
class KGpgImport;
class KGpgTextInterface;
class KGpgTransactionJob;

class KStatusNotifierItem;


class AddUid : public QWidget, public Ui::AddUid
{
public:
  AddUid( QWidget *parent ) : QWidget( parent ) {
    setupUi( this );
  }
};

class KeysManager : public KXmlGuiWindow
{
    Q_OBJECT

public:
    KeysManager(QWidget *parent = 0);
    ~KeysManager();

    KGpgItemModel *getModel();

    KgpgEditor *s_kgpgEditor;

    void saveToggleOpts(void);
    void showTrayMessage(const QString &message);

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
    void encryptFiles(KUrl::List);

public slots:
    void slotGenerateKey();
    void refreshkey();
    void readAllOptions();
    void showKeyInfo(const QString &keyID);
    void findKey();
    void findFirstKey();
    void findNextKey();
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
    
protected:
    bool eventFilter(QObject *, QEvent *e);
    void removeFromGroups(KGpgKeyNode *nd);

private slots:
    void statusBarTimeout();
    void changeMessage(const QString &msg, const int nb, const bool keep = false);

    void slotGenerateKeyDone(KJob *job);

    void slotShowTrust();
    void slotShowSize();
    void slotShowCreation();
    void slotShowExpiration();

    void slotAddUidFin(int res);
    void slotDelPhotoFinished(int res);
    void quitApp();
    void slotToggleSecret(bool);
    void slotShowLongId(bool);
    void slotSetTrustFilter(int);
    void slotGotoDefaultKey();
    void slotDelUid();
    void slotDelUidDone(int);
    void slotPrimUid();
    void slotAddUid();
    void slotAddUidEnable(const QString &name);
    void slotUpdatePhoto();
    void slotDeletePhoto();
    void slotAddPhoto();
    void slotAddPhotoFinished(int res);
    void slotSetPhotoSize(int size);
    void slotShowPhoto();
    void slotrevoke(const QString &keyID, const KUrl &revokeUrl, const int reason, const QString &description);
    void revokeWidget();
    void doFilePrint(const KUrl &url);
    void doPrint(const QString &txt);
    void checkList();
    void signLoop();
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
    void delsignkey();
    void preimportsignkey();
    void importallsignkey();
    void signatureResult(int success, const QString &keyId, KgpgInterface*);
    void delsignatureResult(bool);
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
    void slotedit();
    void addToKAB();
    void editGroup();
    void createNewGroup();
    void deleteGroup();
    void slotImportRevoke(const KUrl &url);
    void slotImportRevokeTxt(const QString &revokeText);
    void refreshKeyFromServer();
    void slotKeyRefreshDone(int result);
    void slotregenerate();
    void secretKeyDeleted(int);
    void getMissingSigs(QSet<QString> &missingKeys, const KGpgExpandableNode *nd);
    void slotEditDone(int exitcode);
    void importRemoteFinished(int result);
    void slotRefreshKeys(KgpgInterface *iface, const QStringList &keys);
    void slotDelKeyDone(int ret);
    void slotImportDone(int ret);
    void slotImportDone(KGpgImport *import, int ret);
    void slotSetClip(const QString &newtxt, KGpgTextInterface *iface);

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

    QList<KGpgNode *> signList;
    QList<KGpgKeyNode *> refreshList;

    QClipboard::Mode m_clipboardmode;
    QTimer *m_statusbartimer;

    KMenu *m_popuppub;          // popup on a public key
    KMenu *m_popupsec;          // popup on a secret key
    KMenu *m_popupgroup;        // popup on a group
    KMenu *m_popupout;          // popup there is no key or when the user don't right-click on a key
    KMenu *m_popupsig;          // popup on a signature
    KMenu *m_popupphoto;        // popup on a photo
    KMenu *m_popupuid;          // popup on an user id
    KMenu *m_popuporphan;       // popup on an orphan key

    KStatusBar *m_statusbar;

    KLineEdit *m_listviewsearch;
    KDialog *addUidWidget;

    KAction *importSignatureKey;
    KAction *importAllSignKeys;
    KAction *signKey;
    KAction *signUid;
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
    KAction *createGroup;

    KeyServer *kServer;
    KgpgInterface *revKeyProcess;

    bool continueSearch;
    bool globalisLocal;
    bool showTipOfDay;
    bool m_isterminal;
    bool m_signuids;

    int keyCount;
    int globalChecked;

    long searchOptions;

    KGpgKeyNode *terminalkey; // the key currently edited in a terminal
    KGpgKeyNode *delkey;	// key currently deleted
    KGpgKeyNodeList m_delkeys;

    void startImport(KGpgImport *import);

    // react to network status changes
    bool m_online;
    Solid::Networking::Notifier *m_netnote;
    void toggleNetworkActions(bool online);

    KStatusNotifierItem *m_trayicon;
    void setupTrayIcon();

    void setActionDescriptions(int cnt);
};

#endif // KEYSMANAGER_H
