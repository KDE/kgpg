/***************************************************************************
                          kgpgeditor.h  -  description
                             -------------------
    begin                : Mon Jul 8 2002
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

#ifndef KGPGEDITOR_H
#define KGPGEDITOR_H

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
#include <qregexp.h>
#include <qwidgetlist.h>

#include <kurl.h>
#include <kpopupmenu.h>
#include <kiconloader.h>
#include <khelpmenu.h>
#include <ksystemtray.h>
#include <ktip.h>
#include <ksimpleconfig.h>
#include <kfiledialog.h>
#include <kuniqueapplication.h>
#include <kdeversion.h>
#include <kshred.h>

#if (KDE_VERSION >= 310)
#include <kpassivepopup.h>
#endif

#include "kgpginterface.h"
#include "kgpgview.h"
#include "popupname.h"
#include "kgpgoptions.h"
#include "kgpglibrary.h"

class KAction;

class KgpgApp : public KMainWindow
{
        Q_OBJECT

        friend class KgpgView;

public:
        /** construtor of KgpgApp, calls all init functions to create the application.
         */
        KgpgApp(QWidget *parent=0, const char *name=0,WFlags f = 0);
        ~KgpgApp();
        /** opens a file specified by commandline option
         */
        void openDocumentFile(const KURL& url=0);
        void openEncryptedDocumentFile(const KURL& url=0);
        /** returns a pointer to the current document connected to the KTMainWindow instance and is used by
         * the View class to access the document object's methods
         */
        KURL Docname;
        int version;
        bool ascii,untrusted,hideid,pgpcomp,encryptfileto,tipofday;
        QString messages,filekey;
        QPixmap fileEnc,fileDec;
        KgpgView *view;


protected:
        void readOptions(bool doresize=true);
        void saveOptions();
        void initActions();
        void initView();
        void closeEvent( QCloseEvent * e );

private slots:
        //void slotOptions();
        void slotFileQuit();
        void slotFileNew();

        void slotFilePreEnc();
        void slotFilePreDec();
        void slotFileOpen();
        void slotFileSave();
        void slotFileSaveAs();
        void slotFilePrint();
        void slotEditCut();
        void slotEditCopy();
        void slotEditPaste();
        void slotCheckMd5();

        void slotPreSignFile();
        void slotSignFile(KURL url);
        void slotVerifyFile(KURL url);
        void slotPreVerifyFile();
        void importSignatureKey(QString ID);

        void slotundo();
        void slotredo();

private:

#if (KDE_VERSION >= 310)
        KPassivePopup *pop;
#else
        QDialog *clippop;
#endif

        QString customDecrypt;
        //QDialog *clippop;

        KURL urlselected;
        /** the configuration object of the application */
        KConfig *config;

        KAction* fileSave;
        KAction* editUndo;
        KAction *editRedo;

        //  KToggleAction* viewEditorFirst;
};
#endif

