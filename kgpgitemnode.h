#ifndef KGPGITEMNODE_H
#define KGPGITEMNODE_H

#include <QVariant>
#include <QPixmap>
#include "kgpgkey.h"

using namespace KgpgCore;

class KGpgExpandableNode;
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
			return NULL;
		}

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
	KGpgExpandableNode *getParentKeyNode() const
		{ return m_parent; }
};

class KGpgExpandableNode : public KGpgNode
{
	friend class KGpgSignNode;
	friend class KGpgGroupMemberNode;
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
	friend class KGpgGroupMemberNode;

private:
	KgpgKey *m_key;

	void insertSigns(KGpgExpandableNode *node, const KgpgKeySignList &list);

protected:
	virtual void readChildren();

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
};

typedef QList<KGpgKeyNode *> KGpgKeyNodeList;

class KGpgRootNode : public KGpgExpandableNode
{
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
	KGpgKeyNode *findKey(const QString &keyId);
	int findKeyRow(const QString &keyId);

	int groupChildren() const
		{ return m_groups; }
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
	virtual QString getSize() const;
	virtual QString getName() const;
	virtual QString getEmail() const;
	virtual QString getId() const;
	virtual KGpgKeyNode *getParentKeyNode() const
		{ return static_cast<KGpgKeyNode *>(m_parent); }
};

class KGpgSignNode : public KGpgNode
{
private:
	KgpgKeySign *m_sign;

public:
	explicit KGpgSignNode(KGpgExpandableNode *parent, const KgpgKeySign &s);
	virtual ~KGpgSignNode()
		{ delete m_sign; }

	virtual KgpgItemType getType() const
		{ return ITYPE_SIGN; }
	virtual QString getName() const;
	virtual QString getEmail() const;
	virtual QDate getExpiration() const;
	virtual QDate getCreation() const;
	virtual QString getId() const;

	bool isUnknown() const;
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
	virtual KGpgKeyNode *getParentKeyNode() const
		{ return static_cast<KGpgKeyNode *>(m_parent); }
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
	virtual KGpgKeyNode *getParentKeyNode() const
		{ return static_cast<KGpgKeyNode *>(m_parent); }
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
	virtual QString getSize() const;
	virtual QString getName() const;
};

class KGpgGroupMemberNode : public KGpgNode
{
private:
	// this is not what we want. We want a reference to the KGpgNode of that key id.
	KgpgKey *m_key;

public:
	explicit KGpgGroupMemberNode(KGpgGroupNode *parent, const QString &k);
	explicit KGpgGroupMemberNode(KGpgGroupNode *parent, const KGpgKeyNode *k);
	virtual ~KGpgGroupMemberNode()
		{ delete m_key; }

	virtual KgpgKeyTrust getTrust() const
		{ return m_key->trust(); }
	virtual KgpgItemType getType() const
		{ return KGpgKeyNode::getType(m_key) | ITYPE_GROUP; }
	virtual QString getSize() const;
	virtual QString getName() const;
	virtual QString getEmail() const;
	virtual QDate getExpiration() const;
	virtual QDate getCreation() const;
	virtual QString getId() const;
	virtual KGpgGroupNode *getParentKeyNode() const
		{ return static_cast<KGpgGroupNode *>(m_parent); }
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
