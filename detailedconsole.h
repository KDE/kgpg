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

#ifndef KGPGDETAILEDCONSOLE_H
#define KGPGDETAILEDCONSOLE_H

#include <kdialog.h>
#include <kdialogbase.h>

class KgpgDetailedConsole : public KDialog
{
public:
    KgpgDetailedConsole(QWidget *parent = 0, const QString &boxLabel = QString::null, const QString &errormessage = QString::null);
};

class KgpgDetailedInfo : public KDialogBase
{
public:
    KgpgDetailedInfo(QWidget *parent = 0, const char *name = 0,const QString &boxLabel = QString::null, const QString &errormessage = QString::null, const QStringList &keysList = QStringList());
};

#endif // KGPGDETAILEDCONSOLE_H
