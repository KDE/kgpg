/* Copyright 2008  Rolf Eike Beer <kde@opensource.sf-tec.de>
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
#ifndef KGPGITEMNODE_H
#define KGPGITEMNODE_H

#include <QPixmap>
#include "kgpgkey.h"

using namespace KgpgCore;

class KGpgExpandableNode;
class KGpgKeyNode;
class KGpgRootNode;
class KGpgUidNode;
class KGpgSubkeyNode;
class KGpgUatNode;
class KGpgGroupNode;
class KGpgRefNode;
class KGpgGroupMemberNode;
class KGpgSignNode;
class KGpgOrphanNode;
class KGpgItemModel;

class KGpgNode : public QObject
{
	Q_OBJECT

	friend class KGpgItemModel;

protected:
	KGpgExpandableNode *m_parent;
	KGpgItemModel *m_model;

public:
	explicit KGpgNode(KGpgExpandableNode *parent = 0);
	virtual ~KGpgNode();

	virtual bool hasChildren() const
		{ return false; }
	// this can't be "const" as it triggers loading the children
	virtual int getChildCount()
		{ return 0; }
	virtual KGpgNode *getChild(const int &index) const
		{
			Q_UNUSED(index);
			return NULL;
		}
	virtual int getChildIndex(KGpgNode *node) const
		{
			Q_UNUSED(node);
			return 0;
		}

	/**
	 * Returns the item type of this object
	 *
	 * Since every subclass returns a distinct value you can use the
	 * result of this function to decide which cast to take. Note that
	 * there are subclasses (KGpgKeyNode, KGpgGroupMemberNode) that
	 * can return two different values.
	 */
	virtual KgpgItemType getType() const = 0;
	virtual KgpgKeyTrust getTrust() const
		{ return TRUST_NOKEY; }
	virtual QString getSize() const
		{ return QString(); }
	virtual QString getName() const
		{ return QString(); }
	virtual QString getEmail() const
		{ return QString(); }
	virtual QDate getExpiration() const
		{ return QDate(); }
	virtual QDate getCreation() const
		{ return QDate(); }
	virtual QString getId() const
		{ return QString(); }
	virtual QString getComment() const
		{ return QString(); }
	virtual QString getNameComment() const;
	/**
	 * Returns the parent node in the key hierarchy
	 *
	 * For all "primary" items like keys and key groups this will
	 * return the (invisible) root node. Calling this function for
	 * the root node will return %NULL. No other node but the root
	 * node has a %NULL parent.
	 */
	KGpgExpandableNode *getParentKeyNode() const
		{ return m_parent; }

	KGpgExpandableNode *toExpandableNode();
	KGpgKeyNode *toKeyNode();
	KGpgRootNode *toRootNode();
	KGpgUidNode *toUidNode();
	KGpgSubkeyNode *toSubkeyNode();
	KGpgUatNode *toUatNode();
	KGpgGroupNode *toGroupNode();
	KGpgRefNode *toRefNode();
	KGpgGroupMemberNode *toGroupMemberNode();
	KGpgSignNode *toSignNode();
	KGpgOrphanNode *toOrphanNode();
};

class KGpgExpandableNode : public KGpgNode
{
	friend class KGpgRefNode;
	friend class KGpgSubkeyNode;

protected:
	QList<KGpgNode *> children;

	virtual void readChildren() = 0;

public:
	explicit KGpgExpandableNode(KGpgExpandableNode *parent = 0);
	virtual ~KGpgExpandableNode();

	virtual bool hasChildren() const
		{ return (children.count() != 0); }
	virtual bool wasExpanded() const
		{ return (children.count() != 0); }
	virtual int getChildCount();
	virtual KGpgNode *getChild(const int &index) const;
	virtual int getChildIndex(KGpgNode *node) const
		{ return children.indexOf(node); }
	virtual void deleteChild(KGpgNode *child)
		{ children.removeAll(child); }
};

class KGpgKeyNode : public KGpgExpandableNode
{
	Q_OBJECT

	friend class KGpgGroupMemberNode;

private:
	KgpgKey *m_key;
	int m_signs;

