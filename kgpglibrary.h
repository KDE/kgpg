/***************************************************************************
                          kgpglibrary.h  -  description
                             -------------------
    begin                : Mon Jul 8 2002
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


#ifndef KGPGLIBRARY_H
#define KGPGLIBRARY_H

#include <qevent.h>
#include <qwidget.h>
#include <qstring.h>
#include <qwidgetlist.h>
#include <qapplication.h>

#include <kurl.h>
#include <kdeversion.h>
#include <kpassivepopup.h>
#include <kprogress.h>
#include <kio/job.h> 

#include "popuppublic.h"
#include "kgpginterface.h"


class KgpgLibrary : public QObject
{

        Q_OBJECT

public:
        /**
         * Initialize the class
         */
        KgpgLibrary(QWidget *parent=0,bool pgpExtension=false);
        ~KgpgLibrary();

        KURL::List urlselecteds;

public slots:
        void slotFileEnc(KURL::List urls=KURL(""),QStringList opts=QString::null,QString defaultKey="");
        void slotFileDec(KURL srcUrl,KURL destUrl,QStringList customDecryptOption=QStringList());
	void shredprocessenc(KURL::List filesToShred);

private slots:
	void startencode(QStringList encryptKeys,QStringList encryptOptions,bool shred,bool symetric);
        void fastencode(KURL &fileToCrypt,QStringList selec,QStringList encryptOptions,bool shred,bool symetric);
//        void startencode(QString &selec,QString encryptOptions,bool shred,bool symetric);
	void slotShredResult( KIO::Job * job );
	void shredpreprocessenc(KURL fileToShred);
        void processenc(KURL);
        void processdecover();
        void processdecerror(QString mssge);
        void processencerror(QString mssge);
        void processpopup(QString fileName);
        void processpopup2(QString fileName);

private:
        QString customDecrypt,tempFile,extension;
        KURL urlselected;
        KPassivePopup *pop;
	KProgress *shredProgressBar;
	bool popIsActive;
	int filesToEncode;
	QWidget *panel;
	QStringList _encryptKeys;
	QStringList _encryptOptions;
	bool _shred;
	bool _symetric;

signals:
        void decryptionOver();
	void importOver(QStringList);
	void systemMessage(QString,bool reset=false);


};
#endif
