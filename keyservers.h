/***************************************************************************
                          keyservers.h  -  description
                             -------------------
    begin                : Tue Nov 26 2002
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

#ifndef KEYSERVERS_H
#define KEYSERVERS_H

#include <klistview.h>

#include "keyserver.h"
#include "searchres.h"
#include "kgpginterface.h"
#include "detailedconsole.h"


class keyServer : public KDialogBase
{
        Q_OBJECT
public:
        keyServer(QWidget *parent=0, const char *name=0,bool modal=false);
        ~keyServer();
        QDialog *importpop;
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
	void slotExport(QString keyId);
        void preimport();
        void slotsearchresult(KProcess *);
        void slotEnableProxyI(bool on);
        void slotEnableProxyE(bool on);
        void handleQuit();
    	void slotTextChanged( const QString &text);

private:

        KConfig *config;
        uint keyNumbers;
        QString readmessage;
        KProcIO *importproc,*exportproc;
        searchRes *listpop;
        int count;
        bool cycle;
        KListViewItem *kitem;
};

#endif
