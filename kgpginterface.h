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

#include <QDateTime>
#include <QPixmap>


#include <KUrl>

#include <kgpgkey.h>

class KTemporaryFile;
class K3Process;
class K3ProcIO;
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

    static int gpgVersion();

    static QStringList getGpgGroupNames(const QString &configfile);
    static QStringList getGpgGroupSetting(const QString &name, const QString &configfile);
    static void setGpgGroupSetting(const QString &name, const QStringList &values, const QString &configfile);
    static void delGpgGroup(const QString &name, const QString &configfile);

    static QString getGpgSetting(QString name, const QString &configfile);
    static void setGpgSetting(const QString &name, const QString &value, const QString &url);

    static bool getGpgBoolSetting(const QString &name, const QString &configfile);
    static void setGpgBoolSetting(const QString &name, const bool &enable, const QString &url);

/************** function to send a passphrase to gpg **************/
private:
    /**
     * @param text text is the message that must be displayed in the MessageBox
     * @param process gnupg process
     * @param isnew if the password is a \e new password that must be confirmed. Default is true
     * @return 0 if there is no error
     * @return 1 if there is an error
     */
    int sendPassphrase(const QString &text, K3ProcIO *process, const bool isnew = true);
/******************************************************************/


/************** function update the userIDs variable **************/
private:
    void updateIDs(QString txt);

/******************************************************************/


/************** extract public keys **************/
signals:
    void readPublicKeysFinished(KgpgCore::KgpgKeyList, KgpgInterface*);

public slots:
    KgpgCore::KgpgKeyList readPublicKeys(const bool &block = false, const QStringList &ids = QStringList(), const bool &withsigs = false);
    KgpgCore::KgpgKeyList readPublicKeys(const bool &block, const QString &ids, const bool &withsigs = false)
	{ return readPublicKeys(block, QStringList(ids), withsigs); }

private slots:
    void readPublicKeysProcess(GPGProc *p);
    void readPublicKeysFin(GPGProc *p, const bool &block = false);

private:
    int m_numberid;
    QString cycle;
    KgpgCore::KgpgKey m_publickey;
    KgpgCore::KgpgKeyList m_publiclistkeys;

/*************************************************/


/************** extract secret keys **************/
public slots:
    KgpgCore::KgpgKeyList readSecretKeys(const QStringList &ids = QStringList());
    KgpgCore::KgpgKeyList readJoinedKeys(const KgpgCore::KgpgKeyTrust &trust, const QStringList &ids = QStringList());

private slots:
    void readSecretKeysProcess(GPGProc *p);

private:
    bool m_secretactivate;
    KgpgCore::KgpgKey m_secretkey;
    KgpgCore::KgpgKeyList m_secretlistkeys;

/*************************************************/


/************** get keys as a text **************/
signals:
    void getKeysStarted(KgpgInterface*);
    void getKeysFinished(QString, KgpgInterface*);

public slots:
    QString getKeys(const bool &block = false, const QString *attributes = NULL, const QStringList &ids = QStringList());

private slots:
    void getKeysProcess(K3ProcIO *p);
    void getKeysFin(K3Process *p);

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
    void encryptTextProcess(K3ProcIO *p);
    void encryptTextFin(K3Process *p);

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
    void decryptTextStdOut(K3Process *p, char *data, int);
    void decryptTextStdErr(K3Process *, char *data, int);
    void decryptTextFin(K3Process *p);

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
    void signTextProcess(K3ProcIO *p);
    void signTextFin(K3Process *p);

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
    void verifyTextProcess(K3ProcIO *p);
    void verifyTextFin(K3Process*);

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
    void errorMessage(const QString &, KgpgInterface*);

    /**
     *  true if encryption successful, false on error.
     */
    void fileEncryptionFinished(KUrl, KgpgInterface*);

public slots:
    /**
     * Encrypt file function
     * @param encryptKeys the recipients key id's.
     * @param srcUrl Kurl of the file to encrypt.
     * @param destUrl Kurl for the encrypted file.
     * @param Options String List with the wanted gpg options. ex: "--armor"
     * @param symetrical bool whether the encryption should be symmetrical.
     */
    void encryptFile(const QStringList &encryptkeys, const KUrl &srcurl, const KUrl &desturl, const QStringList &options = QStringList(), const bool &symetrical = false);

private slots:
    /**
     * Reads output of the current encryption process + allow overwriting of a file
     */
    void fileReadEncProcess(K3ProcIO *p);

    /**
     * Checks if the encrypted file was saved.
     */
    void fileEncryptFin(K3Process *p);

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
    void signKeyProcess(K3ProcIO *p);

    /**
     * Checks output of the signature process
     */
    void signKeyFin(K3Process *p);

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
    void keyExpireProcess(K3ProcIO *p);
    void keyExpireFin(K3Process *p);

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
    void changePassProcess(K3ProcIO *p);
    void changePassFin(K3Process *p);

/*************************************************/


/************** change key trust **************/
signals:
    void changeTrustFinished(KgpgInterface*);

