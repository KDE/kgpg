/***************************************************************************
                          kgpginterface.h  -  description
                             -------------------
    begin                : Sat Jun 29 2002
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

#ifndef KGPGINTERFACE_H
#define KGPGINTERFACE_H

#include <QPixmap>


#include <KUrl>

#include <kgpgkey.h>

class KTemporaryFile;
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

    static QString checkForUtf8(QString txt);
    static QString checkForUtf8bis(QString txt);

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

    static QStringList getGpgGroupNames(const QString &configfile);
    static QStringList getGpgGroupSetting(const QString &name, const QString &configfile);
    static void setGpgGroupSetting(const QString &name, const QStringList &values, const QString &configfile);
    static void delGpgGroup(const QString &name, const QString &configfile);

    static QString getGpgSetting(const QString &name, const QString &configfile);
    static void setGpgSetting(const QString &name, const QString &value, const QString &url);

    static bool getGpgBoolSetting(const QString &name, const QString &configfile);
    static void setGpgBoolSetting(const QString &name, const bool enable, const QString &url);

    /**
     * @brief ask the user for a passphrase and send it to the given gpg process
     * @param text text is the message that must be displayed in the MessageBox
     * @param process gnupg process
     * @param isnew if the password is a \e new password that must be confirmed. Default is true
     * @return 0 if there is no error
     * @return 1 if there is an error
     */
    static int sendPassphrase(const QString &text, KProcess *process, const bool isnew = true);

private:
    static QString getGpgProcessHome(const QString &binary);

/************** function update the userIDs variable **************/
    void updateIDs(QString txt);

/******************************************************************/


/************** extract public keys **************/
signals:
    void readPublicKeysFinished(KgpgCore::KgpgKeyList);

public:
    KgpgCore::KgpgKeyList readPublicKeys(const bool block = false, const QStringList &ids = QStringList(), const bool withsigs = false);
    KgpgCore::KgpgKeyList readPublicKeys(const bool block, const QString &ids, const bool withsigs = false)
	{ return readPublicKeys(block, QStringList(ids), withsigs); }

private slots:
	void readPublicKeysProcess(GPGProc *p = NULL);
	void readPublicKeysFin(GPGProc *p = NULL, const bool block = false);

private:
    int m_numberid;
    QString cycle;
    KgpgCore::KgpgKey m_publickey;
    KgpgCore::KgpgKeyList m_publiclistkeys;

/*************************************************/


/************** extract secret keys **************/
public slots:
    KgpgCore::KgpgKeyList readSecretKeys(const QStringList &ids = QStringList());
    KgpgCore::KgpgKeyList readJoinedKeys(const KgpgCore::KgpgKeyTrust trust, const QStringList &ids = QStringList());

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
    KTemporaryFile *m_kgpginfotmp;

/*******************************************************/


/********************************************************/
public slots:

    /**
     * Key signature deletion function
     * @param keyID the ID of the key
     * @param uid the user id that is signed
     * @param signKeyID the ID of the signature key
     */
    void KgpgDelSignature(const QString &keyID, const QString &uid, QString signKeyID);

    void KgpgRevokeKey(const QString &keyID, const KUrl &revokeUrl, const int reason, const QString &description);
    void revokeover();
    void revokeprocess();


private slots:
    /**
     * Reads output of the delete signature process
     */
    void delsigprocess();

    /**
     * Checks output of the delete signature process
     */
    void delsignover();

    /**
     * Finds the offset of the given signatures to a uid
     */
    void findSigns(const QString &keyID, const QStringList &ids, const QString &uid, QList<int> *res);

signals:
    /**
     *  true if key signature deletion successful, false on error.
     */
    void delsigfinished(bool);

    void revokecertificate(const QString &);
    void revokeurl(const KUrl &);

private:
    // Globals private
    int m_success;
    QString m_partialline;
    bool m_ispartial;
    QString message;
    QString userIDs;
    QString log;

    /**
     * @internal structure for communication
     */
    QString output;

    bool deleteSuccess;
    bool revokeSuccess;
    bool addSuccess;
    bool delSuccess;

    int expSuccess;
    int step;
    int signb;
    int sigsearch;
    int expirationDelay;
    int revokeReason;
    int photoCount;
    QString revokeDescription;
    KUrl certificateUrl;
    QString photoUrl;
    QString decryptUrl;

    QString gpgOutput;
};

#endif // KGPGINTERFACE_H
