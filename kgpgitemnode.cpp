#include <KDebug>
#include <KLocale>
#include "kgpgitemnode.h"
#include "kgpginterface.h"
#include "kgpgsettings.h"
#include "convert.h"

KGpgNode::KGpgNode(KGpgNode *parent)
	: QObject(), m_parent(parent)
{
}

KGpgNode::~KGpgNode()
{
}

KGpgExpandableNode::KGpgExpandableNode(KGpgExpandableNode *parent)
	: KGpgNode(parent)
{
	if (parent != NULL)
		parent->children.append(this);
}

KGpgExpandableNode::~KGpgExpandableNode()
{
	for (int i = 0; i < children.count(); i++)
		delete children[i];
}

int
KGpgExpandableNode::getChildCount()
{
	if (children.count() == 0)
		readChildren();

	return children.count();
}

KGpgRootNode::KGpgRootNode()
	: KGpgExpandableNode(NULL)
{
	addGroups();
}

unsigned int
KGpgRootNode::addGroups()
{
	QStringList groups = KgpgInterface::getGpgGroupNames(KGpgSettings::gpgConfigPath());

	for (QStringList::Iterator it = groups.begin(); it != groups.end(); ++it)
		new KGpgGroupNode(this, QString(*it));

	return groups.count();
}

void
KGpgRootNode::addKeys()
{
	KgpgInterface *interface = new KgpgInterface();

	KgpgKeyList publiclist = interface->readPublicKeys(true);

	KgpgKeyList secretlist = interface->readSecretKeys();
	QStringList issec = secretlist;

	delete interface;

	for (int i = 0; i < publiclist.size(); ++i)
	{
		KgpgKey key = publiclist.at(i);

		int index = issec.indexOf(key.fullId());
		if (index != -1)
		{
			key.setSecret(true);
			issec.removeAt(index);
			secretlist.removeAt(index);
		}

		new KGpgKeyNode(this, key);
	}

	for (int i = 0; i < secretlist.count(); ++i)
	{
		KgpgKey key = secretlist.at(i);

		new KGpgOrphanNode(this, key);
	}
}

KGpgKeyNode *
KGpgRootNode::findKey(const QString &keyId)
{
	for (int i = 0; i < children.count(); i++) {
		KGpgNode *node = children[i];
		if ((node->getType() & ITYPE_PAIR) == 0)
			continue;

		KGpgKeyNode *key = static_cast<KGpgKeyNode *>(node);

		if (keyId == key->getKeyId().mid(0, keyId.length()))
			return key;
	}

	return NULL;
}

KGpgKeyNode::KGpgKeyNode(KGpgExpandableNode *parent, const KgpgKey &k)
	: KGpgExpandableNode(parent), m_key(new KgpgKey(k))
{
}

KGpgKeyNode::~KGpgKeyNode()
{
}

KgpgItemType
KGpgKeyNode::getType() const
{
	return getType(m_key);
}

KgpgItemType
KGpgKeyNode::getType(const KgpgKey *k)
{
	if (k->secret())
		return ITYPE_PAIR;

	return ITYPE_PUBLIC;
}

QString
KGpgKeyNode::getSize() const
{
	return QString::number(m_key->size());
}

QString
KGpgKeyNode::getName() const
{
	return m_key->name();
}

QString
KGpgKeyNode::getEmail() const
{
	return m_key->email();
}

QDate
KGpgKeyNode::getExpiration() const
{
	return m_key->expirationDate();
}

QDate
KGpgKeyNode::getCreation() const
{
	return m_key->creationDate();
}

QString
KGpgKeyNode::getId() const
{
	return m_key->fullId();
}

void
KGpgKeyNode::readChildren()
{
	KgpgInterface *interface = new KgpgInterface();
	KgpgKeyList keys = interface->readPublicKeys(true, m_key->fullId(), true);
	KgpgKey key = keys.at(0);

	/********* insertion of sub keys ********/
	for (int i = 0; i < key.subList()->size(); ++i)
	{
		KgpgKeySub sub = key.subList()->at(i);

		KGpgSubkeyNode *n = new KGpgSubkeyNode(this, sub);
		insertSigns(n, sub.signList());
	}

	/********* insertion of users id ********/
	for (int i = 0; i < key.uidList()->size(); ++i)
	{
		KgpgKeyUid uid = key.uidList()->at(i);

		KGpgUidNode *n = new KGpgUidNode(this, uid);
		insertSigns(n, uid.signList());
	}

	/******** insertion of photos id ********/
	QStringList photolist = key.photoList();
	for (int i = 0; i < photolist.size(); ++i)
	{
		KgpgKeyUat uat = key.uatList()->at(i);

		KGpgUatNode *n = new KGpgUatNode(this, uat, photolist.at(i));
		insertSigns(n, uat.signList());
	}
	/****************************************/

	delete interface;

	/******** insertion of signature ********/
	insertSigns(this, key.signList());
}

void KGpgKeyNode::insertSigns(KGpgExpandableNode *node, const KgpgKeySignList &list)
{
	for (int i = 0; i < list.size(); ++i)
	{
		(void) new KGpgSignNode(node, list.at(i));
	}
}

