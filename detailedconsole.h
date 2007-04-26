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

class KgpgDetailedConsole : public KDialog
{
public:
    KgpgDetailedConsole(QWidget *parent = 0, const QString &boxLabel = QString(), const QString &errormessage = QString());
};

class KgpgDetailedInfo : public KDialog
{
public:
    KgpgDetailedInfo(QWidget *parent = 0, const QString &boxLabel = QString(), const QString &errormessage = QString(), const QStringList &keysList = QStringList());
};

#endif // KGPGDETAILEDCONSOLE_H
