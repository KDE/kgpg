/***************************************************************************
                          kgpgapplet.h  -  description
                             -------------------
    begin                : Mon Nov 18 2002
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

#ifndef KGPGAPPLET_H
#define KGPGAPPLET_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <qevent.h>
#include <qwidget.h>
#include <qstring.h>
#include <qpixmap.h>
#include <qclipboard.h>
#include <qdragobject.h>
#include <qpopupmenu.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qtimer.h>
#include <qregexp.h>
#include <qwidgetlist.h>

#include <kurl.h>
#include <kaction.h>
#include <kiconloader.h>
#include <kpopupmenu.h>
#include <kiconloader.h>
#include <khelpmenu.h>
#include <ksystemtray.h>
#include <kuniqueapplication.h>
#include <kdeversion.h>
#include <kcmdlineargs.h>

#if (KDE_VERSION >= 310)
#include <kpassivepopup.h>
#endif

#include "kgpgeditor.h"
#include "kgpgoptions.h"
#include "keyservers.h"
#include "popuppublic.h"
#include "listkeys.h"


class MyView : public QLabel
{

   Q_OBJECT

public:
    MyView( QWidget *parent = 0, const char *name = 0);
	~MyView();
int ufileDropEvent,efileDropEvent;
bool	 showeclip,showdclip,showomanager,showoeditor,showserver,autostart;
KURL droppedUrl;

public slots:
void init();


private:
bool ascii,untrusted,hideid,pgpcomp,fastact,encrypttodefault,encryptfileto,tipofday;
KConfig *ksConfig;
QPopupMenu *droppopup,*udroppopup;
KPopupMenu *m_popup,*conf_popup;
KAboutData   *_aboutData;
QString customDecrypt;

QString filekey;

class listKeys *m_keyManager;
class keyServer *m_keyServer;


#if (KDE_VERSION >= 310)
KPassivePopup *pop;
#else
QDialog *clippop;
#endif
public slots:
void  encryptDroppedFile();
void  decryptDroppedFile();
void  slotVerifyFile();
void  signDroppedFile();
void  showDroppedFile ();

private slots:
void slotSetClip(QString newtxt);
void killDisplayClip();
void encryptClipboard(QString &selec,QString encryptOptions);
void help();
void preferences();
void about();
void firstRun();
void readOptions();
void  droppedfile (KURL);
void  droppedtext (QString inputText) ;
void  openKeyManager();
void  openKeyServer();
void  openEditor();
void  clipDecrypt();
void  clipEncrypt();
void clickedMenu(int);
void clickedConfMenu(int);
void slotKeyManagerClosed();
void slotKeyServerClosed();

protected:
void mousePressEvent(QMouseEvent *);
virtual void dragEnterEvent(QDragEnterEvent *);
virtual void  dropEvent (QDropEvent*);

protected slots:
void showPopupMenu( QPopupMenu * );
};

class kgpgapplet : public KSystemTray//KUniqueApplication
{
    Q_OBJECT
	
public:
    kgpgapplet();
    /** destructor */
    ~kgpgapplet();
	MyView *w;
	
private:
	KSystemTray *kgpgapp;
	
};

class KCmdLineArgs;

class KgpgAppletApp : public KUniqueApplication
{
 friend class kgpgapplet;
 public:
    KgpgAppletApp();
    ~KgpgAppletApp();
    int newInstance ();
protected:
 KCmdLineArgs *args;
 private:
    kgpgapplet *kgpg_applet;
};



#endif
