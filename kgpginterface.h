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

#ifndef __KGPGINTERFACE_H__
#define __KGPGINTERFACE_H__

#include <QStringList>
#include <QDateTime>
#include <QString>

#include <kdialogbase.h>
#include <kurl.h>
#include <kgpgkey.h>

class QWidget;
class QLabel;

class KProcess;
class KProcIO;
class KLed;

/**
 * Encrypt a file using gpg.
 */
class KgpgInterface : public QObject
{
    Q_OBJECT

public:
    /**
     * Initialize the class
     */
    KgpgInterface();

    static int getGpgVersion();

    static QString checkForUtf8(QString txt);
    static QString checkForUtf8bis(QString txt);

    static QStringList getGpgGroupNames(const QString &configfile);
    static QStringList getGpgGroupSetting(const QString &name, const QString &configfile);
    static void setGpgGroupSetting(const QString &name, const QStringList &values, const QString &configfile);
    static void delGpgGroup(const QString &name, const QString &configfile);

    static QString getGpgSetting(QString name, const QString &configfile);
    static void setGpgSetting(const QString &name, const QString &value, const QString &url);

    static bool getGpgBoolSetting(const QString &name, const QString &configfile);
    static void setGpgBoolSetting(const QString &name, const bool &enable, const QString &url);

    static int checkUID(const QString &keyid);


/************** function to send a passphrase to gpg **************/
private:
    /**
     * This is a secure method to send the passphrase to gpg.
     * It will shred (1 pass) the memory before deleting the object
     * that contains the passphrase.
     * @return 1 if there is an error
     * @return 0 if there is no error
     */
    int sendPassphrase(const QString &text, KProcess *process, const bool isnew = true);
/******************************************************************/


/************** extract public keys **************/
signals:
    void startedReadPublicKeys();
    void finishedReadPublicKeys(KgpgListKeys, KgpgInterface*);

public slots:
    KgpgListKeys readPublicKeys(const bool &block = false, const QStringList &ids = QStringList());

private slots:
    void readPublicKeyLines(KProcIO *p);
    void readPublicKeysExited(KProcess *p, const bool &block = false);

private:
    KgpgKeyPtr m_publickey;
    KgpgListKeys m_publiclistkeys;

/*************************************************/


/************** extract secret keys **************/
signals:
    void startedReadSecretKeys();
    void finishedReadSecretKeys(KgpgListKeys, KgpgInterface*);

public slots:
    KgpgListKeys readSecretKeys(const bool &block = false, const QStringList &ids = QStringList());

private slots:
    void readSecretKeyLines(KProcIO *p);
    void readSecretKeysExited(KProcess *p, const bool &block = false);

private:
    KgpgKeyPtr m_secretkey;
    KgpgListKeys m_secretlistkeys;

/*************************************************/


/************** encrypt a text **************/
signals:
    /**
     *  emitted when a txt encryption starts.
     */
    void txtEncryptionStarted();

    /**
     *  emitted when a txt encryption finished. returns encrypted text
     */
    void txtEncryptionFinished(QString, KgpgInterface*);

public slots:
    /**
     * Encrypt text function
     * @param text text to be encrypted.
     * @param userIDs the recipients key id's.
     * @param Options a list of string with the wanted gpg options. ex: "--armor"
     */
    void encryptText(const QString &text, const QStringList &userids, const QStringList &options = QStringList());

private slots:
    void txtReadEncProcess(KProcIO *p);
    void txtEncryptFin(KProcess *p);

/********************************************/


/************** decrypt a text **************/
signals:
    /**
     *  emitted when a txt decryption starts.
     */
    void txtDecryptionStarted();

    /**
     *  emitted when a txt decryption finished. returns decrypted text
     */
    void txtDecryptionFinished(QString, KgpgInterface*);

    /**
     *  emitted when a txt decryption failed. returns log output
     */
    void txtDecryptionFailed(QString, KgpgInterface*);

public slots:
    /**
     * Decrypt text function
     * @param text QString text to be decrypted.
     * @param Options StringList with the wanted gpg options.
     */
    void decryptText(const QString &text, const QStringList &options = QStringList());

private slots:
    void txtReadDecStdErr(KProcess *, char *data, int);
    void txtReadDecStdOut(KProcess *p, char *data, int);
    void txtDecryptFin(KProcess *p);

private:
    int m_textlength;

/********************************************/


/************** sign a text **************/
signals:
    void txtSigningStarted();
    void txtSigningFinished(QString, KgpgInterface*);
    void txtSigningFailed(QString, KgpgInterface*);

public slots:
    /**
     * Sign text function
     * @param text QString text to sign.
     * @param userIDs the recipient key id's.
     * @param Options StringList with the wanted gpg options.
     */
    void signText(const QString &text, const QString &userid, const QStringList &options);

private slots:
    void txtSignProcess(KProcIO *p);
    void txtSignFin(KProcess *p);

/*****************************************/


/************** verify a text **************/
signals:
    void txtVerifyStarted();
    void txtVerifyMissingSignature(QString, KgpgInterface*);
    void txtVerifyFinished(QString, QString, KgpgInterface*);

public slots:
    /**
     * Verify text function
     * @param text QString text to be verified.
     */
    void verifyText(const QString &text);

private slots:
    void txtVerifyProcess(KProcIO *p);
    void txtVerifyFin(KProcess*);

/*******************************************/


/************** encrypt a file **************/
signals:
    /**
     *  emitted when the process starts
     */
    void processstarted(QString);

