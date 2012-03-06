/*
 * Copyright (C) 2009,2012 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kgpgkeyservertransaction.h"

#include "gpgproc.h"

#include <KLocale>
#include <KProgressDialog>

KGpgKeyserverTransaction::KGpgKeyserverTransaction(QObject *parent, const QString &keyserver, const bool withProgress, const QString &proxy)
	: KGpgTransaction(parent),
	m_progress(NULL),
	m_showprogress(false)
{
	addArgument(QLatin1String( "--status-fd=1" ));
	addArgument(QLatin1String( "--command-fd=0" ));
	addArgument(QLatin1String( "--keyserver-options" ));
	m_proxypos = addArgument(QString());
	addArgument(QLatin1String( "--keyserver" ));
	m_keyserverpos = addArgument(QString());

	setKeyserver(keyserver);
	setProxy(proxy);

	setProgressEnable(withProgress);
}

KGpgKeyserverTransaction::~KGpgKeyserverTransaction()
{
	delete m_progress;
}

void
KGpgKeyserverTransaction::setKeyserver(const QString &server)
{
	m_keyserver = server;

	replaceArgument(m_keyserverpos, server);
}

void
KGpgKeyserverTransaction::setProxy(const QString &proxy)
{
	m_proxy = proxy;

	replaceArgument(m_proxypos, proxy);
}

void
KGpgKeyserverTransaction::finish()
{
	if (m_progress != NULL)
		m_progress->hide();
}

bool
KGpgKeyserverTransaction::preStart()
{
	if (m_showprogress) {
		Q_ASSERT(m_progress != NULL);
		m_progress->show();
	}

	return true;
}

void
KGpgKeyserverTransaction::slotAbort()
{
	// no idea if this works on Windows, maybe we need ->kill() there
	getProcess()->terminate();
	setSuccess(TS_USER_ABORTED);
}

void
KGpgKeyserverTransaction::setProgressEnable(const bool b)
{
	m_showprogress = b;

	if (b && (m_progress == NULL)) {
		m_progress = new KProgressDialog(qobject_cast<QWidget *>(parent()),
				i18n("Keyserver"), i18n("<b>Connecting to the server...</b>"));

		m_progress->hide();
		m_progress->setModal(true);
		m_progress->progressBar()->setRange(0, 0);

		connect(m_progress, SIGNAL(cancelClicked()), SLOT(slotAbort()));
	}
}

#include "kgpgkeyservertransaction.moc"
