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
class KProcess;
class QString;

/**
 * This class is the interface for gpg.
 */
namespace KgpgInterface {
    /**
     * @brief get all groups from GnuPG config file
     * @return list of groups names and their keys
     *
     * The strings are themself space separated list. The first entry is the
     * group name, the others are the keys inside
     */
    QStringList readGroups();
    /**
     * @brief write a group entry with the given keys
     * @param name name of the group
     * @param values key ids to add to group
     * @param configfile the name of the GnuPG config file
     *
     * If a group with the given name already exists it is replaced.
     */
    void setGpgGroupSetting(const QString &name, const QStringList &values, const QString &configfile);
    /**
     * @brief rename a group entry
     * @param oldName name of the group
     * @param newName new group name to set
     * @param configfile the name of the GnuPG config file
     * @return true if the group was renamed
     */
    bool renameGroup(const QString &oldName, const QString &newName, const QString &configfile);
    /**
     * @brief remove a group entry
     * @param name name of the group
     * @param configfile GnuPG config file to use
     */
    void delGpgGroup(const QString &name, const QString &configfile);

    QString getGpgSetting(const QString &name, const QString &configfile);
    void setGpgSetting(const QString &name, const QString &value, const QString &url);

    bool getGpgBoolSetting(const QString &name, const QString &configfile);
    void setGpgBoolSetting(const QString &name, const bool enable, const QString &url);

    /**
     * @brief ask the user for a passphrase and send it to the given gpg process
     * @param text text is the message that must be displayed in the MessageBox
     * @param process GnuPG process
     * @param isnew if the password is a \e new password that must be confirmed. Default is true
     * @param widget parent widget of this dialog or NULL
     * @return 0 if there is no error
     * @return 1 if there is an error
     */
    int sendPassphrase(const QString &text, KProcess *process, QWidget *widget = NULL);

    KgpgCore::KgpgKeyList readPublicKeys(const QStringList &ids = QStringList());
    KgpgCore::KgpgKey readSignatures(KGpgKeyNode *node);
    KgpgCore::KgpgKeyList readSecretKeys(const QStringList &ids = QStringList());
};

#endif // KGPGINTERFACE_H
