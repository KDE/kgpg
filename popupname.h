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
 #ifndef POPUPNAME_H
#define POPUPNAME_H

#include <qlayout.h>
#include <qlabel.h>
#include <qdialog.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qhbuttongroup.h>
#include <qvbuttongroup.h>

#include <kdialogbase.h>
#include <klineedit.h>
#include <kurl.h>


class popupName : public KDialogBase
{
  Q_OBJECT
  public:

  popupName(QWidget *parent=0, const char *name=0,KURL oldnam=NULL,bool email=false);
  
  QRadioButton *choix0,*choix1,*choix2;
//  QVButtonGroup *vgroup; 
  QHButtonGroup *hgroup; 
  KLineEdit *lineedit;
  KURL path;
  
      QButtonGroup* bGroupSources;
    QPushButton* buttonToolbar;
    
protected:
    QGridLayout* bGroupSourcesLayout;  
  
  private:
protected slots:
//virtual void slotOk();
private slots:

public slots:
void slotchooseurl();
void slotenable(bool);
bool getfmode();
bool getmailmode();
QString getfname();

signals:

};

#endif