	void insertSigns(KGpgExpandableNode *node, const KgpgKeySignList &list);

protected:
	virtual void readChildren();

	QList<KGpgRefNode *> m_refs;

public:
	explicit KGpgKeyNode(KGpgExpandableNode *parent, const KgpgKey &k);
	virtual ~KGpgKeyNode();

	virtual bool hasChildren() const
		{ return true; }

	static KgpgItemType getType(const KgpgKey *k);

	virtual KgpgItemType getType() const;
	virtual KgpgKeyTrust getTrust() const
		{ return m_key->trust(); }
	QString getKeyId() const
		{ return m_key->fullId(); }
	virtual QString getSize() const;
	virtual QString getName() const;
	virtual QString getEmail() const;
	virtual QDate getExpiration() const;
	virtual QDate getCreation() const;
	virtual QString getId() const;
	/**
	 * Print the full key fingerprint with spaces inserted
	 *
	 * For display purposes you normally don't want to print the full
	 * fingerprint as is because it's too many hex characters at once.
	 * This function returns the fingerprint in the format usually used
	 * for printing this out, i.e. with a space after each fourth hex
	 * character.
	 *
	 * @return the full fingerprint with spaces inserted
	 */
	virtual QString getBeautifiedFingerprint() const
		{ return m_key->fingerprintBeautified(); }
	virtual QString getComment() const
		{ return m_key->comment(); }
	/**
	 * Return the number of signatures of the primary user id
	 *
	 * This is different from the number of children of this node as there
	 * is usually at least one subkey and there may also be additional
	 * user ids or attributes. This does not count the signatures to those
	 * slave objects, only the ones that are direct children of this node.
	 *
	 * @return the number of signatures to the primary user id
	 */
	virtual QString getSignCount() const;
	/**
	 * Creates a copy of the KgpgKey that belongs to this class
	 */
	virtual KgpgKey *copyKey() const;
	/**
	 * Replaces the current key information with the new one. All sub-items
	 * (i.e. signatures, user ids ...) will be deleted. This must only be
	 * used when the id of both new and old key is the same.
	 */
	void setKey(const KgpgKey &key);
	/**
	 * Returns a reference to the key used in this object. This allows
	 * direct access to the values of the key e.g. for KgpgKeyInfo
	 */
	const KgpgKey *getKey() const;

	/**
	 * Returns the size of the signing key.
	 * @return signing key size in bits
	 */
	virtual unsigned int getSignKeySize() const;
	/**
	 * Returns the size of the first encryption subkey.
	 * @return encryption key size in bits
	 */
	virtual unsigned int getEncryptionKeySize() const;
	/**
	 * Notify this key that a KGpgRefNode now references this key.
	 * @param node object that takes the reference
	 */
	void addRef(KGpgRefNode *node);
	/**
	 * Remove a reference to this object
	 * @param node node that no longer has the reference
	 *
	 * Note that this must not be called as reply when this object
	 * emits updated(NULL)
	 */
	void delRef(KGpgRefNode *node);
	/**
	 * returns a list of all groups this key is member of
	 */
	QList <KGpgGroupNode *> getGroups(void) const;
	/**
	 * returns a list of all group member nodes that reference this key
	 */
	QList <KGpgGroupMemberNode *> getGroupRefs(void) const;

Q_SIGNALS:
	void updated(KGpgKeyNode *);
};

typedef QList<KGpgKeyNode *> KGpgKeyNodeList;

class KGpgRootNode : public KGpgExpandableNode
{
	Q_OBJECT

	friend class KGpgGroupNode;

private:
	int m_groups;

protected:
	virtual void readChildren()
		{};

public:
	explicit KGpgRootNode(KGpgItemModel *model);
	virtual ~KGpgRootNode()
		{}

	virtual KgpgItemType getType() const
		{ return 0; }

