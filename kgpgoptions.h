/***************************************************************************
                          kgpgoptions.h  -  description
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
 #ifndef KGPGOPTIONS_H
#define KGPGOPTIONS_H

#include <qlayout.h>
#include <qlabel.h>
#include <qdialog.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qhbuttongroup.h>
#include <qvbuttongroup.h>

#include <kdialogbase.h>
#include <kcombobox.h>

#include <kiconloader.h>
#include <kmessagebox.h>
#include <kfiledialog.h>
#include <kmenubar.h>
#include <ktoolbar.h>
//#include <kstatusbar.h>

#include <kconfig.h>
#include <klocale.h>
#include "kgpgoption.h"



class kgpgOptions : public KOptions
{
  Q_OBJECT
  public:
 kgpgOptions(QWidget *parent=0, const char *name=0);
  ~kgpgOptions();
  QStringList names,ids;
  
  private:
  KConfig *config;
private slots:
void listkey();
QString namecode(QString kid);
QString idcode(QString kname);
void slotOk();
void slotInstallDecrypt(QString mimetype);
void slotInstallSign(QString mimetype);

void slotRemoveMenu(QString menu);
public slots:
void checkMimes();
signals:

};

#endif
