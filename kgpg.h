/***************************************************************************
                          kgpg.h  -  description
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
#include <kdesktopfile.h>

#if (KDE_VERSION >= 310)
#include <kpassivepopup.h>
#endif

#include "kgpgeditor.h"
#include "kgpgoptions.h"
#include "keyservers.h"
#include "popuppublic.h"
#include "listkeys.h"
#include "kgpgwizard.h"
#include "kgpgshredwidget.h"


class MyView : public QLabel
{

        Q_OBJECT

public:
        MyView( QWidget *parent = 0, const char *name = 0);
        ~MyView();
        int ufileDropEvent,efileDropEvent;
        bool	 showeclip,showdclip,showomanager,showoeditor,showserver,autostart;
        KURL droppedUrl;
        KURL::List droppedUrls;
        KConfig *ksConfig;

private:
        bool ascii,untrusted,hideid,pgpcomp,fastact,encrypttodefault,encryptfileto,tipofday;
        QPopupMenu *droppopup,*udroppopup;
        KAboutData   *_aboutData;
        QString customDecrypt;
        KgpgWizard *wiz;
        QString filekey;

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
        void  clipDecrypt();
        void  clipEncrypt();
        void preferences();
        void  openEditor();
	void  shredDroppedFile();

private slots:
	void  slotWizardClose();
        void  startWizard();
        void  slotSaveOptionsPath();
        void  slotGenKey();
        void importSignature(QString ID);
        void slotSetClip(QString newtxt);
        void killDisplayClip();
        void encryptClipboard(QString &selec,QString encryptOptions);
        void help();
        void about();
        void firstRun();
        void readOptions();
        void  droppedfile (KURL::List);
        void  droppedtext (QString inputText) ;
        void  openKeyServer();
        void slotKeyServerClosed();

protected:
        virtual void dragEnterEvent(QDragEnterEvent *);
        virtual void  dropEvent (QDropEvent*);

protected slots:
        void showPopupMenu( QPopupMenu * );
};

class kgpgapplet : public KSystemTray//KUniqueApplication
{
        Q_OBJECT

public:
        kgpgapplet( QWidget *parent = 0, const char *name = 0);
        /** destructor */
        ~kgpgapplet();
        MyView *w;

private:
        KSystemTray *kgpgapp;
private slots:
        void slotencryptclip();
        void slotdecryptclip();
        void sloteditor();
        void slotOptions();
};

class KCmdLineArgs;

class KgpgAppletApp : public KUniqueApplication
{
        Q_OBJECT
        friend class kgpgapplet;
public:
        KgpgAppletApp();
        ~KgpgAppletApp();
        int newInstance ();
        KURL::List urlList;

protected:
        KCmdLineArgs *args;
private:
        kgpgapplet *kgpg_applet;
        class listKeys *s_keyManager;

private slots:
        void slotHandleQuit();
};



#endif
