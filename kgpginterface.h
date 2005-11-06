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

    /**
     * Destructor for the class.
     */
    ~KgpgInterface();

    static int getGpgVersion();


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

private:
    QString m_txttoencrypt;

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

















public slots:


    /**
     * Encrypt file function
     * @param encryptKeys the recipients key id's.
     * @param srcUrl Kurl of the file to encrypt.
     * @param destUrl Kurl for the encrypted file.
     * @param Options String List with the wanted gpg options. ex: "--armor"
     * @param symetrical bool whether the encryption should be symmetrical.
     */
    void KgpgEncryptFile(QStringList encryptKeys, KURL srcUrl, KURL destUrl, QStringList Options = QStringList(), bool symetrical = false);

    /**
     * Decrypt file function
     * @param srcUrl Kurl of the file to decrypt.
     * @param destUrl Kurl for the decrypted file.
     * @param Options String List with the wanted gpg options
     */
    void KgpgDecryptFile(KURL srcUrl, KURL destUrl, QStringList Options = QStringList());


    /**
     * Sign text function
     * @param text QString text to sign.
     * @param userIDs the recipient key id's.
     * @param Options StringList with the wanted gpg options.
     */
    void KgpgSignText(QString text, QString userIDs, QStringList Options);

    /**
     * Decrypt File to text function
     * @param srcUrl Kurl of the file to decrypt.
     * @param Options StringList with the wanted gpg options.
     */
    void KgpgDecryptFileToText(KURL srcUrl, QStringList Options);

    /**
     * Verify text function
     * @param text QString text to be verified.
     */
    void KgpgVerifyText(QString text);

    /**
     * Key signature function
     * @param keyID QString the ID of the key to be signed
     * @param signKeyID QString the ID of the signing key
     * @param signKeyMail QString the name of the signing key (only used to prompt user for passphrase)
     * @param local bool should the signature be local
     */
    void KgpgSignKey(QString keyID, QString signKeyID, QString signKeyMail = QString::null, bool local = false, int checking = 0);

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

    void slotverifyread(KProcIO *p);
    void slotverifyresult(KProcess*);

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

/*    void getOutput(KProcess *, char *data, int);
    void getCmdOutput(KProcess *p, char *data, int);*/

    QString getKey(QStringList IDs, bool attributes);

    void KgpgKeyExpire(QString keyID, QDate date, bool unlimited);
    void KgpgTrustExpire(QString keyID, int keyTrust);
    void KgpgChangePass(QString keyID);

    void KgpgRevokeKey(QString keyID, QString revokeUrl, int reason, QString description);
    void revokeover(KProcess *);
    void revokeprocess(KProcIO *p);
    void KgpgDeletePhoto(QString keyID, QString uid);
    void KgpgAddPhoto(QString keyID, QString imagePath);

    void KgpgAddUid(QString keyID, QString name, QString email, QString comment);

    static QString getGpgSetting(QString name, QString configFile);
    static void setGpgSetting(QString name, QString ID, QString url);
    static bool getGpgBoolSetting(QString name, QString configFile);
    static void setGpgBoolSetting(QString name, bool enable, QString url);
    static QStringList getGpgGroupNames(QString configFile);
    static QStringList getGpgGroupSetting(QString name, QString configFile);
    static void setGpgGroupSetting(QString name, QStringList values, QString configFile);
    static void delGpgGroup(QString name, QString configFile);
    static QString checkForUtf8(QString txt);
    static QString checkForUtf8bis(QString txt);


private slots:
    void openSignConsole();

    /**
     * Checks output of the signature process
     */
    void signover(KProcess *);

    /**
     * Read output of the signature process
     */
    void sigprocess(KProcIO *p);

    /**
     * Checks if the encrypted file was saved.
     */
    void encryptfin(KProcess *);

    /**
     * Checks if the decrypted file was saved.
     */
    void decryptfin(KProcess *);

    /**
     * Checks if the signing was successful.
     */
    void signfin(KProcess *p);

    /**
     * Checks the number of uid's for a key-> if greater than one, key signature will switch to konsole mode
     */
    int checkuid(QString KeyID);

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
     * Reads output of the current encryption process + allow overwriting of a file
     */
    void readencprocess(KProcIO *p);

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

    void expprocess(KProcIO *p);
    void expover(KProcess*);
    void trustprocess(KProcIO *p);
    void passprocess(KProcIO *p);
    void trustover(KProcess *);
    void passover(KProcess *);


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

    void txtsignprocess(KProcIO *p);
    void txtsignfin(KProcess *);

    //void txtreaddecprocess(KProcIO *p);
    //void txtdecryptfin(KProcess *);

signals:
    void missingSignature(QString);
    void verifyOver(QString, QString);




    /**
     *  emitted when an error occurred
     */
    void errormessage(QString);

    /**
     *  true if encryption successful, false on error.
     */
    void encryptionfinished(KURL);

    /**
     *  true if key signature deletion successful, false on error.
     */
    void delsigfinished(bool);

    /**
     * Signature process result: 0=successful, 1=error, 2=bad passphrase
     */
    void signatureFinished(int);

    /**
     *  emitted when user cancels process
     */
    void processaborted(bool);

    /**
     *  emitted when the process starts
     */
    void processstarted(QString);

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

    void trustfinished();
    void revokecertificate(QString);
    void revokeurl(QString);
    void expirationFinished(int);
    void signalPhotoList(QStringList);
    void passwordChanged();

    void txtSignOver(QString);

private:
    // Globals private
    QString m_partialline;
    bool m_ispartial;

    /**
     * @internal structure for communication
     */
    QString message;
    QString tempKeyFile;
    QString userIDs;
    QString output;
    QString keyString;
    QString log;

    bool deleteSuccess;
    bool konsLocal;
    bool anonymous;
    bool decfinished;
    bool decok;
    bool badmdc;
    bool revokeSuccess;
    bool addSuccess;
    bool delSuccess;
    bool signmiss;

    QString signID;
    int signSuccess;
    int expSuccess;
    int trustValue;
    int konsChecked;
    int step;
    int signb;
    int sigsearch;
    int expirationDelay;
    QString konsSignKey;
    QString konsKeyID;
    QString errMessage;
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
    Md5Widget(QWidget *parent = 0, const char *name = 0,KURL url = KURL());
    ~Md5Widget();

public slots:
    void slotApply();

private:
    QString mdSum;
    KLed *KLed1;
    QLabel *TextLabel1_2;
};

#endif // KGPGINTERFACE_HKGPGINTERFACE_H
