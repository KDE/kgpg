/**
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2007,2008 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KGPGTEXTINTERFACE_H
#define KGPGTEXTINTERFACE_H

#include <QStringList>
#include <QString>
#include <QProcess>

#include <KUrl>

class KGpgTextInterfacePrivate;

class KGpgTextInterface : public QObject
{
	Q_OBJECT

private:
	KGpgTextInterfacePrivate *d;

public:
	explicit KGpgTextInterface(QObject *parent = 0);
	~KGpgTextInterface();

Q_SIGNALS:
    /**
     *  emitted when a txt encryption finished. returns encrypted text
     */
    void txtEncryptionFinished(QString, KGpgTextInterface*);

    /**
     *  emmitted if signature key is missing & user want to import it from keyserver
     */
    void verifyquerykey(QString ID);

    /**
     *  true if verify successful, false on error.
     */
    void verifyfinished();

    /**
     *  emitted when a txt decryption finished. returns decrypted text
     */
    void txtDecryptionFinished(QByteArray, KGpgTextInterface*);

    /**
     *  emitted when a txt decryption failed. returns log output
     */
    void txtDecryptionFailed(QString, KGpgTextInterface*);

    void txtSigningFinished(QString, KGpgTextInterface*);
    void txtSigningFailed(QString, KGpgTextInterface*);

    void txtVerifyMissingSignature(QString, KGpgTextInterface*);
    void txtVerifyFinished(QString, QString, KGpgTextInterface*);

    /**
     *  emitted when the process starts
     */
    void processstarted(QString);

    /**
     *  emitted when an error occurred
     */
    void errorMessage(const QString &, KGpgTextInterface*);

    /**
     *  true if encryption successful, false on error.
     */
    void fileEncryptionFinished(KUrl, KGpgTextInterface*);

    void decryptFileStarted(KUrl url);
    /**
     * @brief emitted when the decryption has finished
     *
     * @param result decryption result
     * 0 = decryption successful
     * 1 = Bad passphrase
     * 2 = the gpg process crashed or was killed
     * 3 = gpg process returned with error
     * 4 = gpg did not print successful status
     * @param iface pointer to this class
     */
    void decryptFileFinished(int result, KGpgTextInterface *iface);

    /**
     * Emitted when all files passed to KgpgSignFile() where processed.
     * Passes as argument the files that failed.
     */
    void fileSignFinished(KGpgTextInterface *, KUrl::List &);

public Q_SLOTS:
    /**
     * Encrypt text function
     * @param text text to be encrypted.
     * @param userids the recipients key id's.
     * @param options a list of string with the wanted gpg options. ex: "--armor"
     */
    void encryptText(const QString &text, const QStringList &userids, const QStringList &options = QStringList());

    /**
     * Decrypt text function
     * @param text QString text to be decrypted.
     * @param options StringList with the wanted gpg options.
     */
    void decryptText(const QString &text, const QStringList &options = QStringList());

    /**
     * Sign text function
     * @param text QString text to sign.
     * @param userid the recipient key id's.
     * @param options StringList with the wanted gpg options.
     */
    void signText(const QString &text, const QString &userid, const QStringList &options);

    /**
     * Verify text function
     * @param text QString text to be verified.
     */
    void verifyText(const QString &text);

    /**
     * Encrypt file function
     * @param encryptkeys the recipients key id's.
     * @param srcurl Kurl of the file to encrypt.
     * @param desturl Kurl for the encrypted file.
     * @param options String List with the wanted gpg options. ex: "--armor"
     * @param symetrical bool whether the encryption should be symmetrical.
     */
    void encryptFile(const QStringList &encryptkeys, const KUrl &srcurl, const KUrl &desturl, const QStringList &options = QStringList(), const bool &symetrical = false);

    /**
     * Decrypt file function
     * @param src Kurl of the file to decrypt.
     * @param dest Kurl for the decrypted file.
     * @param options String List with the wanted gpg options
     */
    void decryptFile(const KUrl &src, const KUrl &dest, const QStringList &options = QStringList());

    /**
     * Decrypt File to text function
     * @param srcUrl Kurl of the file to decrypt.
     * @param options StringList with the wanted gpg options.
     */
    void KgpgDecryptFileToText(const KUrl &srcUrl, const QStringList &options);

    /**
     * Sign file function
     * @param keyID QString the signing key ID.
     * @param srcUrl Kurl of the file to sign.
     * @param options String with the wanted gpg options. ex: "--armor"
     */
    void signFiles(const QString &keyID, const KUrl::List &srcUrl, const QStringList &options = QStringList());
    void signFilesBlocking(const QString &keyID, const KUrl::List &srcUrls, const QStringList &options);

    /**Verify file function
     * @param sigUrl Kurl of the signature file.
     * @param srcUrl Kurl of the file to be verified. If empty, gpg will try to find it using the signature file name (by removing the .sig extensio)
     */
    void KgpgVerifyFile(const KUrl &sigUrl, const KUrl &srcUrl = KUrl()) ;

private Q_SLOTS:
    void encryptTextProcess();
    void encryptTextFin();

    void decryptTextStdOut();
    void decryptTextStdErr();
    void decryptTextFin();

    /**
     * Reads output of the current process + allow overwriting of a file
     */
    void readVerify();

    /**
     * Checks output of the verify process
     */
    void verifyfin();

    void signTextProcess();
    void signTextFin();

    void verifyTextFin();

    /**
     * Reads output of the current encryption process + allow overwriting of a file
     */
    void fileReadEncProcess();

    /**
     * Checks if the encrypted file was saved.
     */
    void fileEncryptFin();

    void decryptFileProcess();
	void decryptFileFin(int, QProcess::ExitStatus);

	void slotSignFile(int);
	void slotSignFinished(int);
};

#endif
