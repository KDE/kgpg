/***************************************************************************
                          popuppublic.h  -  description
                             -------------------
    begin                : Sat Jun 29 2002
    copyright            : (C) 2002 by 
    email                : 
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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <qstring.h>
#include <qfile.h>
#include <qobject.h>
#include <qlabel.h>

#include <kled.h>
#include <kprocess.h>
#include <kprocio.h>
#include <kdialogbase.h>
#include <kurl.h>


/**
 * Encrypt a file using gpg.
 */
//class KgpgEncryptFile : public QObject {
class KgpgInterface : public QObject {

  Q_OBJECT

    public:
	/**
	 * Initialize the class
	 */
        KgpgInterface();
	
	/**Encrypt file function
	 * @param userIDs the recipients key id's.
	 * @param srcUrl Kurl of the file to encrypt.
	 * @param destUrl Kurl for the encrypted file.
	 * @param Options String with the wanted gpg options. ex: "--armor"
	 * @param symetrical bool whether the encryption should be symmetrical.
	 */
	void KgpgEncryptFile(QString userIDs,KURL srcUrl,KURL destUrl,QString Options="",bool symetrical=false);
	
	/**Encrypt file function
	 * @param userIDs the key user identification.
	 * @param srcUrl Kurl of the file to decrypt.
	 * @param destUrl Kurl for the decrypted file.
	 * @param chances int number of trials left for decryption (used only as an info displayed in the password dialog)
	 */
	int KgpgDecryptFile(QString userIDs="",KURL srcUrl=0,KURL destUrl=0,int chances=0);
	
	/**Sign file function
	 * @param keyName QString the signing key name.
	 * @param keyID QString the signing key ID.
	 * @param srcUrl Kurl of the file to sign.
	 * @param Options String with the wanted gpg options. ex: "--armor"
	 */
	void KgpgSignFile(QString keyName="",QString keyID="",KURL srcUrl=0,QString Options="");
	
	/**Verify file function
	 * @param srcUrl Kurl of the file to verify.
	 * @param srcUrl Kurl of the signature file.
	 */
	void KgpgVerifyFile(KURL srcUrl,KURL sigUrl) ;
	
	
	void KgpgSignKey(QString keyID="",QString signKeyID="",QString signKeyMail="",bool local=false);
	
	void KgpgDelSignature(QString keyID="",QString signKeyID="");
	
	/**Encrypt text function
	 * @param text QString text to be encrypted.
	 * @param userIDs the recipients key id's.
	 * @param Options String with the wanted gpg options. ex: "--armor"
	 * returns the encrypted text or empty string if encyption failed
	 */
	static QString KgpgEncryptText(QString text,QString userIDs, QString Options="");
	
	static QString KgpgDecryptText(QString text,QString userID="");

	/*
	 * Destructor for the class.
	 */
	~KgpgInterface();
	

	
    private slots:
	
	void signover(KProcess *p);
	void sigprocess(KProcIO *p);//ess *p,char *buf, int buflen);

	
        /**
         * Checks if the encrypted file was saved.
         */
	void encryptfin(KProcess *);
	
	/**
         * Checks if the decrypted file was saved.
         */
	void decryptfin(KProcess *);
	
	/**
         * Checks if the signing was successfull.
         */
	void timerDone();
	void signfin(KProcess *p);
	int checkuid(QString KeyID);
	void delsigprocess(KProcIO *p);
	void delsignover(KProcess *p);
	
	void readprocess(KProcIO *p);//ess *p, char *buff, int bufflen);
	
	void verifyfin(KProcess *p);

signals:
	/**
         * returns true if encryption successfull, false on error.
         */
    void encryptionfinished(bool);	
	/**
         * returns true if decryption successfull, false on error.
         */
	void signatureFinished(int); //// 0=successfull, 1=error, 2=bad passphrase
    void decryptionfinished(bool);
	void badpassphrase(bool);

	        
    private:
    /**
	 * @internal structure for communication
	 */
        QString message;
		QCString passphrase;
		bool deleteSuccess;
		int signSuccess;
		int step,signb,sigsearch;
	/**
	 * @internal structure for the file information
	 */
        KURL file,filedec;
	/**
	 * @internal structure to send signal only once on error.
	 */
	bool encError,decError;
};

 class  Md5Widget :public KDialogBase
 {
     Q_OBJECT
 public:
 Md5Widget(QWidget *parent=0, const char *name=0,KURL url=0);
 ~Md5Widget();
  public slots:
  void slotApply();
  private:
  QString mdSum;
  KLed *KLed1;
QLabel *TextLabel1_2;
 };

#endif