public slots:
    /**
     * This method changes the trust of a key
     * @param keyid the id of the key
     * @param keytrust the new trust
     */
    void changeTrust(const QString &keyid, const KgpgCore::KgpgKeyOwnerTrust &keytrust);

private slots:
    void changeTrustProcess(K3ProcIO *p);
    void changeTrustFin(K3Process *p);

private:
    int m_trustvalue;

/**********************************************/


/************** change disable key **************/
signals:
    void changeDisableFinished(KgpgInterface*, int res);

public slots:
    void changeDisable(const QString &keyid, const bool &ison);

private slots:
    void changeDisableFin(int res);

/************************************************/


/************** load a photo in a QPixmap **************/
signals:
    void loadPhotoFinished(QPixmap, KgpgInterface*);

public slots:
    QPixmap loadPhoto(const QString &keyid, const QString &uid, const bool &block = false);

private slots:
    void loadPhotoFin(K3Process *p, const bool &block = false);

private:
    QPixmap m_pixmap;
    KTemporaryFile *m_kgpginfotmp;

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
    void addPhotoProcess(K3ProcIO *p);
    void addPhotoFin(K3Process *p);

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
    void deletePhotoProcess(K3ProcIO *p);
    void deletePhotoFin(K3Process *p);

/*****************************************************/


// TODO : ajouter KgpgInterface a importKeyFinished
/************** import a key **************/
signals:
    void importKeyFinished(QStringList);
    void importKeyOrphaned();

public slots:
    /**
     * Import key function
     * @param url Kurl the url of the key file. Allows public & secret key import.
     */
    void importKey(const KUrl &url);

    /**
     * Import key function
     * @param keystr QString containing th key. Allows public & secret key import.
     */
    void importKey(const QString &keystr);

private slots:
    /**
     * Read output of the import process
     */
    void importKeyProcess(K3ProcIO *p);
    void importKeyFinished(K3Process *p);

private:
    QString m_tempkeyfile;

/******************************************/


/************** add uid **************/
signals:
    /**
     * 0 = Unknown error
     * 1 = Bad passphrase
     * 2 = Uid Added
     * 3 = Aborted
     * 4 = email is not valid
     */
    void addUidFinished(int, KgpgInterface*);

public slots:
    /**
     * @param email email MUST be a valid email address or an empty string.
     */
    void addUid(const QString &keyid, const QString &name, const QString &email, const QString &comment);

private slots:
    void addUidProcess(K3ProcIO *p);
    void addUidFin(K3Process *p);

private:
    QString uidName;
    QString uidEmail;
    QString uidComment;

/*************************************/


/************** generate a key **************/
signals:
    /**
     * This signal is emitted when the key has been generated or when
     * there is an error.
     * @param int
     * 0 = Unknown error
     * 1 = Bad passphrase
     * 2 = Key generated
     * 3 = Aborted
     * 4 = email is not valid
     * @param KgpgInterface give a pointer to \em this Interface
     * @param QString the first QString is the name (keyname)
     * @param QString the seconde QString is the email (keyemail)
     * @param QString the third QString is the id of the new key if it is created or QString() otherwise
     * @param QString the last QString is the fingerprint
     */
    void generateKeyFinished(int, KgpgInterface*, QString, QString, QString, QString);
    void generateKeyStarted(KgpgInterface*);

public slots:
    /**
     * This slot will generate a new key-pair.
     * @param keyname the name of the key, it is also the user's name.
     * @param keymail email MUST be a valid email address or an empty string.
     * @param keycomment is a comment, it can be an empty string
     * @param keyalgo this is the type of the key, RSA or DSA & ELGAMAL (see Kgpg::KeyAlgo).
     * @param keysize this is the length of the key (1024, 2048, ...)
     * @param keyexp that will indicate if \em keyexpnumber represents: days, weeks, months, years.
     * 0 = no expiration
     * 1 = day
     * 2 = week
     * 3 = month
     * 4 = year
     * @param keyexpnumber is the number of X (see keyexp for X) while the key will be valid.
     */
    void generateKey(const QString &keyname, const QString &keyemail, const QString &keycomment, const KgpgCore::KgpgKeyAlgo &keyalgo, const uint &keysize, const uint &keyexp, const uint &keyexpnumber);

private slots:
    void generateKeyProcess(K3ProcIO *p);
    void generateKeyFin(K3Process *p);

private:
    QString m_newkeyid;
    QString m_newfingerprint;
    QString m_keyname;
    QString m_keyemail;
    QString m_keycomment;
    KgpgCore::KgpgKeyAlgo m_keyalgo;
    uint m_keysize;
    uint m_keyexpnumber;
    uint m_keyexp;

/********************************************/


/************** decrypt file **************/
signals:
    /**
     * 0 = Unknown error
     * 1 = Bad passphrase
     * 2 = Uid Added
     * 3 = Aborted
     * 5 = email is not valid
     */
    void decryptFileStarted(KUrl url);
    void decryptFileFinished(int, KgpgInterface*);

