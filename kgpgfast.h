/***************************************************************************
                          kgpgfast.h  -  description
                             -------------------
    begin                : Thu Jul 4 2002
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

#ifndef KGPGFAST_H
#define KGPGFAST_H

#include <qfile.h>
#include <qstring.h>

#include <kurl.h>
#include <kdialogbase.h>
class KLineEdit;
class QPushButton;


class KgpgOverwrite : public KDialogBase
{
        Q_OBJECT

public:
        KgpgOverwrite( QWidget *parent = 0, const char *name = 0,KURL ofile=0);

        QPushButton *bouton1,*bouton2,*bouton3;
        KLineEdit *lineedit;
        QString direc;
private slots:
        void enablerename();
        void annule();
        void slotok();
        void slotcheck();
public slots:
        QString getfname();

};


#endif