    /**
     *  emitted when an error occurred
     */
    void errorMessage(QString, KgpgInterface*);

    /**
     *  true if encryption successful, false on error.
     */
    void fileEncryptionFinished(KURL, KgpgInterface*);

public slots:
    /**
     * Encrypt file function
     * @param encryptKeys the recipients key id's.
     * @param srcUrl Kurl of the file to encrypt.
     * @param destUrl Kurl for the encrypted file.
     * @param Options String List with the wanted gpg options. ex: "--armor"
     * @param symetrical bool whether the encryption should be symmetrical.
     */
    void encryptFile(const QStringList &encryptkeys, const KURL &srcurl, const KURL &desturl, const QStringList &options = QStringList(), const bool &symetrical = false);

private slots:
    /**
     * Reads output of the current encryption process + allow overwriting of a file
     */
    void fileReadEncProcess(KProcIO *p);

    /**
     * Checks if the encrypted file was saved.
     */
    void fileEncryptFin(KProcess *p);

/********************************************/


/************** sign a key **************/
signals:
    void signKeyStarted();

    /**
     * Signature process result:
     * 0 = Unknown error
     * 1 = Bad passphrase
     * 2 = Good passphrase
     * 3 = Aborted by user
     * 4 = Already signed
     */
    void signKeyFinished(int, KgpgInterface*);

public slots:
    /**
     * Key signature function
     * @param keyid the ID of the key to be signed
     * @param signkeyid the ID of the signing key
     * @param local bool should the signature be local
     * @param checkin
     * @param terminal if the user want to sign the key manually
     */
    void signKey(const QString &keyid, const QString &signkeyid, const bool &local, const int &checking, const bool &terminal = false);

private slots:
    /**
     * Read output of the signature process
     */
    void signKeyProcess(KProcIO *p);

    /**
     * Checks output of the signature process
     */
    void signKeyFin(KProcess *p);

    /**
     * Opens the console when the user want to sign
     * a key manually.
     */
    void signKeyOpenConsole();

private:
    QString m_signkey;
    QString m_keyid;
    int m_checking;
    bool m_local;

/****************************************/


/************** change key expiration **************/
signals:
    void keyExpireStarted();

    /**
     * 0 = unknown error
     * 1 = Bad Passphrase
     * 2 = Good Passphrase
     * 3 = Aborted
     */
    void keyExpireFinished(int, KgpgInterface*);

public slots:
    void keyExpire(const QString &keyid, const QDate &date, const bool &unlimited);

private slots:
    void keyExpireProcess(KProcIO *p);
    void keyExpireFin(KProcess *p);

/***************************************************/


/************** change key password **************/
signals:
    /**
     * 0 = Unknown error
     * 1 = Bad Passphrase
     * 2 = Passphrase changed
     * 3 = Aborted
     */
    void changePassFinished(int, KgpgInterface*);

public slots:
    void changePass(const QString &keyid);

private slots:
    void changePassProcess(KProcIO *p);
    void changePassFin(KProcess *p);

/*************************************************/


/************** change key trust **************/
signals:
    void changeTrustFinished(KgpgInterface*);

public slots:
    void changeTrust(const QString &keyid, const int &keytrust);

private slots:
    void changeTrustProcess(KProcIO *p);
    void changeTrustFin(KProcess *p);

/**********************************************/














public slots:
    /**
     * Decrypt file function
     * @param srcUrl Kurl of the file to decrypt.
     * @param destUrl Kurl for the decrypted file.
     * @param Options String List with the wanted gpg options
     */
    void KgpgDecryptFile(KURL srcUrl, KURL destUrl, QStringList Options = QStringList());

    /**
     * Decrypt File to text function
     * @param srcUrl Kurl of the file to decrypt.
     * @param Options StringList with the wanted gpg options.
     */
    void KgpgDecryptFileToText(KURL srcUrl, QStringList Options);

    /**
     * Sign file function
     * @param keyID QString the signing key ID.
     * @param srcUrl Kurl of the file to sign.
     * @param Options String with the wanted gpg options. ex: "--armor"
     */
    void KgpgSignFile(QString keyID, KURL srcUrl, QStringList Options = QStringList());

    /**Verify file function
     * @param sigUrl Kurl of the signature file.
     * @param srcUrl Kurl of the file to be verified. If empty, gpg will try to find it using the signature file name (by removing the .sig extensio)
     */
    void KgpgVerifyFile(KURL sigUrl, KURL srcUrl = KURL()) ;

    /**
     * Import key function
     * @param url Kurl the url of the key file. Allows public & secret key import.
     */
    void importKeyURL(KURL url);

