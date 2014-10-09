/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2007,2008,2009,2010,2011,2012,2013,2014
 *               Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kgpg.h"

#include <KAboutData>
#include <KCmdLineArgs>
#include <KLocale>
#include <KUniqueApplication>

static const char description[] =
        I18N_NOOP("KGpg - simple gui for gpg\n\nKGpg was designed to make gpg very easy to use.\nI tried to make it as secure as possible.\nHope you enjoy it.");

static const char version[] = "2.13.1";

int main(int argc, char *argv[])
{
    KAboutData about("kgpg", 0, ki18n("KGpg"), version, ki18n(description), KAboutData::License_GPL, ki18n("(C) 2003 Jean-Baptiste Mardelle"), KLocalizedString(), "http://utils.kde.org/projects/kgpg");
    about.addAuthor(ki18n("Jean-Baptiste Mardelle"), ki18n("Author and former maintainer"), "bj@altern.org");
    about.addAuthor(ki18n("Jimmy Gilles"), KLocalizedString(), "jimmygilles@gmail.com");
    about.addAuthor(ki18n("Rolf Eike Beer"), ki18n("Maintainer"), "kde@opensource.sf-tec.de");

    KCmdLineArgs::init(argc, argv, &about);

    KCmdLineOptions options;
    options.add("e", ki18n("Encrypt file"));
    options.add("k", ki18n("Open key manager"));
    options.add("d", ki18n("Open editor"));
    options.add("s", ki18n("Show encrypted file"));
    options.add("S", ki18n("Sign File"));
    options.add("V", ki18n("Verify signature"));
    options.add("+[File]", ki18n("File to open"));
    KCmdLineArgs::addCmdLineOptions(options);
    KUniqueApplication::addCmdLineOptions();

    if (!KUniqueApplication::start())
        return 0;

    KApplication *app = new KGpgApp();
    app->setQuitOnLastWindowClosed(false);
    return app->exec();
}
