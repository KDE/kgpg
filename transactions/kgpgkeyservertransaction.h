/**
 * Copyright (C) 2009 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KGPGKEYSERVERTRANSACTION_H
#define KGPGKEYSERVERTRANSACTION_H

#include <QObject>
#include <QString>

#include "kgpgtransaction.h"

/**
 * \brief base class for transactions involving key servers
 */
class KGpgKeyserverTransaction: public KGpgTransaction {
	Q_OBJECT

public:
	/**
	 * \brief construct a new transaction for the given keyserver
	 * @param parent object that own the transaction
	 * @param server keyserver to work with
	 * @param proxy http proxy to use
	 *
	 * You should call this from the childrens constructor to set up
	 * everything properly
	 */
	KGpgKeyserverTransaction(QObject *parent, const QString &keyserver, const QString &proxy = QString());
	virtual ~KGpgKeyserverTransaction();

	/**
	 * \brief set the keyserver
	 * @param server keyserver to work with
	 */
	void setKeyserver(const QString &server);
	/**
	 * \brief set the http proxy
	 * @param proxy http proxy to use
	 *
	 * If the server is set to an empty value no proxy is used.
	 */
	void setProxy(const QString &proxy);

public slots:
	/**
	 * \brief abort the current operation
	 */
	void slotAbort();

private:
	/**
	 * \brief forbidden
	 */
	KGpgKeyserverTransaction();
	Q_DISABLE_COPY(KGpgKeyserverTransaction)

	QString m_keyserver;
	int m_keyserverpos;
	QString m_proxy;
	int m_proxypos;
};

#endif // KGPGUIDTRANSACTION_H