	void addGroups();
	void addKeys(const QStringList &ids = QStringList());
	void refreshKeys(KGpgKeyNodeList nodes);
	/**
	 * Find a key node with the given id
	 *
	 * This scans the list of primary keys for a key with the given id
	 * and returns the corresponding key node.
	 *
	 * The key id will be matched against the characters given in keyId.
	 * If you give only 8 or 16 byte you will still find the key if it
	 * exists. To be really sure to find the correct node you should pass
	 * the complete fingerprint whenever possible.
	 *
	 * @param keyId the key id to find, any length is permitted
	 * @return pointer to key node or %NULL if no such key
	 */
	KGpgKeyNode *findKey(const QString &keyId);
	/**
	 * Return the child number of the key with the given id
	 *
	 * This scans the list of direct children for a key with the given
	 * key id. It returns the number in the internal list of children
	 * which is identical to the row number in the item model. Since
	 * proxy models may sort the items you should only call this function
	 * from the primary model (i.e. KGpgItemModel).
	 *
	 * The key id will be matched against the characters given in keyId.
	 * If you give only 8 or 16 byte you will still find the key if it
	 * exists. To be really sure to find the correct node you should pass
	 * the complete fingerprint whenever possible.
	 *
	 * @param keyId the key id to find, any length is permitted
	 * @return the child number or -1 if there is no such key
	 */
	int findKeyRow(const QString &keyId);

	int groupChildren() const
		{ return m_groups; }

Q_SIGNALS:
	void newKeyNode(KGpgKeyNode *);
};

class KGpgUidNode : public KGpgExpandableNode
{
private:
	KgpgKeyUid *m_uid;

protected:
	virtual void readChildren()
		{};

public:
	explicit KGpgUidNode(KGpgKeyNode *parent, const KgpgKeyUid &u);
	virtual ~KGpgUidNode()
		{ delete m_uid; }

	virtual KgpgItemType getType() const
		{ return ITYPE_UID; }
	virtual KgpgKeyTrust getTrust() const
		{ return m_uid->trust(); }
	virtual QString getName() const;
	virtual QString getEmail() const;
	virtual QString getId() const;
	virtual KGpgKeyNode *getParentKeyNode() const;
	virtual QString getComment() const
		{ return m_uid->comment(); }
	/**
	 * Return the number of signatures of this user id
	 *
	 * @return the number of signatures to this id
	 */
	virtual QString getSignCount() const;
};

class KGpgSubkeyNode : public KGpgExpandableNode
{
private:
	KgpgKeySub m_skey;

protected:
	virtual void readChildren()
		{}

public:
	explicit KGpgSubkeyNode(KGpgKeyNode *parent, const KgpgKeySub &k);
	virtual ~KGpgSubkeyNode()
		{}

	virtual KgpgItemType getType() const
		{ return ITYPE_SUB; }
	virtual KgpgKeyTrust getTrust() const
		{ return m_skey.trust(); }
	virtual QString getSize() const;
	virtual QString getName() const;
	virtual QDate getExpiration() const;
	virtual QDate getCreation() const;
	virtual QString getId() const;
	virtual KGpgKeyNode *getParentKeyNode() const;
	/**
	 * Return the number of signatures of this subkey
	 *
	 * @return the number of signatures to this subkey
	 */
	virtual QString getSignCount() const;
};

class KGpgUatNode : public KGpgExpandableNode
{
private:
	KgpgKeyUat m_uat;
	QPixmap m_pic;
	QString m_idx;

// 	void insertSigns(KGpgExpandableNode *node, const KgpgKeySignList &list);

protected:
	virtual void readChildren()
		{}

public:
	explicit KGpgUatNode(KGpgKeyNode *parent, const KgpgKeyUat &k, const QString &index);
	virtual ~KGpgUatNode()
		{}

	virtual KgpgItemType getType() const
		{ return ITYPE_UAT; }
	virtual KgpgKeyTrust getTrust() const
		{ return TRUST_NOKEY; }
	QPixmap getPixmap() const
		{ return m_pic; }
	virtual QString getId() const
		{ return m_idx; }
	virtual QString getSize() const;
	virtual QString getName() const;
	virtual QDate getCreation() const;
	virtual KGpgKeyNode *getParentKeyNode() const;
	/**
	 * Return the number of signatures of this attribute
	 *
	 * @return the number of signatures to this attribute
	 */
	virtual QString getSignCount() const;
};

