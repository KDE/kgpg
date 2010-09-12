/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2007,2008,2009,2010 Rolf Eike Beer <kde@opensource.sf-tec.de>
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

#include <QPixmap>

#include <kgpgkey.h>

class KGpgKeyNode;
class KGpgSignableNode;
class KProcess;
class GPGProc;

/**
 * This class is the interface for gpg.
 */
class KgpgInterface : public QObject
{
    Q_OBJECT

public:
    /**
     * Initialize the class
     */
    KgpgInterface();
    ~KgpgInterface();

    /**
     * @brief parse GnuPG version string and return version as number
     * @param vstr version string
     * @return -1 if vstr is empty, -2 on parse error, parsed number on success
     *
     * The version string must be in format A.B.C with A, B, and C numbers. The
     * returned number is A * 65536 + B * 256 + C.
     */
    static int gpgVersion(const QString &vstr);
    /**
     * @brief get the GnuPG version string of the given binary
     * @param binary name or path to GnuPG binary
     * @return version string or empty string on error
     *
     * This starts a GnuPG process and asks the binary for version information.
     * The returned string is the version information without any leading text.
     */
    static QString gpgVersionString(const QString &binary);
    /**
     * @brief find users GnuPG directory
     * @param binary name or path to GnuPG binary
     * @return path to directory
     *
     * Use this function to find out where GnuPG would store it's configuration
     * and data files. The returned path always ends with a '/'.
     */
    static QString getGpgHome(const QString &binary);

    /**
     * @brief get all group names from a GnuPG config file
     * @param configfile names of groups in this file
     * @return list of groups names
     */
    static QStringList getGpgGroupNames(const QString &configfile);
    /**
     * @brief read the key ids for the given group name
     * @param name name of the group
     * @param configfile the GnuPG config file to use
     * @return list of key ids in this group
     */
    static QStringList getGpgGroupSetting(const QString &name, const QString &configfile);
    /**
     * @brief write a group entry with the given keys
     * @param name name of the group
     * @param values key ids to add to group
     * @param configfile the name of the GnuPG config file
     *
     * If a group with the given name already exists it is replaced.
     */
    static void setGpgGroupSetting(const QString &name, const QStringList &values, const QString &configfile);
    /**
     * @brief rename a group entry
     * @param oldName name of the group
     * @param newName new group name to set
     * @param configfile the name of the GnuPG config file
     * @return true if the group was renamed
     */
    static bool renameGroup(const QString &oldName, const QString &newName, const QString &configfile);
    /**
     * @brief remove a group entry
     * @param name name of the group
     * @param configfile GnuPG config file to use
     */
    static void delGpgGroup(const QString &name, const QString &configfile);

    static QString getGpgSetting(const QString &name, const QString &configfile);
    static void setGpgSetting(const QString &name, const QString &value, const QString &url);

    static bool getGpgBoolSetting(const QString &name, const QString &configfile);
    static void setGpgBoolSetting(const QString &name, const bool enable, const QString &url);

    /**
     * @brief ask the user for a passphrase and send it to the given gpg process
     * @param text text is the message that must be displayed in the MessageBox
     * @param process GnuPG process
     * @param isnew if the password is a \e new password that must be confirmed. Default is true
     * @return 0 if there is no error
     * @return 1 if there is an error
     */
    static int sendPassphrase(const QString &text, KProcess *process, const bool isnew = true);

private:
    static QString getGpgProcessHome(const QString &binary);

    /************** extract public keys **************/
signals:
    void readPublicKeysFinished(KgpgCore::KgpgKeyList);

public:
    KgpgCore::KgpgKeyList readPublicKeys(const bool block = false, const QStringList &ids = QStringList());
    KgpgCore::KgpgKey readSignatures(KGpgKeyNode *node);
    KgpgCore::KgpgKeyList readPublicKeys(const bool block, const QString &ids)
    {
        return readPublicKeys(block, QStringList(ids));
    }

private slots:
    void readPublicKeysProcess(GPGProc *p = NULL);
    void readPublicKeysFin(GPGProc *p = NULL, const bool block = false);

private:
	unsigned int m_numberid;
	KgpgCore::KgpgKey m_publickey;
	KgpgCore::KgpgKeyList m_publiclistkeys;

    /*************************************************/


    /************** extract secret keys **************/
public slots:
    KgpgCore::KgpgKeyList readSecretKeys(const QStringList &ids = QStringList());

private slots:
    void readSecretKeysProcess(GPGProc *p);

private:
    bool m_secretactivate;
    KgpgCore::KgpgKey m_secretkey;
    KgpgCore::KgpgKeyList m_secretlistkeys;

    /*************************************************/


    /************** load a photo in a QPixmap **************/
signals:
    void loadPhotoFinished(QPixmap);

public slots:
    QPixmap loadPhoto(const QString &keyid, const QString &uid, const bool block = false);

private slots:
    void loadPhotoFin(int exitCode);

private:
    QPixmap m_pixmap;
    void readPixmapFromProcess(KProcess *proc);

    /*******************************************************/

private:
    // Globals private
    int m_success;
    QString log;

    /**
     * @internal structure for communication
     */
    QString output;

    KGpgKeyNode *m_readNode;	///< the node where the signature are read for
    KGpgSignableNode *m_currentSNode;	///< the current (sub)node signature are read for
};

#endif // KGPGINTERFACE_H
