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

#include <qlayout.h>
#include <qlabel.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qvbuttongroup.h>
#include <qcombobox.h>
#include <qcheckbox.h>


#include <kmainwindow.h>
#include <kurl.h>
#include <ktempfile.h>
#include <kstatusbar.h>
#include <ktoolbar.h>
#include <klineedit.h>
#include <klistview.h>
#include <kdialogbase.h>
#include <kbuttonbox.h>
#include <kmessagebox.h>
#include <kprocess.h>
#include <kprocio.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kaction.h>
#include <kapp.h>

#include "kgpg.h"
#include "keygener.h"

//#include "popupname.h"

class KgpgSelKey : public KDialogBase
{
    Q_OBJECT

public:
    KgpgSelKey( QWidget *parent = 0, const char *name = 0,bool showlocal=true);
    KListView *keysListpr;
QPixmap keyPair;
QCheckBox *local;
private slots:
void slotOk();
void slotpreOk();
void slotSelect(QListViewItem *item);
public slots:
QString getkeyID();
QString getkeyMail();
bool getlocal();
};

class KgpgKeyInfo : public KDialogBase
{
    Q_OBJECT

public:
    KgpgKeyInfo( QWidget *parent = 0, const char *name = 0,QString sigkey=0);
	
private slots:
void slotinfoimgread(KProcess *);//IO *);
private:
KTempFile *kgpginfotmp;
QLabel *keyinfoPhoto;
};

class listKeys : public KMainWindow//QDialog //KMainWindow
{
  Q_OBJECT
  public:
  listKeys(QWidget *parent=0, const char *name=0,WFlags f = 0);
  ~listKeys();
  QLabel *keyPhoto;
  KListView *keysList2;
  QPopupMenu *popup,*popupsec,*popupout,*popupsig;
  QString secretList,exportresult;
   KStatusBar *statusbar;
   KToolBar *toolbar;
QString message,defKey,issec;
QStringList keynames;
QPixmap keyPair,keySingle,dkeyPair,dkeySingle,nkeyPair,nkeySingle,gkeyPair,gkeySingle,signature,userid;
QDialog *pop;

  private:
QPushButton *bouton1,*bouton2,*bouton0;
KConfig *config;
QString tempKeyFile;
QString photoKeysList;
KTempFile *kgpgtmp;
bool showPhoto;

public slots:
    void slotgenkey();

private slots:
void displayPhoto();
void hidePhoto();
void slotProcessPhoto(KProcess *);
void readOptions();
void genover(KProcess *p);
void slotParentOptions();
void slotSetDefKey();
void annule();
void confirmdeletekey();
void deletekey();
void deleteseckey();
void signkey();
void delsignkey();
void signatureResult(int);
void delsignatureResult(bool);
void listsigns();
void slotexport();
void slotexportsec();
void slotmenu(QListViewItem *,const QPoint &,int);
void slotprocresult(KProcess *);
void slotprocread(KProcIO *);
void refreshkey();
void slotImportKey();
void slotstatus(QListViewItem *);
void slotedit();

signals:
//void selectedKey(QString &);


};

#endif
