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

#include <kdialogbase.h>
#include <klistview.h>
#include <kmainwindow.h>
#include <klistviewsearchline.h>

#include <kactionclasses.h> 
#include <qclipboard.h>
//Added by qt3to4:
#include <QPixmap>
#include <QCloseEvent>
#include <QDragMoveEvent>
#include <QEvent>
#include <QDropEvent>
#include <QLabel>
#include <QVBoxLayout>
#include <Q3PopupMenu>

#include "dcopiface.h"

#include <q3ptrlist.h>
#include <qstringlist.h>
#include <kurl.h>

#include <qcheckbox.h>
#include <kmainwindow.h>

class QPushButton;
class Q3PopupMenu;
class QLabel;
class QCheckbox;
class KStatusBar;
class KPassivePopup;
class KProcess;
class KProcIO;
class QEvent;
class KTempFile;
class KgpgApp;
class keyServer;
class groupEdit;
class KgpgInterface;
class KSelectAction;

typedef struct gpgKey
{
        QString gpgkeymail;
        QString gpgkeyname;
        QString gpgkeyid;
        QString gpgkeytrust;
        QString gpgkeyvalidity;
        QString gpgkeysize;
        QString gpgkeycreation;
        QString gpgkeyexpiration;
        QString gpgkeyalgo;
        QPixmap trustpic;
};

class KgpgSelKey : public KDialogBase
{
        Q_OBJECT

public:
        KgpgSelKey( QWidget *parent = 0, const char *name = 0);
        KListView *keysListpr;
        QPixmap keyPair;
        QCheckBox *local;
        QVBoxLayout *vbox;
        QWidget *page;
private slots:
        void slotOk();
        void slotpreOk();
        void slotSelect(Q3ListViewItem *item);

public slots:
        QString getkeyID();
        QString getkeyMail();
};



class KeyView : public KListView
{
        Q_OBJECT
        friend class listKeys;
public:
        KeyView( QWidget *parent = 0, const char *name = 0);
        bool displayPhoto,displayOnlySecret,displayDisabled;
        int previewSize;
	QString secretList;
	QPixmap trustbad;
	
private:

        QString orphanList;
        QString photoKeysList;
        QPixmap pixkeyPair, pixkeySingle, pixkeyGroup, pixsignature, pixuserid, pixuserphoto;
	QPixmap trustunknown, trustrevoked, trustgood, pixRevoke, pixkeyOrphan;
        Q3ListViewItem *itemToOpen;
        KTempFile *kgpgphototmp;
        int groupNb;

public slots:
        void slotRemoveColumn(int d);
        void slotAddColumn(int c);

private slots:
	void refreshTrust(int color,QColor newColor);
        void  droppedfile (KURL);
        void refreshkeylist();
        gpgKey extractKey(QString keyColon);
        void expandKey(Q3ListViewItem *item);
        void expandGroup(Q3ListViewItem *item);
        void refreshcurrentkey(Q3ListViewItem *current);
        void refreshcurrentkey(QString currentID);
        void refreshselfkey();
        void refreshgroups();
        void insertOrphanedKeys(QStringList orpans);
        void insertOrphan(QString currentID);
        QPixmap slotGetPhoto(QString photoId,bool mini=false);
        void slotReloadKeys(QStringList keyIDs);
        void slotReloadOrphaned();

signals:
        void statusMessage(QString,int,bool keep=false);

protected:
        virtual void startDrag();
        virtual void contentsDragMoveEvent(QDragMoveEvent *e);
        virtual void  contentsDropEvent (QDropEvent*);
};

class mySearchLine: public KListViewSearchLine
{
    Q_OBJECT
public:
    mySearchLine(QWidget *parent = 0, KeyView *listView = 0, const char *name = 0);
    virtual ~mySearchLine();
private:
 KeyView *searchListView;    
    
public slots:
virtual void updateSearch(const QString &s = QString::null);
protected:
virtual bool itemMatches(const Q3ListViewItem *item, const QString & s)  const;
};


class listKeys : public KMainWindow, virtual public KeyInterface
{
        friend class KeyView;
        Q_OBJECT

public:
        listKeys(QWidget *parent=0, const char *name=0);
        ~listKeys();
        QLabel *keyPhoto;
        KeyView *keysList2;
        Q3PopupMenu *popup,*popupsec,*popupout,*popupsig,*popupgroup,*popupphoto,*popupuid,*popuporphan;
        QString message;
        QStringList keynames;
        KPassivePopup *pop;
        KToggleAction *sTrust,*sCreat,*sExpi,*sSize;
        KSelectAction *photoProps;
        KStatusBar *keyStatusBar;
	KgpgApp *s_kgpgEditor;

private:
        QPushButton *bouton1,*bouton2,*bouton0;
        QString tempKeyFile,newKeyMail,newKeyName,newkeyFinger,newkeyID;
	KListViewSearchLine* listViewSearch;	
        bool continueSearch;
        bool showPhoto;
        keyServer *kServer;
        KTempFile *kgpgtmp;
        KAction *importSignatureKey,*importAllSignKeys,*signKey,*refreshKey;
        Q3PtrList<Q3ListViewItem> signList,keysList;
        uint globalCount,keyCount;
        int globalChecked;
        bool globalisLocal,showTipOfDay;
        QString globalkeyMail,globalkeyID,searchString;
        long searchOptions;
        groupEdit *gEdit;
        KgpgInterface *revKeyProcess;
        KDialogBase *addUidWidget;
        QClipboard::Mode clipboardMode;
        QTimer *statusbarTimer;


protected:
        void closeEvent( QCloseEvent * e );
        bool eventFilter( QObject *, QEvent *e );

public slots:
        void slotgenkey();
        void refreshkey();
        void readAllOptions();
        void showKeyInfo(QString keyID);
        void findKey();
        void findFirstKey();
        void findNextKey();
        void slotSetDefaultKey(QString newID);

private slots:
        void quitApp();
        void  slotOpenEditor();
        void changeMessage(QString,int, bool keep=false);
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
        void slotGpgError(QString errortxt);
        void slotUpdatePhoto();
        void slotDeletePhoto();
        void slotAddPhoto();
        void slotSetPhotoSize(int size);
        void slotShowPhoto();
        void readgenprocess(KProcIO *p);
        void newKeyDone(KProcess *);
        void slotrevoke(QString keyID,QString revokeUrl,int reason,QString description);
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
        void slotSetDefaultKey(Q3ListViewItem *newdef);
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
        void signatureResult(int success);
        void delsignatureResult(bool);
        void listsigns();
        void slotexport();
        void slotexportsec();
        void slotmenu(Q3ListViewItem *,const QPoint &,int);
        void slotPreImportKey();
        void slotedit();
        void addToKAB();
        //	void allToKAB();
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
	
signals:
        void readAgainOptions();
        void certificate(QString);
        void closeAsked();
        void fontChanged(QFont);
	void encryptFiles(KURL::List);
	void installShredder();

};


#endif // LISTKEYS_H

