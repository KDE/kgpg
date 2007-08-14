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

#include <QDropEvent>
#include <QClipboard>
#include <QPixmap>
#include <Q3ListViewItem>

#include <KToggleAction>
#include <K3ListViewSearchLine>
#include <KXmlGuiWindow>
#include <KDialog>
#include <K3ListView>
#include <KUrl>

#include "kgpgkey.h"
#include "keylistview.h"
#include "ui_adduid.h"
#include "ui_groupedit.h"

class QCloseEvent;
class QEvent;

class KSelectAction;
class KPassivePopup;
class KStatusBar;
class KMenu;

class KgpgInterface;
class groupEdit;
class KeyServer;
class KgpgEditor;

class groupEdit : public QWidget, public Ui::groupEdit
{
public:
  groupEdit( QWidget *parent = 0 ) : QWidget( parent ) {
    setupUi( this );
  }
};


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

    KeyListView *keysList2;

    KToggleAction *sTrust;
    KToggleAction *sCreat;
    KToggleAction *sExpi;
    KToggleAction *sSize;
    KToggleAction *hPublic;
    KToggleAction *hExRev;
    KSelectAction *photoProps;
    KgpgEditor *s_kgpgEditor;

    void saveToggleOpts(void);
signals:
    void readAgainOptions();
    void certificate(QString);
    void closeAsked();
    void fontChanged(QFont);
    void encryptFiles(KUrl::List);
    void installShredder();

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
    bool importRemoteKey(const QString &keyID);
    void showKeyServer();
    void showOptions();

protected:
    void closeEvent(QCloseEvent *e);
    bool eventFilter(QObject *, QEvent *e);
    bool isSignature(KeyListViewItem *);
    bool isSignatureUnknown(KeyListViewItem *);

private slots:
    void slotOpenEditor();

    void statusBarTimeout();
    void changeMessage(const QString &msg, const int nb, const bool keep = false);

    void slotGenerateKeyProcess(KgpgInterface *);
    void slotGenerateKeyDone(int res, KgpgInterface *interface, const QString &name, const QString &email, const QString &id, const QString &fingerprint);

    void slotShowTrust();
    void slotShowSize();
    void slotShowCreation();
    void slotShowExpiration();

    void slotAddUidFin(int res, KgpgInterface *interface);
    void slotDelPhotoFinished(int res, KgpgInterface *interface);
    void quitApp();
    void slotToggleSecret();
    void slotToggleDisabled();
    void slotGotoDefaultKey();
    void slotDelUid();
    void slotPrimUid();
    void slotAddUid();
    void slotAddUidEnable(const QString &name);
    void slotUpdatePhoto();
    void slotDeletePhoto();
    void slotAddPhoto();
    void slotSetPhotoSize(int size);
    void slotShowPhoto();
    void slotrevoke(const QString &keyID, const QString &revokeUrl, const int reason, const QString &description);
    void revokeWidget();
    void doFilePrint(const QString &url);
    void doPrint(const QString &txt);
    void checkList();
    void signLoop();
    void slotManpage();
    void slotTip();
    void slotProcessExportMail(const QString &keys);
    void slotProcessExportClip(const QString &keys);
    void readOptions();
    void slotSetDefKey();
    void slotSetDefaultKey(KeyListViewItem *newdef);
    void annule();
    void confirmdeletekey();
    void deletekey();
    void deleteseckey();
    void signkey();
    void delsignkey();
    void preimportsignkey();
    void importsignkey(const QStringList &importKeyId);
    void importallsignkey();
    void importfinished();
    void signatureResult(int success, KgpgInterface*);
    void delsignatureResult(bool);
    void listsigns();
    void slotexport();
    void slotexportsec();

    void slotMenu(Q3ListViewItem *, const QPoint &, int);

    void slotPreImportKey();
    void slotedit();
    void addToKAB();
    void editGroup();
    void groupAdd();
    void groupRemove();
    void groupInit(const QStringList &keysGroup);
    void groupChange();
    void createNewGroup();
    void deleteGroup();
    void slotImportRevoke(const QString &url);
    void slotImportRevokeTxt(const QString &revokeText);
    void refreshKeyFromServer();
    void refreshFinished();
    void slotregenerate();
    void reloadSecretKeys();
    void dcopImportFinished();

private:
    QString message;
    QString globalkeyMail;
    QString globalkeyID;
    QString searchString;

    QList<KeyListViewItem*> signList;
    QList<KeyListViewItem*> keysList;

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

    KPassivePopup *pop;
    KStatusBar *m_statusbar;

    KeyListViewSearchLine* m_listviewsearch;
    KDialog *addUidWidget;

    QAction *importSignatureKey;
    QAction *importAllSignKeys;
    QAction *signKey;
    QAction *refreshKey;
    QAction *setPrimUid;
    QAction *delSignKey;

    KeyServer *kServer;
    groupEdit *gEdit;
    KgpgInterface *revKeyProcess;

    bool continueSearch;
    bool showPhoto;
    bool globalisLocal;
    bool showTipOfDay;
    bool m_isterminal;

    uint globalCount;
    uint keyCount;
    int globalChecked;

    long searchOptions;
};

#endif // KEYSMANAGER_H
