/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2007,2008,2009,2010,2011,2012 Rolf Eike Beer <kde@opensource.sf-tec.de>
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
     *  emmitted if signature key is missing & user want to import it from keyserver
     */
    void verifyquerykey(QString ID);

    /**
     *  true if verify successful, false on error.
     */
    void verifyfinished();

    void txtSigningFinished(QString);

    void txtVerifyMissingSignature(QString);
    void txtVerifyFinished(QString, QString);

    /**
     * Emitted when all files passed to KgpgSignFile() where processed.
     * Passes as argument the files that failed.
     */
    void fileSignFinished(KUrl::List &);

public Q_SLOTS:
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

	void slotSignFile(int);
	void slotSignFinished(int);
};

#endif
