/***************************************************************************
                          detailledconsole.h  -  description
                             -------------------
    begin                : Mon Jul 8 2002
    copyright            : (C) 2002 by Jean-Baptiste Mardelle
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

#ifndef KDETAILED_H
#define KDETAILED_H

#include <kdialogbase.h>

class KDetailedConsole : public KDialogBase
{
public:

        KDetailedConsole(QWidget *parent=0, const char *name=0,const QString &boxLabel=QString::null,const QString &errormessage=QString::null);
        ~KDetailedConsole();

};


class KDetailedInfo : public KDialogBase
{
public:

        KDetailedInfo(QWidget *parent=0, const char *name=0,const QString &boxLabel=QString::null,const QString &errormessage=QString::null,QStringList keysList=QStringList());
        ~KDetailedInfo();

};

#endif // KDETAILED_H

