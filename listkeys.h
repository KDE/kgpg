/***************************************************************************
                          listkeys.h  -  description
                             -------------------
    begin                : Thu Jul 4 2002
    copyright            : (C) 2002 by y0k0
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

#include <qpushbutton.h>
#include <qdialog.h>
#include <qwidget.h>
#include <qpopupmenu.h>
#include <qptrlist.h>
#include <qlistview.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qvbuttongroup.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qdragobject.h>
#include <qevent.h>
#include <qdatetime.h>
#include <qpalette.h>
#include <qcolor.h>
#include <qtooltip.h>
#include <qwizard.h>

#include <kmainwindow.h>
#include <kurl.h>
#include <ktempfile.h>
#include <kstatusbar.h>
#include <ktoolbar.h>
#include <klineedit.h>
#include <klistview.h>
#include <kdialogbase.h>
#include <kbuttonbox.h>
#include <kcombobox.h>
#include <kmessagebox.h>
#include <kprocess.h>
#include <kprocio.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kkeydialog.h>
#include <kpassdlg.h>
#include <kaction.h>
#include <kapplication.h>
#include <kedittoolbar.h>
#include <dcopclient.h>
#include <kstandarddirs.h>
#include <kfinddialog.h>
#include <kfind.h>


#include "kgpg.h"
#include "keygener.h"
#include "popupimport.h"
#include "popupname.h"
#include "kgpgoptions.h"
#include "keyservers.h"
#include "keyinfowidget.h"
#include "groupedit.h"
#include "dcopiface.h"

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
        void slotSelect(QListViewItem *item);
        QString extractKeyName(QString fullName);
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

private:
        bool displayMailFirst;
        QString secretList,defKey;
        QString photoKeysList,configFilePath;
        QPixmap pixkeyPair,pixkeySingle,pixkeyGroup,pixsignature,pixuserid,pixuserphoto,trustunknown, trustbad, trustgood;

private slots:
        void  droppedfile (KURL);
        void refreshkeylist();
        gpgKey extractKey(QString keyColon);
        QString extractKeyName(QString name,QString mail);
        void expandKey(QListViewItem *item);
	void expandGroup(QListViewItem *item);
        void refreshcurrentkey(QListViewItem *current);
	void refreshgroups();

protected:
        virtual void startDrag();
        virtual void contentsDragMoveEvent(QDragMoveEvent *e);
        virtual void  contentsDropEvent (QDropEvent*);
};

class listKeys : public KMainWindow, virtual public KeyInterface
{
        friend class KeyView;
        Q_OBJECT

public:
        listKeys(QWidget *parent=0, const char *name=0,WFlags f = 0);
        ~listKeys();
        QLabel *keyPhoto;
        KeyView *keysList2;
        QPopupMenu *popup,*popupsec,*popupout,*popupsig,*popupgroup;
        QString message, optionsDefaultKey,configUrl;
        QStringList keynames;
        QDialog *pop;

private:
        QPushButton *bouton1,*bouton2,*bouton0;
        KConfig *config;
        QString tempKeyFile;
        keyServer *kServer;
        KTempFile *kgpgtmp;
        bool showPhoto,configshowToolBar;
        KAction *importSignatureKey, *editKey,*setDefaultKey,*importAllSignKeys,*signKey,*addToAddressBook,*createGroup,*delGroup,*editCurrentGroup;
        QPtrList<QListViewItem> signList;
        uint globalCount;
	int globalChecked;
        bool globalisLocal;
        QString globalkeyMail,globalkeyID,searchString;
	long searchOptions;
	groupEdit *gEdit;

protected:
        void closeEvent( QCloseEvent * e );

public slots:
        void slotgenkey();
        void refreshkey();
	void readAllOptions();
	void showKeyInfo(QString keyID);
	void findKey();
	void findFirstKey();
	void findNextKey();

private slots:
        void checkList();
	void signLoop();
        void configuretoolbars();
        void saveToolbarConfig();
        void checkPhotos();
        void slotManpage();
        void slotTip();
        void slotConfigureShortcuts();
        void keyserver();
        void slotReadProcess(KProcIO *p);
        void slotProcessExportMail(QString keys);
        void slotProcessExportClip(QString keys);
        void displayPhoto();
        void hidePhoto();
        void slotProcessPhoto(KProcess *);
        void readOptions();
        void genover(KProcess *p);
        void slotOptions();
        void slotSetDefKey();
        void annule();
        void confirmdeletekey();
        void deletekey();
        void deleteseckey();
        void signkey();
        void delsignkey();
        void preimportsignkey();
        void importsignkey(QString importKeyId);
        void importallsignkey();
        void importfinished();
        void signatureResult(int);
        void delsignatureResult(bool);
        void listsigns();
        void slotexport();
        void slotexportsec();
        void slotmenu(QListViewItem *,const QPoint &,int);
        void slotPreImportKey();
        void slotedit();
	void addToKAB();
	void allToKAB();
	void editGroup(QString newGroupName="");
	void groupAdd();
	void groupRemove();
	void groupInit(QStringList keysGroup);
	void groupChange();
	void groupNewChange();
	void createNewGroup();
	void deleteGroup();

signals:
        void readAgainOptions();


};


#endif