class KGpgGroupNode : public KGpgExpandableNode
{
private:
	QString m_name;

protected:
	virtual void readChildren();

public:
	explicit KGpgGroupNode(KGpgRootNode *parent, const QString &name);
	explicit KGpgGroupNode(KGpgRootNode *parent, const QString &name, const KGpgKeyNodeList &members);
	virtual ~KGpgGroupNode();

	virtual KgpgItemType getType() const
		{ return ITYPE_GROUP; }
	/**
	 * Return size of group
	 *
	 * @return the number of keys in this group
	 */
	virtual QString getSize() const;
	virtual QString getName() const;
};

/**
 * class for child objects that are only a reference to a primary key
 *
 * This is the base class for all type of objects that match these criteria:
 * -they can not have child objects
 * -they are only a reference to a primary key (which needs not to be in the
 *  key ring)
 *
 * Do not create instances from this class. Use KGpgGroupMemberNode and
 * KGpgSignNode as those represent the existing objects. This class exists
 * only to get the hierarchy right.
 */
class KGpgRefNode: public KGpgNode
{
	Q_OBJECT

private:
	QString m_id;

protected:
	KGpgKeyNode *m_keynode;

	explicit KGpgRefNode(KGpgExpandableNode *parent, KGpgKeyNode *key);
	explicit KGpgRefNode(KGpgExpandableNode *parent, const QString &keyid);

public:
	virtual ~KGpgRefNode();

	virtual QString getId() const;
	virtual QString getName() const;
	virtual QString getEmail() const;
	/**
	 * Get the node of the primary key this node references to
	 *
	 * This will return the key node of the primary key this node
	 * references. This may be %NULL if the primary key is not in the key
	 * ring, e.g. if this is a signature of an unknown key.
	 *
	 * @return the node of the primary key or %NULL
	 */
	virtual KGpgKeyNode *getRefNode() const
		{ return m_keynode; }

	/**
	 * Check if the referenced key exists
	 *
	 * @return if getRefNode() will return %NULL or not
	 */
	bool isUnknown() const;

private Q_SLOTS:
	void keyUpdated(KGpgKeyNode *);
};

class KGpgGroupMemberNode : public KGpgRefNode
{
public:
	explicit KGpgGroupMemberNode(KGpgGroupNode *parent, const QString &k);
	explicit KGpgGroupMemberNode(KGpgGroupNode *parent, KGpgKeyNode *k);
	virtual ~KGpgGroupMemberNode()
		{ }

	virtual KgpgKeyTrust getTrust() const;
	virtual KgpgItemType getType() const;
	virtual QString getSize() const;
	virtual QDate getExpiration() const;
	virtual QDate getCreation() const;
	virtual KGpgGroupNode *getParentKeyNode() const;

	/**
	 * Returns the size of the signing key.
	 * @return signing key size in bits
	 */
	virtual unsigned int getSignKeySize() const;
	/**
	 * Returns the size of the first encryption subkey.
	 * @return encryption key size in bits
	 */
	virtual unsigned int getEncryptionKeySize() const;
};

class KGpgSignNode : public KGpgRefNode
{
private:
	KgpgKeySign *m_sign;

public:
	explicit KGpgSignNode(KGpgExpandableNode *parent, const KgpgKeySign &s);
	virtual ~KGpgSignNode()
		{ delete m_sign; }

	virtual KgpgItemType getType() const
		{ return ITYPE_SIGN; }
	virtual QDate getExpiration() const;
	virtual QDate getCreation() const;
	virtual QString getId() const;
	virtual QString getComment() const
		{ return m_sign->comment(); }
};

class KGpgOrphanNode : public KGpgNode
{
private:
	KgpgKey *m_key;

public:
	explicit KGpgOrphanNode(KGpgExpandableNode *parent, const KgpgKey &k);
	virtual ~KGpgOrphanNode();

	virtual KgpgItemType getType() const;
	virtual KgpgKeyTrust getTrust() const
		{ return m_key->trust(); }
	QString getKeyId() const
		{ return m_key->fullId(); }
	virtual QString getSize() const;
	virtual QString getName() const;
	virtual QString getEmail() const;
	virtual QDate getExpiration() const;
	virtual QDate getCreation() const;
	virtual QString getId() const;
};

#endif
