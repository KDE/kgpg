/*
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

#ifndef KGPGSENDKEYS_H
#define KGPGSENDKEYS_H

#include <QObject>
#include <QString>
#include <QStringList>

#include "kgpgkeyservertransaction.h"

/**
 * @brief class for uploading keys from the keyring to a keyserver
 */
class KGpgSendKeys: public KGpgKeyserverTransaction {
	Q_OBJECT

	Q_DISABLE_COPY(KGpgSendKeys)
	/**
	 * @brief forbidden
	 */
	KGpgSendKeys(); // = delete C++0x
public:
	/**
	 * @brief construct a new transaction for the given keyserver
	 * @param parent object that own the transaction
	 * @param keyserver keyserver to work with
	 * @param keys the key ids to get
	 * @param attropt attributes to export (@see setAttributeOptions())
	 * @param withProgress show a progress window with cancel button
	 * @param proxy http proxy to use
	 *
	 * You should call this from the childrens constructor to set up
	 * everything properly
	 */
	KGpgSendKeys(QObject *parent, const QString &keyserver, const QStringList &keys, const QString &attropt = QString(), const bool withProgress = false, const QString &proxy = QString());
	virtual ~KGpgSendKeys();

	void setKeyIds(const QStringList &keys);
	/**
	 * @brief set which attributes are exported
	 * @param opt GnuPG attribute options
	 *
	 * If opt is empty no attributes are exported.
	 */
	void setAttributeOptions(const QString &opt);

	const QStringList &getLog() const;

protected:
	virtual bool preStart();
	virtual bool nextLine(const QString &line);

private:
	int m_attrpos;
	QString m_attributeopt;
	QStringList m_keys;
	QStringList m_log;
};

#endif // KGPGSENDKEYS_H
