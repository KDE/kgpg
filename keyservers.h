/***************************************************************************
                          keyservers.h  -  description
                             -------------------
    begin                : Tue Nov 26 2002
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

#ifndef KEYSERVERS_H
#define KEYSERVERS_H

#include <kdialogbase.h>
#include "keyserver.h"

class KListViewItem;
class KSimpleConfig;
class KProcIO;
class KProcess;

class searchRes;

class keyServer : public KDialogBase
{
        Q_OBJECT
public:
        keyServer(QWidget *parent=0, const char *name=0,bool modal=false,bool autoClose=false);
        ~keyServer();
	keyServerWidget *page;

public slots:
        void abortImport();
	void abortExport();
        void abortSearch();
        void transferKeyID();
        void slotsearchread(KProcIO *p);
        void slotOk();
        void syncCombobox();
        void slotImport();
        void slotSearch();
        void slotimportresult(KProcess*);
	void slotexportresult(KProcess*);
        void slotimportread(KProcIO *p);
        void slotprocread(KProcIO *p);
        void slotPreExport();
	void slotExport(QStringList keyIds);
        void preimport();
        void slotsearchresult(KProcess *);
        void slotEnableProxyI(bool on);
        void slotEnableProxyE(bool on);
        void handleQuit();
    	void slotTextChanged( const QString &text);

private:

        QDialog *importpop;
        KSimpleConfig *config;
        uint keyNumbers;
        QString readmessage;
        KProcIO *importproc,*exportproc;
	KProcIO *searchproc;
        searchRes *listpop;
        int count;
        bool cycle,autoCloseWindow;
        KListViewItem *kitem;
	KDialogBase *dialogServer;
	
signals:
	void importFinished(QString);
};

#endif // KEYSERVERS_H

