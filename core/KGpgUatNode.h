/* Copyright 2008,2009,2010 Rolf Eike Beer <kde@opensource.sf-tec.de>
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
#ifndef KGPGUATNODE_H
#define KGPGUATNODE_H

#include "KGpgSignableNode.h"

#include <QDateTime>
#include <QPixmap>

class KGpgKeyNode;
class QPixmap;

class KGpgUatNodePrivate;

/**
 * @brief A user attribute (i.e. photo id) of a public key or key pair
 */
class KGpgUatNode : public KGpgSignableNode
{
private:
	KGpgUatNodePrivate * const d_ptr;
	Q_DECLARE_PRIVATE(KGpgUatNode)

protected:
	void readChildren() override;

public:
	explicit KGpgUatNode(KGpgKeyNode *parent, const unsigned int index, const QStringList &sl);
    ~KGpgUatNode() override;

	KgpgCore::KgpgItemType getType() const override;
	KgpgCore::KgpgKeyTrust getTrust() const override;
	const QPixmap &getPixmap() const;
	QString getId() const override;
	QString getSize() const override;
	QString getName() const override;
	QDateTime getCreation() const override;
	virtual KGpgKeyNode *getParentKeyNode() const;
	KGpgKeyNode *getKeyNode(void) override;
	const KGpgKeyNode *getKeyNode(void) const override;
};

#endif /* KGPGUATNODE_H */