KGpgUidNode::KGpgUidNode(KGpgKeyNode *parent, const KgpgKeyUid &u)
	: KGpgExpandableNode(parent), m_uid(new KgpgKeyUid(u))
{
}

QString
KGpgUidNode::getName() const
{
	return m_uid->name();
}

QString
KGpgUidNode::getEmail() const
{
	return m_uid->email();
}

QString
KGpgUidNode::getSize() const
{
	return i18np("1 signature", "%1 signatures", children.count());
}

KGpgSignNode::KGpgSignNode(KGpgExpandableNode *parent, const KgpgKeySign &s)
	: KGpgNode(parent), m_sign(new KgpgKeySign(s))
{
	Q_ASSERT(parent != NULL);
	parent->children.append(this);
}

QString
KGpgSignNode::getName() const
{
	return m_sign->name();
}

QString
KGpgSignNode::getEmail() const
{
	return m_sign->email();
}

QDate
KGpgSignNode::getExpiration() const
{
	return m_sign->expirationDate();
}

QDate
KGpgSignNode::getCreation() const
{
	return m_sign->creationDate();
}

QString
KGpgSignNode::getId() const
{
	return m_sign->fullId();
}

KGpgSubkeyNode::KGpgSubkeyNode(KGpgKeyNode *parent, const KgpgKeySub &k)
	: KGpgExpandableNode(parent), m_skey(k)
{
	Q_ASSERT(parent != NULL);
}

QDate
KGpgSubkeyNode::getExpiration() const
{
	return m_skey.expirationDate();
}

QDate
KGpgSubkeyNode::getCreation() const
{
	return m_skey.creationDate();
}

QString
KGpgSubkeyNode::getId() const
{
	return m_skey.id();
}

QString
KGpgSubkeyNode::getName() const
{
	return i18n("%1 subkey", Convert::toString(m_skey.algorithm()));
}

QString
KGpgSubkeyNode::getSize() const
{
	return QString::number(m_skey.size());
}

KGpgUatNode::KGpgUatNode(KGpgKeyNode *parent, const KgpgKeyUat &k, const QString &index)
	: KGpgExpandableNode(parent), m_uat(k)
{
	KgpgInterface iface;

	m_pic = iface.loadPhoto(parent->getKeyId(), index, true);
}

QString
KGpgUatNode::getName() const
{
	return i18n("Photo id");
}

QString
KGpgUatNode::getSize() const
{
	return QString::number(m_pic.width()) + "x" + QString::number(m_pic.height());
}

QDate
KGpgUatNode::getCreation() const
{
	return m_uat.creationDate();
}

KGpgGroupNode::KGpgGroupNode(KGpgRootNode *parent, const QString &name)
	: KGpgExpandableNode(parent), m_name(name)
{
	readChildren();
}

QString
KGpgGroupNode::getName() const
{
	return m_name;
}

QString
KGpgGroupNode::getSize() const
{
	return i18np("1 key", "%1 keys", children.count());
}

void
KGpgGroupNode::readChildren()
{
	QStringList keys = KgpgInterface::getGpgGroupSetting(m_name, KGpgSettings::gpgConfigPath());

	children.clear();

	for (QStringList::Iterator it = keys.begin(); it != keys.end(); ++it)
		new KGpgGroupMemberNode(this, QString(*it));
}

KGpgGroupMemberNode::KGpgGroupMemberNode(KGpgGroupNode *parent, const QString &k)
	: KGpgNode(parent)
{
	KgpgInterface *iface = new KgpgInterface();
	KgpgKeyList l = iface->readPublicKeys(true, k);
	delete iface;

	m_key = new KgpgKey(l.at(0));

	parent->children.append(this);
}

QString
KGpgGroupMemberNode::getName() const
{
	return m_key->name();
}

QString
KGpgGroupMemberNode::getEmail() const
{
	return m_key->email();
}

QString
KGpgGroupMemberNode::getSize() const
{
	return QString::number(m_key->size());
}

QDate
KGpgGroupMemberNode::getExpiration() const
{
	return m_key->expirationDate();
}

QDate
KGpgGroupMemberNode::getCreation() const
{
	return m_key->creationDate();
}

QString
KGpgGroupMemberNode::getId() const
{
	return m_key->fullId();
}

KGpgOrphanNode::KGpgOrphanNode(KGpgExpandableNode *parent, const KgpgKey &k)
	: KGpgNode(parent), m_key(new KgpgKey(k))
{
}

KGpgOrphanNode::~KGpgOrphanNode()
{
	delete m_key;
}

KgpgItemType
KGpgOrphanNode::getType() const
{
	return ITYPE_SECRET;
}

QString
KGpgOrphanNode::getName() const
{
	return m_key->name();
}

QString
KGpgOrphanNode::getEmail() const
{
	return m_key->email();
}

QString
KGpgOrphanNode::getSize() const
{
	return QString::number(m_key->size());
}

QDate
KGpgOrphanNode::getExpiration() const
{
	return m_key->expirationDate();
}

QDate
KGpgOrphanNode::getCreation() const
{
	return m_key->creationDate();
}

QString
KGpgOrphanNode::getId() const
{
	return m_key->fullId();
}

