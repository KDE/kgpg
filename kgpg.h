/***************************************************************************
                          kgpg.h  -  description
                             -------------------
    begin                : Mon Nov 18 2002
    copyright          : (C) 2002 by Jean-Baptiste Mardelle
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

#ifndef KGPGAPPLET_H
#define KGPGAPPLET_H

#include <KUniqueApplication>
#include <KUrl>

class KCmdLineArgs;
class KeysManager;
class KGpgExternalActions;
class QString;

class KGpgApp : public KUniqueApplication
{
    Q_OBJECT

public:
    KGpgApp();
    ~KGpgApp();

    int newInstance ();
    bool running;
    KUrl::List urlList;

protected:
    KCmdLineArgs *args;

private:
    KGpgExternalActions *w;
    KeysManager *s_keyManager;

private slots:
    void slotHandleQuit();
    void assistantOver(const QString &defaultKeyId);
};

#endif // KGPGAPPLET_H
