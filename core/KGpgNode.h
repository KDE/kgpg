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
#ifndef KGPGNODE_H
#define KGPGNODE_H

#include <QPixmap>
#include "kgpgkey.h"

class KGpgExpandableNode;
class KGpgKeyNode;
class KGpgRootNode;
class KGpgUidNode;
class KGpgSignableNode;
class KGpgSubkeyNode;
class KGpgUatNode;
class KGpgGroupNode;
class KGpgRefNode;
class KGpgGroupMemberNode;
class KGpgSignNode;
class KGpgOrphanNode;
class KGpgItemModel;

/**
 * @brief The abstract base class for all classes representing keyring data
 */
class KGpgNode : public QObject
{
	Q_OBJECT

	friend class KGpgItemModel;

	KGpgNode(); // = delete C++0x
protected:
	KGpgExpandableNode *m_parent;
	KGpgItemModel *m_model;
	explicit KGpgNode(KGpgExpandableNode *parent = NULL);

public:
	typedef QList<KGpgNode *> List;

	virtual ~KGpgNode();

	virtual bool hasChildren() const;
	// this can't be "const" as it triggers loading the children
	virtual int getChildCount();
	virtual KGpgNode *getChild(const int &index) const;
	virtual int getChildIndex(KGpgNode *node) const;

	/**
	 * Returns the item type of this object
	 *
	 * Since every subclass returns a distinct value you can use the
	 * result of this function to decide which cast to take. Note that
	 * there are subclasses (KGpgKeyNode, KGpgGroupMemberNode) that
	 * can return two different values.
	 */
	virtual KgpgCore::KgpgItemType getType() const = 0;
	virtual KgpgCore::KgpgKeyTrust getTrust() const;
	virtual QString getSize() const;
	virtual QString getName() const;
	virtual QString getEmail() const;
	virtual QDate getExpiration() const;
	virtual QDate getCreation() const;
	virtual QString getId() const;
	virtual QString getComment() const;
	virtual QString getNameComment() const;
	/**
	 * Returns the parent node in the key hierarchy
	 *
	 * For all "primary" items like keys and key groups this will
	 * return the (invisible) root node. Calling this function for
	 * the root node will return %NULL. No other node but the root
	 * node has a %NULL parent.
	 */
	KGpgExpandableNode *getParentKeyNode() const;

	KGpgExpandableNode *toExpandableNode();
	const KGpgExpandableNode *toExpandableNode() const;
	KGpgSignableNode *toSignableNode();
	const KGpgSignableNode *toSignableNode() const;
	KGpgKeyNode *toKeyNode();
	const KGpgKeyNode *toKeyNode() const;
	KGpgRootNode *toRootNode();
	const KGpgRootNode *toRootNode() const;
	KGpgUidNode *toUidNode();
	const KGpgUidNode *toUidNode() const;
	KGpgSubkeyNode *toSubkeyNode();
	const KGpgSubkeyNode *toSubkeyNode() const;
	KGpgUatNode *toUatNode();
	const KGpgUatNode *toUatNode() const;
	KGpgGroupNode *toGroupNode();
	const KGpgGroupNode *toGroupNode() const;
	KGpgRefNode *toRefNode();
	const KGpgRefNode *toRefNode() const;
	KGpgGroupMemberNode *toGroupMemberNode();
	const KGpgGroupMemberNode *toGroupMemberNode() const;
	KGpgSignNode *toSignNode();
	const KGpgSignNode *toSignNode() const;
	KGpgOrphanNode *toOrphanNode();
	const KGpgOrphanNode *toOrphanNode() const;
};

#endif /* KGPGNODE_H */
