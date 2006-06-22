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

#ifndef LISTKEYS_H
#define LISTKEYS_H

#include <QStringList>
#include <QDropEvent>
#include <QClipboard>
#include <QPixmap>

#include <Q3ListViewItem>

#include <k3listviewsearchline.h>
#include <kactionclasses.h>
#include <kmainwindow.h>
#include <kdialog.h>
#include <k3listview.h>
#include <kurl.h>

#include "kgpgkey.h"
#include "keylistview.h"

class QDragMoveEvent;
class QVBoxLayout;
class QPushButton;
class QCloseEvent;
class QCheckbox;
class QLabel;
class QEvent;

class KSelectAction;
class KPassivePopup;
class KStatusBar;
class KTempFile;
class KProcess;
class KProcIO;
class KMenu;

class KgpgInterface;
class groupEdit;
class keyServer;
class KgpgEditor;

class listKeys : public KMainWindow
{
    Q_OBJECT

public:
    listKeys(QWidget *parent = 0, const char *name = 0);

    KeyListView *keysList2;

    KToggleAction *sTrust;
    KToggleAction *sCreat;
    KToggleAction *sExpi;
    KToggleAction *sSize;
    KSelectAction *photoProps;
    KgpgEditor *s_kgpgEditor;

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
    void showKeyInfo(QString keyID);
    void findKey();
    void findFirstKey();
    void findNextKey();
    void slotSetDefaultKey(QString newID);

protected:
    void closeEvent(QCloseEvent *e);
    bool eventFilter(QObject *, QEvent *e);

private slots:
    void showKeyManager();
    void slotOpenEditor();

    void statusBarTimeout();
    void changeMessage(QString msg, int nb, bool keep = false);

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
    void slotAddUid();
    void slotAddUidEnable(const QString &name);
    void slotUpdatePhoto();
    void slotDeletePhoto();
    void slotAddPhoto();
    void slotSetPhotoSize(int size);
    void slotShowPhoto();
    void slotrevoke(QString keyID, QString revokeUrl, int reason, QString description);
    void revokeWidget();
    void doFilePrint(QString url);
    void doPrint(QString txt);
    void checkList();
    void signLoop();
    void slotManpage();
    void slotTip();
    void showKeyServer();
    void slotProcessExportMail(QString keys);
    void slotProcessExportClip(QString keys);
    void readOptions();
    void showOptions();
    void slotSetDefKey();
    void slotSetDefaultKey(K3ListViewItem *newdef);
    void annule();
    void confirmdeletekey();
    void deletekey();
    void deleteseckey();
    void signkey();
    void delsignkey();
    void preimportsignkey();
    bool importRemoteKey(QString keyID);
    void importsignkey(QString importKeyId);
    void importallsignkey();
    void importfinished();
    void signatureResult(int success, KgpgInterface*);
    void delsignatureResult(bool);
    void listsigns();
    void slotexport();
    void slotexportsec();
    void slotmenu(Q3ListViewItem *, const QPoint &, int);
    void slotPreImportKey();
    void slotedit();
    void addToKAB();
    void editGroup();
    void groupAdd();
    void groupRemove();
    void groupInit(QStringList keysGroup);
    void groupChange();
    void createNewGroup();
    void deleteGroup();
    void slotImportRevoke(QString url);
    void slotImportRevokeTxt(QString revokeText);
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

    QList<Q3ListViewItem*> signList;
    QList<Q3ListViewItem*> keysList;

    /*
    QList<K3ListViewItem> signList;
    QList<K3ListViewItem> keysList;
    */

    QClipboard::Mode clipboardMode;
    QTimer *m_statusbartimer;

    KMenu *popup;
    KMenu *popupsec;
    KMenu *popupout;
    KMenu *popupsig;
    KMenu *popupgroup;
    KMenu *popupphoto;
    KMenu *popupuid;
    KMenu *popuporphan;

    KPassivePopup *pop;
    KStatusBar *m_statusbar;

    KeyListViewSearchLine* listViewSearch;
    KDialog *addUidWidget;

    KAction *importSignatureKey;
    KAction *importAllSignKeys;
    KAction *signKey;
    KAction *refreshKey;

    keyServer *kServer;
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

#endif // LISTKEYS_H
