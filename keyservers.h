/***************************************************************************
                          keyserver.h  -  description
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

#include <qlayout.h>
#include <qlabel.h>
#include <qdialog.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qpushbutton.h>
#include <qbuttongroup.h>
#include <qhbuttongroup.h>
#include <qvbuttongroup.h>

#include <kdialogbase.h>
#include <klocale.h>
#include <kconfig.h>
#include <klineedit.h>
#include <kprocess.h>
#include <kprocio.h>
#include <kurl.h>
#include "keyserver.h"

class keyServer : public Keyserver
{
  Q_OBJECT
  public:
 keyServer(QWidget *parent=0, const char *name=0);
 ~keyServer();
 
 public slots:
 void abortImport();
 void slotAddServer();
 void slotRemoveServer();
 void slotOk();
void syncCombobox();
void slotImport();
void slotimportresult(KProcess*);
void slotimportread(KProcIO *p); 
void slotprocread(KProcIO *p);
void slotExport();
void slotEdit(QListViewItem *);
void slotEditServer();
 
   private:
  KConfig *config;
  QString readmessage;
  KProcIO *importproc;
  QDialog *importpop;
};



#endif
