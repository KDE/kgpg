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
class KGpgSignableNode;
class KGpgSubkeyNode;
class KGpgUatNode;
class KGpgGroupNode;
class KGpgRefNode;
class KGpgGroupMemberNode;
class KGpgSignNode;
class KGpgOrphanNode;
class KGpgItemModel;

typedef QList<KGpgSignNode *> KGpgSignNodeList;

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

/**
 * @brief The abstract base class for all classes that may have child objects
 *
 * Every class that represents something in the keyring that may have
 * child objects inherits from this class. That does not mean that every
 * child object always has children, but every child \em may have children.
 */
class KGpgExpandableNode : public KGpgNode
{
	Q_OBJECT

	friend class KGpgRefNode;
	friend class KGpgSubkeyNode;

	KGpgExpandableNode(); // = delete C++0x
protected:
	List children;

	/**
	 * reimplemented in every base class to read in the child data
	 *
	 * This allows the child objects to delay the loading of the
	 * child objects until they are really needed to avoid time
	 * consuming operations for data never used.
	 */
	virtual void readChildren() = 0;

	explicit KGpgExpandableNode(KGpgExpandableNode *parent = NULL);
public:
	virtual ~KGpgExpandableNode();

	/**
	 * check if there are any child nodes
	 *
	 * The default implementation returns true if any child nodes were loaded.
	 * This may be reimplemented by child classes so they can indicate that
	 * there are child nodes before actually loading them.
	 *
	 * This method indicates if there are children if this node is expanded.
	 * In contrast wasExpanded() will only return true if the child nodes
	 * are actually present in memory.
	 */
	virtual bool hasChildren() const
		{ return (children.count() != 0); }
	/**
	 * check if there are any child nodes present in memory
	 *
	 * Returns true if any child nodes were loaded.
	 *
	 * This method indicates if the children of this node are already loaded
	 * into memory. In contrast hasChildren() may return true even if the child
	 * objects are not present in memory.
	 */
	virtual bool wasExpanded() const
		{ return (children.count() != 0); }
	virtual int getChildCount();
	virtual const List &getChildren() const
		{ return children; }
	virtual KGpgNode *getChild(const int &index) const;
	virtual int getChildIndex(KGpgNode *node) const
		{ return children.indexOf(node); }
	virtual void deleteChild(KGpgNode *child)
		{ children.removeAll(child); }
};

/**
 * @brief An object that may have KGpgSignNode children
 *
 * This class represents an object that may be signed, i.e. key nodes,
 * user ids, user attributes, and subkeys.
 */
class KGpgSignableNode : public KGpgExpandableNode
{
	Q_OBJECT

public:
	KGpgSignableNode(KGpgExpandableNode *parent = NULL);
	virtual ~KGpgSignableNode();

	KGpgSignNodeList getSignatures(void) const;
	/**
	 * @brief count signatures
	 * @return the number of signatures to this object
	 *
	 * This does not include the number of signatures to child objects.
	 */
	virtual QString getSignCount() const;

	bool operator<(const KGpgSignableNode &other) const;
	bool operator<(const KGpgSignableNode *other) const;
};

/**
 * @brief A public key with or without corresponding secret key
 */