    /**
     * Import key function
     * @param keystr QString containing th key. Allows public & secret key import.
     */
    void importKey(QString keystr);

    /**
     * Key signature deletion function
     * @param keyID QString the ID of the key
     * @param signKeyID QString the ID of the signature key
     */
    void KgpgDelSignature(QString keyID, QString signKeyID);

    /**
     * Extract list of photographic user id's
     * @param keyID the recipients key id's.
     */
    void KgpgGetPhotoList(QString keyID);

    QString getKey(QStringList IDs, bool attributes);


    void KgpgRevokeKey(QString keyID, QString revokeUrl, int reason, QString description);
    void revokeover(KProcess *);
    void revokeprocess(KProcIO *p);
    void KgpgDeletePhoto(QString keyID, QString uid);
    void KgpgAddPhoto(QString keyID, QString imagePath);

    void KgpgAddUid(QString keyID, QString name, QString email, QString comment);

private slots:
    /**
     * Checks if the decrypted file was saved.
     */
    void decryptfin(KProcess *);

    /**
     * Checks if the signing was successful.
     */
    void signfin(KProcess *p);

    /**
     * Reads output of the delete signature process
     */
    void delsigprocess(KProcIO *p);

    /**
     * Checks output of the delete signature process
     */
    void delsignover(KProcess *p);

    /**
     * Checks output of the import process
     */
    void importURLover(KProcess *p);

    void importover(KProcess *);

    /**
     * Read output of the import process
     */
    void importprocess(KProcIO *p);

    /**
     * Reads output of the current process + allow overwriting of a file
     */
    void readprocess(KProcIO *p);

    /**
     * Reads output of the current signing process + allow overwriting of a file
     */
    void readsignprocess(KProcIO *p);

    /**
     * Reads output of the current decryption process + allow overwriting of a file
     */
    void readdecprocess(KProcIO *p);

    /**
     * Checks output of the verify process
     */
    void verifyfin(KProcess *p);

    void delphotoover(KProcess *);
    void delphotoprocess(KProcIO *p);
    void addphotoover(KProcess *);
    void addphotoprocess(KProcIO *p);

    void adduidover(KProcess *);
    void adduidprocess(KProcIO *p);

    void slotReadKey(KProcIO *p);
    void photoreadover(KProcess *);
    void photoreadprocess(KProcIO *p);
    bool isPhotoId(int uid);
    void updateIDs(QString txtString);

    //void txtreaddecprocess(KProcIO *p);
    //void txtdecryptfin(KProcess *);

signals:
    /**
     *  true if key signature deletion successful, false on error.
     */
    void delsigfinished(bool);

    /**
     *  emitted when user cancels process
     */
    void processaborted(bool);

    /**
     *  true if decryption successful, false on error.
     */
    void decryptionfinished();

    /**
     * emitted if bad passphrase was giver
     */
    void badpassphrase(bool);

    /**
     *  true if import successful, false on error.
     */
    void importfinished(QStringList);

    /**
     *  true if verify successful, false on error.
     */
    void verifyfinished();

    /**
     *  emmitted if signature key is missing & user want to import it from keyserver
     */
    void verifyquerykey(QString ID);

    /**
     *  true if signature successful, false on error.
     */
    void signfinished();
    void delPhotoFinished();
    void delPhotoError(QString);

    void addPhotoFinished();
    void addPhotoError(QString);
    void refreshOrphaned();

    void addUidFinished();
    void addUidError(QString);

    void revokecertificate(QString);
    void revokeurl(QString);
    void signalPhotoList(QStringList);

private:
    // Globals private
    int m_success;
    QString m_partialline;
    bool m_ispartial;
    QString message;
    QString userIDs;
    QString log;
    bool encok;                 // encrypt ok
    bool decok;                 // decrypt ok
    bool badmdc;                // bad mdc
    bool badpassword;           // bad password

    /**
     * @internal structure for communication
     */
    QString tempKeyFile;
    QString output;
    QString keyString;

    bool deleteSuccess;
    bool anonymous;
    bool decfinished;
    bool revokeSuccess;
    bool addSuccess;
    bool delSuccess;
    bool signmiss;

    QString signID;
    int expSuccess;
    int trustValue;
    int step;
    int signb;
    int sigsearch;
    int expirationDelay;
    int revokeReason;
    int photoCount;
    QString revokeDescription;
    QString certificateUrl;
    QString photoUrl;
    QStringList photoList;
    QString uidName;
    QString uidEmail;
    QString uidComment;
    KURL sourceFile;
    QString decryptUrl;

    QString gpgOutput;

    /**
     * @internal structure for the file information
     */
    KURL file;
};

class  Md5Widget : public KDialogBase
{
    Q_OBJECT

public:
    Md5Widget(QWidget *parent = 0, const char *name = 0, const KURL &url = KURL());

public slots:
    void slotApply();

private:
    QString m_mdsum;
    KLed *m_kled;
    QLabel *m_textlabel;
};

#endif // __KGPGINTERFACE_H__
