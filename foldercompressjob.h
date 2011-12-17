/*
 * Copyright (C) 2011 Rolf Eike Beer <kde@opensource.sf-tec.de>
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
	const KUrl m_source;
	const KUrl m_dest;
	KTemporaryFile * const m_tempfile;
	const QStringList m_keys;
	QStringList m_options;
	const KGpgEncrypt::EncryptOptions m_encOptions;

public:
	/**
	 * @brief create a new KJob to compress and encrypt a folder
	 * @param parent object owning this job
	 * @param description
	 *
	 * The job will take ownership of the transaction, i.e.
	 * will delete the transaction object when the job is done.
	 */
	FolderCompressJob(QObject *parent, const KUrl &source, const KUrl &dest, KTemporaryFile *tempfile, const QStringList &keys, const QStringList &options, const KGpgEncrypt::EncryptOptions encOptions);
	
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
