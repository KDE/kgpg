/*
 * Copyright (C) 2008,2009 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KGPGDELKEY_H
#define KGPGDELKEY_H

#include "kgpgtransaction.h"

#include <core/KGpgKeyNode.h>

#include <QObject>

/**
 * @brief delete a public key
 */
class KGpgDelKey: public KGpgTransaction {
	Q_OBJECT

	Q_DISABLE_COPY(KGpgDelKey)
	KGpgDelKey() = delete;
public:
	KGpgDelKey(QObject *parent, KGpgKeyNode *key);
	KGpgDelKey(QObject *parent, const KGpgKeyNode::List &keys);
	virtual ~KGpgDelKey();

	/**
	 * @brief the keys that were requested to be removed
	 */
	const KGpgKeyNode::List keys;

	/**
	 * @brief the fingerprints of everything in keys
	 */
	const QStringList fingerprints;

protected:
	bool nextLine(const QString &line) override;
	ts_boolanswer boolQuestion(const QString &line) override;
	bool preStart() override;
	
private:
	int m_argscount;

	void setCmdLine();
};

#endif // KGPGDELKEY_H
