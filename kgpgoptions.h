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


#include <klocale.h>



class kgpgOptions : public KDialogBase
{
  Q_OBJECT
  public:
 kgpgOptions(QWidget *parent=0, const char *name=0,bool oascii=true,bool otrusted=false,bool opgp=false,bool ofilekey=false,bool odefkey=false,
QString ofkey=0,QString odkey=0);
  //kgpgOptions(QWidget *parent=0, const char *name=0);
  QCheckBox *choix3,*choix4,*choix2,*choix1,*choix5;
  //QCheckBox *choix5;
  QHButtonGroup *bgroup; 
  KComboBox *selkey,*selkey2;
  QStringList names,ids;
  
  private:
protected slots:
//virtual void slotOk();
private slots:

public slots:
QString namecode(QString kid);
QString idcode(QString kname);
bool getascii();
bool getpgp();
bool getuntrusted();
bool defaultenc();
QString getdefkey();
bool fileenc();
QString getfilekey();
void activateselkey(int state);
void activateselkey2(int state);
void listkey();

signals:

};

#endif
