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
#include <qdragobject.h>
#include <qlabel.h>
#include <qwidgetlist.h>
#include <qhbuttongroup.h>

#include <kurl.h>
#include <kpopupmenu.h>
#include <kiconloader.h>
#include <ksystemtray.h>
#include <kuniqueapplication.h>
#include <kdeversion.h>
#include <kcmdlineargs.h>
#include <kdesktopfile.h>
#include <krun.h>
#include <kpassivepopup.h>
#include <kurlrequesterdlg.h>

#include "kgpgeditor.h"
#include "kgpgoptions.h"
#include "keyservers.h"
#include "popuppublic.h"
#include "listkeys.h"
#include "kgpgwizard.h"

class QPopupMenu;


class MyView : public QLabel
{

        Q_OBJECT

public:
        MyView( QWidget *parent = 0, const char *name = 0);
        ~MyView();

        bool	 showeclip,showdclip,showomanager,showoeditor,showserver,autostart;
        KURL droppedUrl;
        KURL::List droppedUrls;
        KConfig *ksConfig;
        KTempFile *kgpgfoldertmp;
private:
        bool ascii,untrusted,hideid,pgpcomp,fastact,encrypttodefault,encryptfileto,tipofday,pgpExtension;
        QPopupMenu *droppopup,*udroppopup;
        KAboutData   *_aboutData;
        QStringList customDecrypt;
        KgpgWizard *wiz;
        class keyServer *m_keyServer;
        KPassivePopup *pop;
        KTempFile *kgpgFolderExtract;
	int compressionScheme;
	popupPublic *dialogue;

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
        void encryptDroppedFolder();
        void startFolderEncode(QStringList selec,QStringList encryptOptions,bool ,bool symetric);
        void  slotFolderFinished(KURL);
        void  slotFolderFinishedError(QString errmsge);

private slots:

        void readAgain1();
        void  slotWizardClose();
        void  startWizard();
	void  slotWizardChange();
        void  slotSaveOptionsPath();
        void  slotGenKey();
        void importSignature(QString ID);
        void slotSetClip(QString newtxt);
        void encryptClipboard(QStringList selec,QStringList encryptOptions);
        void help();
        void about();
        void firstRun();
        void readOptions();
        void  droppedfile (KURL::List);
        void  droppedtext (QString inputText) ;
        void  openKeyServer();
        void slotKeyServerClosed();
        void  unArchive();
        void slotSetCompression(int cp);

protected:
        virtual void dragEnterEvent(QDragEnterEvent *);
        virtual void  dropEvent (QDropEvent*);

protected slots:


signals:
        void readAgain2();
	void createNewKey();
	void updateDefault(QString);
	void importedKeys(QStringList);
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
        bool running;

protected:
        KCmdLineArgs *args;
private:
        kgpgapplet *kgpg_applet;
        class listKeys *s_keyManager;

private slots:
        void slotHandleQuit();
	void wizardOver(QString defaultKeyId);
};



#endif