class KGpgKeyNode : public KGpgSignableNode
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
	QList<KGpgRefNode *> getRefsOfType(const KgpgItemType &type) const;

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
	 * @brief Print the full key fingerprint with spaces inserted
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
	 * @brief Return the number of signatures of the primary user id
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
	 * @brief Creates a copy of the KgpgKey that belongs to this class
	 */
	virtual KgpgKey *copyKey() const;
	/**
	 * @brief Replaces the current key information with the new one.
	 * All sub-items (i.e. signatures, user ids ...) will be deleted. This must
	 * only be used when the id of both new and old key is the same.
	 */
	void setKey(const KgpgKey &key);
	/**
	 * @brief Returns a reference to the key used in this object.
	 * This allows direct access to the values of the key e.g. for KgpgKeyInfo.
	 */
	const KgpgKey *getKey() const;

	/**
	 * @brief Returns the size of the signing key.
	 * @return signing key size in bits
	 */
	virtual unsigned int getSignKeySize() const;
	/**
	 * @brief Returns the size of the first encryption subkey.
	 * @return encryption key size in bits
	 */
	virtual unsigned int getEncryptionKeySize() const;
	/**
	 * @brief Notify this key that a KGpgRefNode now references this key.
	 * @param node object that takes the reference
	 */
	void addRef(KGpgRefNode *node);
	/**
	 * @brief Remove a reference to this object
	 * @param node node that no longer has the reference
	 *
	 * Note that this must not be called as reply when this object
	 * emits updated(NULL)
	 */
	void delRef(KGpgRefNode *node);
	/**
	 * @brief returns a list of all groups this key is member of
	 */
	QList<KGpgGroupNode *> getGroups(void) const;
	/**
	 * @brief returns a list of all group member nodes that reference this key
	 */
	QList<KGpgGroupMemberNode *> getGroupRefs(void) const;
	/**
	 * @brief returns a list of all sign nodes that reference this key
	 */
	KGpgSignNodeList getSignRefs(void) const;
	/**
	 * @brief returns a list of signatures to this key
	 * @param subkeys if signatures on subkeys should be included
	 */
	KGpgSignNodeList getSignatures(const bool subkeys) const;
	/**
	 * @brief get the user id or user attribute with the given number
	 * @param index the index of the user id to return
	 * @return the requested subitem or NULL if that is not present
	 *
	 * User ids indexes are 1-based, so 0 is not a valid index. Passing
	 * 1 as index will return the object itself, representing the primary
	 * user id.
	 */
	const KGpgSignableNode *getUid(const unsigned int index) const;

Q_SIGNALS:
	void updated(KGpgKeyNode *);
};

typedef QList<KGpgKeyNode *> KGpgKeyNodeList;
typedef QList<const KGpgKeyNode *> KGpgKeyConstNodeList;

/**
 * @brief The parent of all key data objects
 *
 * This object is invisible to the user but acts as the internal base object for
 * everything in the keyring. It is anchestor of all other KGpgNode objects and
 * the only one that will ever return NULL when calling getParentKeyNode() on it.
 *
 * There is only one object of this type around at any time.
 */
class KGpgRootNode : public KGpgExpandableNode
{
	Q_OBJECT

	friend class KGpgGroupNode;

private:
	int m_groups;
	int m_deleting;

protected:
	virtual void readChildren()
		{};

public:
	explicit KGpgRootNode(KGpgItemModel *model);
	virtual ~KGpgRootNode();

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

	/**
	 * Return the child number of the given key
	 * @param key the key to search for
	 *
	 * @overload
	 */
	int findKeyRow(const KGpgKeyNode *key);

	int groupChildren() const
		{ return m_groups; }

	/**
	 * Return a pointer to this object or NULL
	 *
	 * This returns a pointer to this object if the object will persist,
	 * i.e. is not currently in destruction. If the object is already
	 * cleaning up NULL is returned.
	 */
	KGpgRootNode *asRootNode();
	const KGpgRootNode *asRootNode() const;

Q_SIGNALS:
	void newKeyNode(KGpgKeyNode *);
};

/**
 * @brief A user id of a public key or key pair
 */
class KGpgUidNode : public KGpgSignableNode
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
};

/**
 * @brief a subkey of a public key or key pair
 */
class KGpgSubkeyNode : public KGpgSignableNode
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
};

/**
 * @brief A user attribute (i.e. photo id) of a public key or key pair
 */
class KGpgUatNode : public KGpgSignableNode
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
};

/**
 * @brief A GnuPG group of public keys
 */
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
 * @brief Class for child objects that are only a reference to a primary key
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
	bool m_selfsig;		///< if this is a reference to it's own parent

protected:
	KGpgKeyNode *m_keynode;

	explicit KGpgRefNode(KGpgExpandableNode *parent, KGpgKeyNode *key);
	explicit KGpgRefNode(KGpgExpandableNode *parent, const QString &keyid);

	KGpgRootNode *getRootNode() const;

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

/**
 * @brief A member of a GnuPG group
 */
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

/**
 * @brief A signature to another key object
 */
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
	virtual QString getName() const;
	virtual QDate getCreation() const;
	virtual QString getId() const;
	virtual QString getComment() const
		{ return m_sign->comment(); }
};

/**
 * @brief A lone secret key without public key
 */
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
