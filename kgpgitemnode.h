#ifndef KGPGITEMNODE_H
#define KGPGITEMNODE_H

#include <QVariant>
#include <QPixmap>
#include "kgpgkey.h"

using namespace KgpgCore;

class KGpgNode : public QObject
{
	Q_OBJECT

	friend class KGpgItemModel;

protected:
	KGpgNode *m_parent;
public:
	explicit KGpgNode(KGpgNode *parent = 0);
	virtual ~KGpgNode();

	virtual KgpgItemType getType() const = 0;
	virtual QVariant getData(const int &column) const = 0;
	virtual KgpgKeyTrust getTrust() const
		{ return TRUST_NOKEY; }
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

	virtual int getChildCount();
	virtual KGpgNode *getChild(const int &index) const
		{ return children.at(index); }
	virtual int getChildIndex(KGpgNode *node) const
		{
			return children.indexOf(node);
		}
};

class KGpgKeyNode : public KGpgExpandableNode
{
private:
	KgpgKey *m_key;

	void insertSigns(KGpgExpandableNode *node, const KgpgKeySignList &list);

protected:
	virtual void readChildren();

public:
	explicit KGpgKeyNode(KGpgExpandableNode *parent, const KgpgKey &k);
	virtual ~KGpgKeyNode();

	virtual KgpgItemType getType() const;
	virtual QVariant getData(const int &column) const;
	virtual KgpgKeyTrust getTrust() const
		{ return m_key->trust(); }
	QString getKeyId() const
		{ return m_key->fullId(); }

	static KgpgItemType getType(const KgpgKey *k);
};

class KGpgRootNode : public KGpgExpandableNode
{
protected:
	virtual void readChildren()
		{};

public:
	explicit KGpgRootNode();
	virtual ~KGpgRootNode()
		{}

	virtual KgpgItemType getType() const
		{ return 0; }
	virtual QVariant getData(const int &column) const
		{
			Q_UNUSED(column)
			return QVariant();
		}

	unsigned int addGroups();
	unsigned int addKeys();
	KGpgKeyNode *findKey(const QString &keyId);
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
	virtual QVariant getData(const int &column) const;
	virtual KgpgKeyTrust getTrust() const
		{ return m_uid->trust(); }
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
	virtual QVariant getData(const int &column) const;
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
	virtual QVariant getData(const int &column) const;
};

class KGpgUatNode : public KGpgExpandableNode
{
private:
	KgpgKeyUat m_uat;
	QPixmap m_pic;

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
	virtual QVariant getData(const int &column) const;
	virtual KgpgKeyTrust getTrust() const
		{ return TRUST_NOKEY; }
	QPixmap getPixmap() const
		{ return m_pic; }
};

class KGpgGroupNode : public KGpgExpandableNode
{
private:
	QString m_name;

protected:
	virtual void readChildren();

public:
	explicit KGpgGroupNode(KGpgRootNode *parent, const QString &name);
	virtual ~KGpgGroupNode()
		{}

	virtual KgpgItemType getType() const
		{ return ITYPE_GROUP; }
	virtual QVariant getData(const int &column) const;
};

class KGpgGroupMemberNode : public KGpgNode
{
private:
	// this is not what we want. We want a reference to the KGpgNode of that key id.
	KgpgKey *m_key;

public:
	explicit KGpgGroupMemberNode(KGpgGroupNode *parent, const QString &k);
	virtual ~KGpgGroupMemberNode()
		{ delete m_key; }

	virtual KgpgKeyTrust getTrust() const
		{ return m_key->trust(); }
	virtual KgpgItemType getType() const
		{ return KGpgKeyNode::getType(m_key) | ITYPE_GROUP; }
	virtual QVariant getData(const int &column) const;
};

#endif
