/*
 * Copyright (C) 2011,2012 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef FOLDERCOMPRESSJOB_H
#define FOLDERCOMPRESSJOB_H

#include <KJob>
#include <KUrl>
#include <QStringList>

class KTemporaryFile;

#include "transactions/kgpgencrypt.h"

/**
 * @brief Show systray status for something KGpg is doing in the background
 *
 * @author Rolf Eike Beer
 */
class FolderCompressJob : public KJob {
	Q_OBJECT

	Q_DISABLE_COPY(FolderCompressJob)
	FolderCompressJob(); // = delete C++0x

	const QString m_description;
	const KUrl::List m_sources;
	const KUrl m_dest;
	KTemporaryFile * const m_tempfile;
	const QStringList m_keys;
	QStringList m_options;
	const KGpgEncrypt::EncryptOptions m_encOptions;
	const int m_archiveType;

public:
	/**
	 * @brief create a new KJob to compress and encrypt a folder
	 * @param parent object owning this job
	 * @param sources the source directories to include
	 * @param dest the name of the encrypted file
	 * @param tempfile the temporary file that should be used for archiving
	 * @param keys the public key ids to encrypt to
	 * @param options special options to pass to the GnuPG process
	 * @param encOptions special options to pass to the GnuPG process
	 * @param archive the archive type to use
	 */
	FolderCompressJob(QObject *parent, const KUrl::List &sources, const KUrl &dest, KTemporaryFile *tempfile, const QStringList &keys, const QStringList &options, const KGpgEncrypt::EncryptOptions encOptions, const int archive);

	/**
	 * @brief FolderCompressJob destructor
	 */
	virtual ~FolderCompressJob();

	/**
	 * @brief shows the progress indicator
	 */
	virtual void start();

private slots:
	void doWork();
	void slotEncryptionDone(int result);
};

#endif /* FOLDERCOMPRESSJOB_H */
