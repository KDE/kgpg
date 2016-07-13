/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2007,2008,2009,2010,2011,2012,2013,2014,2015,2016
 *               Rolf Eike Beer <kde@opensource.sf-tec.de>
 * Copyright (C) 2016 Andrius Štikonas <andrius@stikonas.eu>
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

#include <QCommandLineOption>
#include <QCommandLineParser>

#include <K4AboutData>
#include <KAboutData>
#include <KDBusService>
#include <KLocalizedString>

static const char description[] =
        I18N_NOOP("KGpg - simple gui for GnuPG");

int main(int argc, char *argv[])
{
    K4AboutData about("kgpg", 0, ki18n("KGpg"), KGPG_VERSION, ki18n(description), K4AboutData::License_GPL, ki18n("&copy; 2003-2016, The KGpg Developers"));
    KGpgApp *app = new KGpgApp(argc, argv);

    KLocalizedString::setApplicationDomain("kgpg");

    about.addAuthor(ki18n("Rolf Eike Beer"), ki18n("Maintainer"), "kde@opensource.sf-tec.de");
    about.addAuthor(ki18n("Jean-Baptiste Mardelle"), ki18n("Author and former maintainer"), "bj@altern.org");
    about.addAuthor(ki18n("Jimmy Gilles"), KLocalizedString(), "jimmygilles@gmail.com");

    about.setHomepage("http://utils.kde.org/projects/kgpg");

    KAboutData::setApplicationData(about);

    QCommandLineParser parser;
    parser.setApplicationDescription(about.shortDescription());
    parser.addVersionOption();
    parser.addHelpOption();
    KAboutData(about).setupCommandLine(&parser);
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("e"), i18n("Encrypt file")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("k"), i18n("Open key manager")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("d"), i18n("Open editor")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("s"), i18n("Show encrypted file")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("S"), i18n("Sign File")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("V"), i18n("Verify signature")));
    parser.addPositionalArgument(QLatin1String("[File]"), i18n("File to open"));

    parser.process(*app);
    KAboutData(about).processCommandLine(&parser);

    app->setQuitOnLastWindowClosed(false);
    KDBusService service(KDBusService::Unique);

    app->newInstance(parser);

    return app->exec();
}