public slots:
    /**
     * Decrypt file function
     * @param src Kurl of the file to decrypt.
     * @param dest Kurl for the decrypted file.
     * @param Options String List with the wanted gpg options
     */
    void decryptFile(const KUrl &src, const KUrl &dest, const QStringList &Options = QStringList());

private slots:
    void decryptFileProcess(K3ProcIO *p);
    void decryptFileFin(K3Process *p);

private:
    KUrl decryptFileUrl;

/******************************************/


/************** download keys from keys server **************/
signals:
    /*
    0 : keys processed
    1 : keys without id
    2 : keys imported
    3 : RSA keys
    4 : unchanged
    5 : Uid imported
    6 : Subkey imported
    7 : Sign imported
    8 : Revocation imported
    9 : secret keys processed
    10 : secret keys imported
    11 : secret keys unchanged
    12 : secret keys not imported
    */
    void downloadKeysFinished(QList<int>, QStringList, bool, QString, KgpgInterface*);
    void downloadKeysAborted(KgpgInterface*);

public slots:
    void downloadKeys(const QStringList &keys, const QString &keyserver, const bool &refresh, const QString &proxy = "");
    void downloadKeysAbort();

private slots:
    void downloadKeysProcess(K3ProcIO *p);
    void downloadKeysFin(K3Process *p);

private:
    K3ProcIO *m_downloadprocess;
    QString m_downloadkeys;
    QString m_downloadkeys_log;

/***********************************************************/



/************** upload keys to keys server **************/
signals:
    void uploadKeysFinished(QString, KgpgInterface*);
    void uploadKeysAborted(KgpgInterface*);

public slots:
    void uploadKeys(const QStringList &keys, const QString &keyserver, const QString &attributes, const QString &proxy = "");
    void uploadKeysAbort();

private slots:
    void uploadKeysProcess(K3ProcIO *p);
    void uploadKeysFin(K3Process *p);

private:
    K3ProcIO *m_uploadprocess;
    QString m_uploadkeys_log;

/********************************************************/













public slots:

    /**
     * Decrypt File to text function
     * @param srcUrl Kurl of the file to decrypt.
     * @param Options StringList with the wanted gpg options.
     */
    void KgpgDecryptFileToText(const KUrl &srcUrl, const QStringList &Options);

    /**
     * Sign file function
     * @param keyID QString the signing key ID.
     * @param srcUrl Kurl of the file to sign.
     * @param Options String with the wanted gpg options. ex: "--armor"
     */
    void KgpgSignFile(const QString &keyID, const KUrl &srcUrl, const QStringList &Options = QStringList());

    /**Verify file function
     * @param sigUrl Kurl of the signature file.
     * @param srcUrl Kurl of the file to be verified. If empty, gpg will try to find it using the signature file name (by removing the .sig extensio)
     */
    void KgpgVerifyFile(const KUrl &sigUrl, const KUrl &srcUrl = KUrl()) ;

    /**
     * Key signature deletion function
     * @param keyID QString the ID of the key
     * @param signKeyID QString the ID of the signature key
     */
    void KgpgDelSignature(const QString &keyID, const QString &uid, QString signKeyID);

    void KgpgRevokeKey(const QString &keyID, const QString &revokeUrl, const int reason, const QString &description);
    void revokeover(K3Process *);
    void revokeprocess(K3ProcIO *p);


private slots:
    /**
     * Checks if the signing was successful.
     */
    void signfin(K3Process *p);

    /**
     * Reads output of the delete signature process
     */
    void delsigprocess(K3ProcIO *p);

    /**
     * Checks output of the delete signature process
     */
    void delsignover(K3Process *p);

    /**
     * Finds the offset of the given signatures to a uid
     */
    void findSigns(const QString &keyID, const QStringList &ids, const QString &uid, QList<int> *res);

    /**
     * Checks output of the import process
     */
    void importURLover(K3Process *p);

    /**
     * Reads output of the current process + allow overwriting of a file
     */
    void readprocess(K3ProcIO *p);

    /**
     * Reads output of the current signing process + allow overwriting of a file
     */
    void readsignprocess(K3ProcIO *p);

    /**
     * Checks output of the verify process
     */
    void verifyfin(K3Process *p);

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
     * emitted if bad passphrase was giver
     */
    void badpassphrase(bool);

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

    void revokecertificate(QString);
    void revokeurl(QString);

private:
    // Globals private
    int m_success;
    QString m_partialline;
    QByteArray m_readin;
    bool m_ispartial;
    QString message;
    QString tmp_message;
    QString userIDs;
    QString log;
    bool encok;                 // encrypt ok
    bool decok;                 // decrypt ok
    bool badmdc;                // bad mdc
    bool badpassword;           // bad password
    KProcess *editprocess;
    static K3ProcIO *gpgProc(const int &statusfd = -1, const int &cmdfd = -1);

    /**
     * @internal structure for communication
     */
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
    KUrl sourceFile;
    QString decryptUrl;

    QString gpgOutput;

    /**
     * @internal structure for the file information
     */
    KUrl file;
};

#endif // KGPGINTERFACE_H
