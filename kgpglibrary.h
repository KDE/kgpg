/***************************************************************************
                          kgpglibrary.h  -  description
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


#ifndef KGPGLIBRARY_H
#define KGPGLIBRARY_H

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
#include <kaction.h>
#include <kiconloader.h>
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

#include "popuppublic.h"
#include "popupname.h"
#include "kgpginterface.h"
#include "kgpgshredwidget.h"

#if (KDE_VERSION >= 310)
#include <kpassivepopup.h>
#endif

class KgpgLibrary : public QObject
{

        Q_OBJECT

public:
        /**
         * Initialize the class
         */
        KgpgLibrary();
        ~KgpgLibrary();

        KURL::List urlselecteds;

public slots:
        //void slotFileEnc(KURL url=0,QString opts="",QString defaultKey="");
        void slotFileEnc(KURL::List urls=KURL(""),QString opts="",QString defaultKey="");
        void slotFileDec(KURL srcUrl=0,KURL destUrl=0,QString customDecryptOption="");

private slots:
        void fastencode(KURL &fileToCrypt,QString &selec,QString encryptOptions,bool shred,bool symetric);
        void startencode(QString &selec,QString encryptOptions,bool shred,bool symetric);

        void processenc(KURL);
        void processdecover();
        void processdecerror(QString mssge);
        void processencerror(QString mssge);
        void processpopup();
        void processpopup2();
        void shredprocessenc(KURL);

private:
        QString customDecrypt,tempFile;
        KURL urlselected;
        bool popIsDisplayed;

#if (KDE_VERSION >= 310)
        KPassivePopup *pop;
#else
        QDialog *clippop;
#endif


};
#endif
