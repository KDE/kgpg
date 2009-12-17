/* Copyright 2008,2009 Rolf Eike Beer <kde@opensource.sf-tec.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef KGPGSIGNNODE_H
#define KGPGSIGNNODE_H

#include <KGpgRefNode.h>

#include "kgpgkey.h"

class KGpgExpandableNode;

/**
 * @brief A signature to another key object
 */
class KGpgSignNode : public KGpgRefNode
{
private:
	KgpgCore::KgpgKeySign *m_sign;

public:
	typedef QList<KGpgSignNode *> List;

	explicit KGpgSignNode(KGpgExpandableNode *parent, const KgpgCore::KgpgKeySign &s);
	virtual ~KGpgSignNode();

	virtual KgpgCore::KgpgItemType getType() const;
	virtual QDate getExpiration() const;
	virtual QString getName() const;
	virtual QDate getCreation() const;
	virtual QString getId() const;
	virtual QString getComment() const;
};

#endif /* KGPGSIGNNODE_H */
