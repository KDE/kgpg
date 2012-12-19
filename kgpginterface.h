/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2007,2008,2009,2010,2011,2012
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

#ifndef KGPGINTERFACE_H
#define KGPGINTERFACE_H

#include "core/kgpgkey.h"
#include <QStringList>

class KGpgKeyNode;
class QString;

/**
 * GnuPG interface functions
 */
namespace KgpgInterface {
    QString getGpgSetting(const QString &name, const QString &configfile);
    void setGpgSetting(const QString &name, const QString &value, const QString &url);

    bool getGpgBoolSetting(const QString &name, const QString &configfile);
    void setGpgBoolSetting(const QString &name, const bool enable, const QString &url);

    KgpgCore::KgpgKeyList readPublicKeys(const QStringList &ids = QStringList());
    void readSignatures(KGpgKeyNode *node);
    KgpgCore::KgpgKeyList readSecretKeys(const QStringList &ids = QStringList());
};

#endif // KGPGINTERFACE_H
