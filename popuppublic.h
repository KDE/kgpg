/***************************************************************************
                          popuppublic.h  -  description
                             -------------------
    begin                : Sat Jun 29 2002
    copyright            : (C) 2002 by
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
#ifndef POPUPPUBLIC_H
#define POPUPPUBLIC_H

#include <qpushbutton.h>
#include <qdialog.h>
#include <qwidget.h>
#include <qcheckbox.h>

#include <klistview.h>
#include <kbuttonbox.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qstring.h>

#include <kprocess.h>
#include <kprocio.h>
#include <kglobal.h>
#include <kiconloader.h>

class popupPublic : public QDialog
  {
    Q_OBJECT
public:

    popupPublic(QWidget *parent=0, const char *name=0,QString sfile="",bool filemode=false);
    KListView *keysList;
    QCheckBox *checkbox1,*checkbox2,*checkbox3,*checkbox4;
    bool fmode,encryptToDefault,trusted;

    QPixmap keyPair,keySingle,dkeyPair,dkeySingle;
    QString seclist,defaultKey,defaultName;

private:
    QPushButton *bouton1,*bouton2;
    KConfig *config;
	bool displayMailFirst;


private slots:
QString extractKeyName(QString fullName);
void annule();
void crypte();
void precrypte();
void slotprocread(KProcIO *);
void slotpreselect();
void refreshkeys();
void refresh(bool state);
void isSymetric(bool state);
void sort();
void enable();

signals:
    void selectedKey(QString &,bool,bool,bool,bool);

  };

#endif
