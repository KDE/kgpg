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

#include "foldercompressjob.h"

#include <KArchive>
#include <KLocale>
#include <KTar>
#include <KTemporaryFile>
#include <KZip>
#include <QDir>
#include <QMetaObject>
#include <QTimer>

FolderCompressJob::FolderCompressJob(QObject *parent, const KUrl::List &sources, const KUrl &dest, KTemporaryFile *tempfile, const QStringList &keys, const QStringList &options,  const KGpgEncrypt::EncryptOptions encOptions, const int archive)
	: KJob(parent),
	m_description(i18n("Processing folder compression and encryption")),
	m_sources(sources),
	m_dest(dest),
	m_tempfile(tempfile),
	m_keys(keys),
	m_options(options),
	m_encOptions(encOptions),
	m_archiveType(archive)
{
}

FolderCompressJob::~FolderCompressJob()
{
}

void
FolderCompressJob::start()
{
	emit description(this, m_description, qMakePair(i18nc("State of operation as in status", "State"), i18nc("Job is started up", "Startup")));
	QMetaObject::invokeMethod(this, "doWork", Qt::QueuedConnection);
}

void
FolderCompressJob::doWork()
{
	KArchive *arch = NULL;

	switch (m_archiveType) {
	case 0:
		arch = new KZip(m_tempfile->fileName());
		break;
	case 1:
		arch = new KTar(m_tempfile->fileName(), QLatin1String( "application/x-gzip" ));
		break;
	case 2:
		arch = new KTar(m_tempfile->fileName(), QLatin1String( "application/x-bzip" ));
		break;
	case 3:
		arch = new KTar(m_tempfile->fileName(), QLatin1String( "application/x-tar" ));
		break;
	case 4:
		arch = new KTar(m_tempfile->fileName(), QLatin1String( "application/x-xz" ));
		break;
	default:
		Q_ASSERT(0);
	}

	if (!arch->open(QIODevice::WriteOnly)) {
		setError(UserDefinedError);
		setErrorText(i18n("Unable to create temporary file"));
		delete arch;
		emitResult();
		return;
	}

	foreach (const KUrl &url, m_sources)
		arch->addLocalDirectory(url.path(), url.fileName());
	arch->close();
	delete arch;

	setPercent(50);

	QDir outPath = m_sources.first().path();
	outPath.cdUp();

	m_options << QLatin1String("--output") << QDir::toNativeSeparators(outPath.path() + QDir::separator()) + m_dest.fileName();

	emit description(this, m_description, qMakePair(i18nc("State of operation as in status", "State"),
			i18nc("Status message 'Encrypting <filename>' (operation starts)", "Encrypting %1", m_dest.path())));
		

	KGpgEncrypt *enc = new KGpgEncrypt(this, m_keys, KUrl::List(KUrl::fromPath(m_tempfile->fileName())), m_encOptions, m_options);
	connect(enc, SIGNAL(done(int)), SLOT(slotEncryptionDone(int)));
	enc->start();
}

void
FolderCompressJob::slotEncryptionDone(int result)
{
	sender()->deleteLater();

	if ((result != KGpgTransaction::TS_OK) && (result != KGpgTransaction::TS_USER_ABORTED)) {
		setError(UserDefinedError + 1);
		setErrorText(i18n("The encryption failed with error code %1", result));
		emit description(this, m_description, qMakePair(i18nc("State of operation as in status", "State"), i18n("Encryption failed.")));
	} else {
		emit description(this, m_description, qMakePair(i18nc("State of operation as in status", "State"),
				i18nc("Status message 'Encrypted <filename>' (operation was completed)", "Encrypted %1", m_dest.path())));
	}

	emitResult();
}

QString
FolderCompressJob::extensionForArchive(const int archive)
{
	switch (archive) {
	case 0:
		return QLatin1String(".zip");
	case 1:
		return QLatin1String(".tar.gz");
	case 2:
		return QLatin1String(".tar.bz2");
	case 3:
		return QLatin1String(".tar");
	case 4:
		return QLatin1String(".tar.xz");
	default:
		Q_ASSERT(archive <= archiveNames().count());
		Q_ASSERT(archive >= 0);
		return QString();
	}
}

QStringList
FolderCompressJob::archiveNames()
{
	static const QStringList archives =
			QStringList(i18n("Zip")) <<
					i18n("Tar/Gzip") <<
					i18n("Tar/Bzip2") <<
					i18n("Tar") <<
					i18n("Tar/XZ");

	return archives;
}

#include "foldercompressjob.moc"
