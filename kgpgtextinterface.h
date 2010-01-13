/*
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

signals:
    /**
     *  emitted when a txt encryption finished. returns encrypted text
     */
    void txtEncryptionFinished(QString);

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
    void txtDecryptionFinished(QByteArray);

    /**
     *  emitted when a txt decryption failed. returns log output
     */
    void txtDecryptionFailed(QString);

    void txtSigningFinished(QString);
    void txtSigningFailed(QString);

    void txtVerifyMissingSignature(QString);
    void txtVerifyFinished(QString, QString);

    /**
     *  emitted when the process starts
     */
    void processstarted(QString);

    /**
     *  emitted when an error occurred
     */
    void errorMessage(const QString &);

    /**
     *  true if encryption successful, false on error.
     */
    void fileEncryptionFinished(KUrl);

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
    void decryptFileFinished(int result);

    /**
     * Emitted when all files passed to KgpgSignFile() where processed.
     * Passes as argument the files that failed.
     */
    void fileSignFinished(KUrl::List &);

public Q_SLOTS:
    /**
     * Encrypt text function
     * @param text text to be encrypted.
     * @param userids the recipients key id's.
     * @param options a list of string with the wanted gpg options, e.g. "--armor"
     */
    void encryptText(const QString &text, const QStringList &userids, const QStringList &options = QStringList());

    /**
     * Decrypt text function
     * @param text text to be decrypted.
     * @param options additional gpg options.
     */
    void decryptText(const QString &text, const QStringList &options = QStringList());

    /**
     * Sign text function
     * @param text text to sign.
     * @param userid the recipient key id's.
     * @param options additional gpg options.
     */
    void signText(const QString &text, const QString &userid, const QStringList &options);

    /**
     * Verify text function
     * @param text QString text to be verified.
     */
    void verifyText(const QString &text);

    /**
     * Encrypt file function
     * @param encryptkeys the recipients key id's
     * @param srcurl file to encrypt
     * @param desturl encrypted file will be stored here
     * @param options additional gpg options, e.g. "--armor"
     * @param symetrical whether the encryption should be symmetrical.
     */
    void encryptFile(const QStringList &encryptkeys, const KUrl &srcurl, const KUrl &desturl, const QStringList &options = QStringList(), const bool &symetrical = false);

    /**
     * Decrypt file function
     * @param src file to decrypt.
     * @param dest decrypted file will be stored here
     * @param options additional gpg options
     */
    void decryptFile(const KUrl &src, const KUrl &dest, const QStringList &options = QStringList());

    /**
     * Decrypt File to text function
     * @param srcUrl file to decrypt.
     * @param options additional gpg options
     */
    void KgpgDecryptFileToText(const KUrl &srcUrl, const QStringList &options);

    /**
     * Sign file function
     * @param keyID the signing key ID.
     * @param srcUrl file to sign.
     * @param options additional gpg options, e.g. "--armor"
     */
    void signFiles(const QString &keyID, const KUrl::List &srcUrl, const QStringList &options = QStringList());
    void signFilesBlocking(const QString &keyID, const KUrl::List &srcUrls, const QStringList &options);

    /**Verify file function
     * @param sigUrl signature file.
     * @param srcUrl Kurl of the file to be verified. If empty, gpg will try to find it using the signature file name (by removing the .sig extension)
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
