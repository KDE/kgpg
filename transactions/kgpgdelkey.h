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
	KGpgDelKey() Q_DECL_EQ_DELETE;
public:
	KGpgDelKey(QObject *parent, KGpgKeyNode *key);
	KGpgDelKey(QObject *parent, const KGpgKeyNode::List &keys);
	virtual ~KGpgDelKey();

	/**
	 * @brief the keys that were requested to be removed
	 * @return the list of key nodes
	 */
	KGpgKeyNode::List keys() const;

protected:
	bool nextLine(const QString &line) Q_DECL_OVERRIDE;
	ts_boolanswer boolQuestion(const QString &line) Q_DECL_OVERRIDE;
	bool preStart() Q_DECL_OVERRIDE;
	
private:
	KGpgKeyNode::List m_keys;
	int m_argscount;

	void setCmdLine();
};

#endif // KGPGDELKEY_H
