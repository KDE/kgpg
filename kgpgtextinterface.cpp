/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2007,2008,2009,2010,2011,2012
 *               Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kgpgtextinterface.h"

#include "gpgproc.h"
#include "kgpgsettings.h"

#include <KLocale>
#include <QFile>
#include <QString>
#include <QStringList>
#include <QTextCodec>

class KGpgTextInterfacePrivate
{
public:
	KGpgTextInterfacePrivate(QObject *parent, const QString &keyID, const QStringList &options);

	GPGProc * const m_process;
	int m_step;
	const QStringList m_gpgopts;
	KUrl::List m_files;

	void signFile(const QString &fileName);
};

static QStringList
buildCmdLine(const QString &keyID, const QStringList &options)
{
	return QStringList(QLatin1String("-u")) <<
		keyID <<
		options <<
		QLatin1String("--detach-sign") <<
		QLatin1String("--output");
}

KGpgTextInterfacePrivate::KGpgTextInterfacePrivate(QObject *parent, const QString &keyID, const QStringList &options)
	: m_process(new GPGProc(parent)),
	m_step(0),
	m_gpgopts(buildCmdLine(keyID, options))
{
}

void
KGpgTextInterfacePrivate::signFile(const QString &fileName)
{
	m_process->resetProcess();
	*m_process <<
			m_gpgopts <<
			fileName + QLatin1String(".sig") <<
			fileName;

	m_process->start();
}

KGpgTextInterface::KGpgTextInterface(QObject *parent, const QString &keyID, const QStringList &options)
	: QObject(parent),
	d(new KGpgTextInterfacePrivate(parent, keyID, options))
{
	connect(d->m_process, SIGNAL(processExited()), SLOT(slotSignFile()));
}

KGpgTextInterface::~KGpgTextInterface()
{
	delete d->m_process;
	delete d;
}

// signatures
void
KGpgTextInterface::signFiles(const KUrl::List &srcUrls)
{
	d->m_files = srcUrls;

	slotSignFile();
}

void
KGpgTextInterface::slotSignFile()
{
	const QString fileName = d->m_files.takeFirst().path();

	if (d->m_files.isEmpty()) {
		disconnect(d->m_process, SIGNAL(processExited()), this, SLOT(slotSignFile()));
		connect(d->m_process, SIGNAL(processExited()), SLOT(slotSignFinished()));
	}

	d->signFile(fileName);
}

void
KGpgTextInterface::slotSignFinished()
{
	emit fileSignFinished();
}

#include "kgpgtextinterface.moc"
