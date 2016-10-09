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

#include <KAboutData>
#include <KCrash>
#include <KDBusService>
#include <Kdelibs4ConfigMigrator>
#include <KLocalizedString>

int main(int argc, char *argv[])
{
    KGpgApp app(argc, argv);

    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    KCrash::initialize();

    Kdelibs4ConfigMigrator migrate(QLatin1Literal("kgpg"));
    migrate.setConfigFiles({ QLatin1Literal("kgpgrc") });
    migrate.setUiFiles({ QStringLiteral("keysmanager.rc"), QStringLiteral("kgpgeditor.rc") });
    migrate.migrate();

    KLocalizedString::setApplicationDomain("kgpg");

    KAboutData about (
        QLatin1String("kgpg"),
        xi18nc("@title", "<application>KGpg</application>"),
        QLatin1String(KGPG_VERSION),
        xi18nc("@title", "KGpg - simple gui for GnuPG"),
        KAboutLicense::GPL,
        xi18nc("@info:credit", "&copy; 2003-2016, The KGpg Developers"));

    about.addAuthor(xi18nc("@info:credit", "Rolf Eike Beer"), i18nc("@info:credit", "Maintainer"), "kde@opensource.sf-tec.de");
    about.addAuthor(xi18nc("@info:credit", "Jean-Baptiste Mardelle"), i18nc("@info:credit", "Author and former maintainer"), "bj@altern.org");
    about.addAuthor(xi18nc("@info:credit", "Jimmy Gilles"), QString(), "jimmygilles@gmail.com");
    about.addAuthor(xi18nc("@info:credit", "Andrius Štikonas"), i18nc("@info:credit", "KF5 port"), "andrius@stikonas.eu");

    about.setHomepage(QLatin1String("http://utils.kde.org/projects/kgpg"));

    about.setOrganizationDomain(QByteArray("kde.org"));
    about.setProductName(QByteArray("kgpg"));

    KAboutData::setApplicationData(about);

    QCommandLineParser parser;
    parser.setApplicationDescription(about.shortDescription());
    parser.addVersionOption();
    parser.addHelpOption();
    about.setupCommandLine(&parser);
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("e"), i18n("Encrypt file")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("k"), i18n("Open key manager")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("d"), i18n("Open editor")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("s"), i18n("Show encrypted file")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("S"), i18n("Sign File")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("V"), i18n("Verify signature")));
    parser.addPositionalArgument(QLatin1String("[File]"), i18n("File to open"));

    parser.process(app);
    about.processCommandLine(&parser);

    app.setQuitOnLastWindowClosed(false);
    KDBusService service(KDBusService::Unique);

    app.newInstance(parser);

    return app.exec();
}
