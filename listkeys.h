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

#include <Q3PtrList>
#include <Q3ListViewItem>

#include <klistviewsearchline.h>
#include <kactionclasses.h>
#include <kmainwindow.h>
#include <kdialogbase.h>
#include <klistview.h>
#include <kurl.h>

#include "dcopiface.h"
#include "kgpgkey.h"

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

class KeyView : public KListView
{
    Q_OBJECT
    friend class listKeys;

public:
    KeyView(QWidget *parent = 0);

    void setPreviewSize(const int &size);
    int previewSize() const;

    void setDisplayPhoto(const bool &display);
    bool displayPhoto() const;

    QPixmap trustbad;
    QString secretList;
    bool displayDisabled;
    bool displayOnlySecret;

signals:
    void statusMessage(QString, int, bool keep = false);

public slots:
    void slotAddColumn(const int &c);
    void slotRemoveColumn(const int &c);

protected:
    virtual void contentsDragMoveEvent(QDragMoveEvent *e);
    virtual void contentsDropEvent(QDropEvent *e);
    virtual void startDrag();

private slots:
    void droppedfile(const KURL &url);

    void slotReloadKeys(const QStringList &keyids);
    void refreshAll();

    bool refreshKeys(QStringList ids = QStringList());
    void refreshcurrentkey(KListViewItem *current);
    void refreshselfkey();

    void slotReloadOrphaned();
    void insertOrphans(QStringList ids);

    void refreshGroups();
    void refreshTrust(int color, QColor newColor);

    void expandKey(Q3ListViewItem *item);
    void expandGroup(KListViewItem *item);
    void insertSigns(KListViewItem *item, const KgpgKeySignList &list);

private:
    QPixmap getTrustPix(const QChar &c, const bool &isvalid);

private:
    QStringList orphanList;
    QString photoKeysList;

    QPixmap pixkeyPair;
    QPixmap pixkeySingle;
    QPixmap pixkeyGroup;
    QPixmap pixsignature;
    QPixmap pixuserid;
    QPixmap pixuserphoto;
    QPixmap trustunknown;
    QPixmap trustrevoked;
    QPixmap trustgood;
    QPixmap pixRevoke;
    QPixmap pixkeyOrphan;

    int groupNb;
    int m_previewsize;
    bool m_displayphoto;
};

class mySearchLine: public KListViewSearchLine
{
    Q_OBJECT

public:
    mySearchLine(QWidget *parent = 0, KeyView *listView = 0);
    virtual ~mySearchLine();

public slots:
    virtual void updateSearch(const QString &s = QString::null);

protected:
    virtual bool itemMatches(const KListViewItem *item, const QString & s)  const;

private:
    KeyView *m_searchlistview;
};

class listKeys : public KMainWindow, virtual public KeyInterface
{
    Q_OBJECT

public:
    listKeys(QWidget *parent = 0, const char *name = 0);

    QLabel *keyPhoto;
    KeyView *keysList2;

    KMenu *popup;
    KMenu *popupsec;
    KMenu *popupout;
    KMenu *popupsig;
    KMenu *popupgroup;
    KMenu *popupphoto;
    KMenu *popupuid;
    KMenu *popuporphan;

    QString message;
    QStringList keynames;

    KPassivePopup *pop;
    KToggleAction *sTrust;
    KToggleAction *sCreat;
    KToggleAction *sExpi;
    KToggleAction *sSize;
    KSelectAction *photoProps;
    KStatusBar *keyStatusBar;
    KgpgEditor *s_kgpgEditor;

signals:
    void readAgainOptions();
    void certificate(QString);
    void closeAsked();
    void fontChanged(QFont);
    void encryptFiles(KURL::List);
    void installShredder();

public slots:
    void slotgenkey();
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
    void slotAddUidFin(int res, KgpgInterface *interface);
    void slotDelPhotoFinished(int res, KgpgInterface *interface);
    void quitApp();
    void slotOpenEditor();
    void changeMessage(QString, int, bool keep=false);
    void statusBarTimeout();
    void slotShowTrust();
    void slotShowSize();
    void slotShowCreat();
    void slotShowExpi();
    void slotToggleSecret();
    void slotToggleDisabled();
    void slotGotoDefaultKey();
    void slotDelUid();
    void slotAddUid();
    void slotAddUidEnable(const QString & name);
    void slotUpdatePhoto();
    void slotDeletePhoto();
    void slotAddPhoto();
    void slotSetPhotoSize(int size);
    void slotShowPhoto();
    void readgenprocess(KProcIO *p);
    void newKeyDone(KProcess *);
    void slotrevoke(QString keyID, QString revokeUrl, int reason, QString description);
    void revokeWidget();
    void doFilePrint(QString url);
    void doPrint(QString txt);
    void checkList();
    void signLoop();
    void slotManpage();
    void slotTip();
    void showKeyServer();
    void showKeyManager();
    void slotReadFingerProcess(KProcIO *p);
    void slotProcessExportMail(QString keys);
    void slotProcessExportClip(QString keys);
    void readOptions();
    void genover(KProcess *p);
    void showOptions();
    void slotSetDefKey();
    void slotSetDefaultKey(KListViewItem *newdef);
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
    //  void allToKAB();
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
    QPushButton *bouton1;
    QPushButton *bouton2;
    QPushButton *bouton0;

    QString tempKeyFile;
    QString newKeyMail;
    QString newKeyName;
    QString newkeyFinger;
    QString newkeyID;
    QString globalkeyMail;
    QString globalkeyID;
    QString searchString;

    QList<Q3ListViewItem*> signList;
    QList<Q3ListViewItem*> keysList;

    /*
    QList<KListViewItem> signList;
    QList<KListViewItem> keysList;
    */

    QClipboard::Mode clipboardMode;
    QTimer *statusbarTimer;

    KListViewSearchLine* listViewSearch;
    KTempFile *kgpgtmp;
    KDialogBase *addUidWidget;

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
