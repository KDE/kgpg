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


#include <qobject.h>
#include <kdialogbase.h>
#include <kurl.h>
#include <qdatetime.h>

class QLabel;
class KProcIO;
class KProcess;
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

	        /*
         * Destructor for the class.
         */
        ~KgpgInterface();

public slots:

        /**Encrypt file function
         * @param userIDs the recipients key id's.
         * @param srcUrl Kurl of the file to encrypt.
         * @param destUrl Kurl for the encrypted file.
         * @param Options String with the wanted gpg options. ex: "--armor"
         * @param symetrical bool whether the encryption should be symmetrical.
         */
        void KgpgEncryptFile(QStringList encryptKeys,KURL srcUrl,KURL destUrl,QStringList Options=QString::null,bool symetrical=false);

        /**Encrypt file function
         * @param userIDs the key user identification.
         * @param srcUrl Kurl of the file to decrypt.
         * @param destUrl Kurl for the decrypted file.
         * @param chances int number of trials left for decryption (used only as an info displayed in the password dialog)
         */
        void KgpgDecryptFile(KURL srcUrl,KURL destUrl,QStringList Options=QStringList());

        /**Sign file function
         * @param keyID QString the signing key ID.
         * @param srcUrl Kurl of the file to sign.
         * @param Options String with the wanted gpg options. ex: "--armor"
         */
        void KgpgSignFile(QString keyID,KURL srcUrl,QStringList Options=QStringList());

        /**Verify file function
         * @param sigUrl Kurl of the signature file.
         * @param srcUrl Kurl of the file to be verified. If empty, gpg will try to find it using the signature file name (by removing the .sig extensio)
         */
        void KgpgVerifyFile(KURL sigUrl,KURL srcUrl=KURL()) ;

	void KgpgVerifyText(QString text);
	void slotverifyread(KProcIO *p);
	void slotverifyresult(KProcess*);
	
	
        /**Import key function
         * @param url Kurl the url of the key file. Allows public & secret key import.
         */
        void importKeyURL(KURL url);
        /**Import key function
         * @param keystr QString containing th key. Allows public & secret key import.
        */
        void importKey(QString keystr);

        /**Key signature function
         * @param keyID QString the ID of the key to be signed
         * @param signKeyID QString the ID of the signing key
         * @param signKeyMail QString the name of the signing key (only used to prompt user for passphrase)
         * @param local bool should the signature be local
         */
        void KgpgSignKey(QString keyID,QString signKeyID,QString signKeyMail=QString::null,bool local=false,int checking=0);

        /**Key signature deletion function
         * @param keyID QString the ID of the key
         * @param signKeyID QString the ID of the signature key
         */
        void KgpgDelSignature(QString keyID,QString signKeyID);

        /**Encrypt text function
         * @param text QString text to be encrypted.
         * @param userIDs the recipients key id's.
         * @param Options String with the wanted gpg options. ex: "--armor"
         * returns the encrypted text or empty string if encyption failed
         */
        void KgpgEncryptText(QString text,QStringList userIDs, QStringList Options=QString::null);

        /**Decrypt text function
        * @param text QString text to be decrypted.
        * @param userID QString the name of the decryption key (only used to prompt user for passphrase)
        */
	//static QString KgpgDecryptText(QString text,QString userID);
	void KgpgDecryptText(QString text,QStringList Options=QString::null);
	void txtdecryptfin(KProcess *);

	/**Extract list of photographic user id's
        * @param keyID the recipients key id's.
        */
	void KgpgGetPhotoList(QString keyID);

	void getOutput(KProcess *, char *data, int );
	void getCmdOutput(KProcess *p, char *data, int );

	QString getKey(QStringList IDs, bool attributes);

        void KgpgKeyExpire(QString keyID,QDate date,bool unlimited);
        void KgpgTrustExpire(QString keyID,int keyTrust);
	void KgpgChangePass(QString keyID);

	void KgpgRevokeKey(QString keyID,QString revokeUrl,int reason,QString description);
	void revokeover(KProcess *);
	void revokeprocess(KProcIO *p);
	void KgpgDeletePhoto(QString keyID,QString uid);
	void KgpgAddPhoto(QString keyID,QString imagePath);

	void KgpgAddUid(QString keyID,QString name,QString email,QString comment);
	
        void KgpgDecryptFileToText(KURL srcUrl,QStringList Options);
	void KgpgSignText(QString text,QString userIDs, QStringList Options);

        static QString getGpgSetting(QString name,QString configFile);
	static QString getGpgMultiSetting(QString name,QString configFile);
        static void setGpgSetting(QString name,QString ID,QString url);
	static void setGpgMultiSetting(QString name,QStringList values,QString url);
        static bool getGpgBoolSetting(QString name,QString configFile);
	static void setGpgBoolSetting(QString name,bool enable,QString url);
        static QStringList getGpgGroupNames(QString configFile);
	static QStringList getGpgGroupSetting(QString name,QString configFile);
	static void setGpgGroupSetting(QString name,QStringList values, QString configFile);
	static void delGpgGroup(QString name, QString configFile);
	static QString checkForUtf8(QString txt);
	static QString checkForUtf8bis(QString txt);
	static int getGpgVersion();



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

        void txtreadencprocess(KProcIO *p);

        void txtencryptfin(KProcess *);

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

        //void txtdecryptfin(KProcess *);


signals:

	void missingSignature(QString);
	void verifyOver(QString,QString);

	/**
               *  emitted when a txt decryption failed. returns log output
               */
	void txtdecryptionfailed(QString);
	/**
               *  emitted when a txt encryption starts.
               */
	void txtencryptionstarted();
	
	/**
               *  emitted when a txt decryption finished. returns decrypted text
               */
	void txtdecryptionfinished(QString);
        /**
               *  emitted when a txt encryption finished. returns encrypted text
               */
        void txtencryptionfinished(QString);
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
        /**
        * @internal structure for communication
        */
        QString message,tempKeyFile,userIDs,output,keyString,txtToEncrypt,log;
        QCString passphrase;
        bool deleteSuccess,konsLocal,anonymous,decfinished,decok,badmdc,revokeSuccess,addSuccess,delSuccess;
	bool signmiss;
	QString signID;
        int signSuccess,expSuccess,trustValue,konsChecked;
        int step,signb,sigsearch,expirationDelay;
        QString konsSignKey, konsKeyID,errMessage;
	int revokeReason,photoCount;
	QString revokeDescription,certificateUrl,photoUrl;
	QStringList photoList;
	QString uidName, uidEmail, uidComment;
        KURL sourceFile;
	QString decryptUrl;

	QString gpgOutput;
	
        /**
         * @internal structure for the file information
         */
        KURL file;

};

class  Md5Widget :public KDialogBase
{
        Q_OBJECT
public:
        Md5Widget(QWidget *parent=0, const char *name=0,KURL url=KURL());
        ~Md5Widget();
public slots:
        void slotApply();
private:
        QString mdSum;
        KLed *KLed1;
        QLabel *TextLabel1_2;
};

#endif // KGPGINTERFACE_HKGPGINTERFACE_H

