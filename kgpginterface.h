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

#include <kprocess.h>
#include <kprocio.h>
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
        /**
         * Checks if the encrypted file was saved.
         */
	void encryptfin(KProcess *);
	/**
         * Checks if the encryption was successfull.
         */
	void encrypterror(KProcess *p, char *buf, int buflen);

signals:
	/**
         * returns true if encryption successfull, false on error.
         */
    void encryptionfinished(bool);
	
	        
    private:
	/**
	 * @internal structure for the file information
	 */
        KURL file;
	/**
	 * @internal structure to send signal only once on error.
	 */
	bool encError;
};
#endif
