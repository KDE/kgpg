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
#include <QPixmap>
#include <QString>

#include <kdialogbase.h>
#include <kurl.h>

#include <kgpgkey.h>

class QWidget;
class QLabel;

class KTempFile;
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
     * that contains the passphrase. The password is securely send
     * to gpg.
     * @param text text is the message that must be displayed in the MessageBox
     * @param process process is the process where we must send the password
     * @param isnew if the password is a \e new password that must be confirmed.
     * @return 0 if there is no error
     * @return 1 if there is an error
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
    void readPublicKeysProcess(KProcIO *p);
    void readPublicKeysFin(KProcess *p, const bool &block = false);

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
    void readSecretKeysProcess(KProcIO *p);
    void readSecretKeysFin(KProcess *p, const bool &block = false);

private:
    KgpgKeyPtr m_secretkey;
    KgpgListKeys m_secretlistkeys;

/*************************************************/


// TODO : mettre cette méthode non bloquante et proposer les deux options (bloquante ou non)
/************** get photo list **************/
signals:
    void getPhotoListFinished(QStringList, KgpgInterface*);

public slots:
    /**
     * Extract list of photographic user id's
     * @param keyid the recipients key id's.
     */
    void getPhotoList(const QString &keyid);

private slots:
    void getPhotoListProcess(KgpgListKeys listkeys, KgpgInterface*);
    bool isPhotoId(uint uid);

/********************************************/


/************** get keys as a text **************/
signals:
    void getKeysFinished(QString, KgpgInterface*);

public slots:
    QString getKeys(const bool &block = false, const bool &attributes = true, const QStringList &ids = QStringList());

private slots:
    void getKeysProcess(KProcIO *p);
    void getKeysFin(KProcess *p);

private:
    QString m_keystring;

/************************************************/


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
    void encryptTextProcess(KProcIO *p);
    void encryptTextFin(KProcess *p);

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
    void decryptTextStdOut(KProcess *p, char *data, int);
    void decryptTextStdErr(KProcess *, char *data, int);
    void decryptTextFin(KProcess *p);

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
    void signTextProcess(KProcIO *p);
    void signTextFin(KProcess *p);

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
    void verifyTextProcess(KProcIO *p);
    void verifyTextFin(KProcess*);

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
     * @param checking
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
     * 0 = Unknown error
     * 1 = Bad Passphrase
     * 2 = Expiration changed
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
    /**
     * This method changes the trust of a key
     * @param keyid the id of the key
     * @param keytrust the new trust,
     * Don't know = 1,
     * Do NOT trust = 2,
     * Marginally = 3,
     * Fully = 4,
     * Ultimately = 5.
     */
    void changeTrust(const QString &keyid, const int &keytrust);

private slots:
    void changeTrustProcess(KProcIO *p);
    void changeTrustFin(KProcess *p);

private:
    int m_trustvalue;

/**********************************************/


/************** change disable key **************/
signals:
    void changeDisableFinished(KgpgInterface*);

public slots:
    void changeDisable(const QString &keyid, const bool &ison);

private slots:
    void changeDisableProcess(KProcIO *p);
    void changeDisableFin(KProcess *p);

/************************************************/


/************** load a photo in a QPixmap **************/
signals:
    void loadPhotoFinished(QPixmap, KgpgInterface*);

public slots:
    void loadPhoto(const QString &keyid, const QString &uid);

private slots:
    void loadPhotoProcess(KProcIO *p);
    void loadPhotoFin(KProcess *p);

private:
    KTempFile *m_kgpginfotmp;

/*******************************************************/


/************** add a photo in a key **************/
signals:
    /**
     * 0 = Unknown error
     * 1 = Bad passphrase
     * 2 = Photo added successfully
     * 3 = Aborted
     */
    void addPhotoFinished(int, KgpgInterface*);

public slots:
    void addPhoto(const QString &keyid, const QString &imagepath);

private slots:
    void addPhotoProcess(KProcIO *p);
    void addPhotoFin(KProcess *p);

/**************************************************/


/************** delete a photo of a key **************/
signals:
    /**
     * 0 = Unknown error
     * 1 = Bad passphrase
     * 2 = Photo deleted
     * 3 = Aborted
     */
    void deletePhotoFinished(int, KgpgInterface*);

public slots:
    void deletePhoto(const QString &keyid, const QString &uid);

private slots:
    void deletePhotoProcess(KProcIO *p);
    void deletePhotoFin(KProcess *p);

/*****************************************************/











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



    void KgpgRevokeKey(QString keyID, QString revokeUrl, int reason, QString description);
    void revokeover(KProcess *);
    void revokeprocess(KProcIO *p);

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

    void adduidover(KProcess *);
    void adduidprocess(KProcIO *p);

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

    void refreshOrphaned();

    void addUidFinished();
    void addUidError(QString);

    void revokecertificate(QString);
    void revokeurl(QString);

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

    bool deleteSuccess;
    bool anonymous;
    bool decfinished;
    bool revokeSuccess;
    bool addSuccess;
    bool delSuccess;
    bool signmiss;

    QString signID;
    int expSuccess;
    int step;
    int signb;
    int sigsearch;
    int expirationDelay;
    int revokeReason;
    int photoCount;
    QString revokeDescription;
    QString certificateUrl;
    QString photoUrl;
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
