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
#include "kgpgitemnode.h"

#include <KLocale>
#include "kgpginterface.h"
#include "kgpgsettings.h"
#include "convert.h"
#include "kgpgitemmodel.h"

KGpgNode::KGpgNode(KGpgExpandableNode *parent)
	: QObject(), m_parent(parent)
{
	if (parent == NULL)
		m_model = NULL;
	else
		m_model = parent->m_model;
}

KGpgNode::~KGpgNode()
{
	Q_ASSERT(m_model);
	m_model->invalidateIndexes(this);

	if (m_parent != NULL)
		m_parent->deleteChild(this);
}

QString
KGpgNode::getNameComment() const
{
	if (getComment().isEmpty())
		return getName();
	else
		return i18nc("Name of uid (comment)", "%1 (%2)", getName(), getComment());
}

KGpgExpandableNode *
KGpgNode::toExpandableNode()
{
	Q_ASSERT(((getType() & ITYPE_GROUP) && !(getType() & ITYPE_PAIR)) ||
			(getType() & (ITYPE_PAIR | ITYPE_SUB | ITYPE_UID | ITYPE_UAT)));

	return static_cast<KGpgExpandableNode *>(this);
}

KGpgKeyNode *
KGpgNode::toKeyNode()
{
	Q_ASSERT(getType() & ITYPE_PAIR);
	Q_ASSERT(!(getType() & ITYPE_GROUP));

	return static_cast<KGpgKeyNode *>(this);
}

KGpgRootNode *
KGpgNode::toRootNode()
{
	Q_ASSERT(m_parent == NULL);

	return static_cast<KGpgRootNode *>(this);
}

KGpgUidNode *
KGpgNode::toUidNode()
{
	Q_ASSERT(getType() & ITYPE_UID);

	return static_cast<KGpgUidNode *>(this);
}

KGpgSubkeyNode *
KGpgNode::toSubkeyNode()
{
	Q_ASSERT(getType() & ITYPE_SUB);

	return static_cast<KGpgSubkeyNode *>(this);
}

KGpgUatNode *
KGpgNode::toUatNode()
{
	Q_ASSERT(getType() & ITYPE_UAT);

	return static_cast<KGpgUatNode *>(this);
}

KGpgGroupNode *
KGpgNode::toGroupNode()
{
	Q_ASSERT((getType() & ITYPE_GROUP) && !(getType() & ITYPE_PAIR));

	return static_cast<KGpgGroupNode *>(this);
}

KGpgRefNode *
KGpgNode::toRefNode()
{
	Q_ASSERT(((getType() & ITYPE_GROUP) && (getType() & ITYPE_PAIR)) || (getType() & ITYPE_SIGN));

	return static_cast<KGpgRefNode *>(this);
}

KGpgGroupMemberNode *
KGpgNode::toGroupMemberNode()
{
	Q_ASSERT((getType() & ITYPE_GROUP) && (getType() & ITYPE_PAIR));

	return static_cast<KGpgGroupMemberNode *>(this);
}

KGpgSignNode *
KGpgNode::toSignNode()
{
	Q_ASSERT(getType() & ITYPE_SIGN);

	return static_cast<KGpgSignNode *>(this);
}

KGpgOrphanNode *
KGpgNode::toOrphanNode()
{
	Q_ASSERT((getType() & ITYPE_SECRET) && !(getType() & ITYPE_GPUBLIC));

	return static_cast<KGpgOrphanNode *>(this);
}

KGpgExpandableNode::KGpgExpandableNode(KGpgExpandableNode *parent)
	: KGpgNode(parent)
{
	if (parent != NULL)
		parent->children.append(this);
}

KGpgExpandableNode::~KGpgExpandableNode()
{
	for (int i = children.count() - 1; i >= 0; i--)
		delete children[i];
}

KGpgNode *
KGpgExpandableNode::getChild(const int &index) const
{
	if ((index < 0) || (index > children.count()))
		return NULL;
	return children.at(index);
}

int
KGpgExpandableNode::getChildCount()
{
	if (children.count() == 0)
		readChildren();

	return children.count();
}

KGpgRootNode::KGpgRootNode(KGpgItemModel *model)
	: KGpgExpandableNode(NULL), m_groups(0)
{
	m_model = model;
	addGroups();
}

void
KGpgRootNode::addGroups()
{
	const QStringList groups = KgpgInterface::getGpgGroupNames(KGpgSettings::gpgConfigPath());

	for (QStringList::const_iterator it = groups.begin(); it != groups.end(); ++it)
		new KGpgGroupNode(this, QString(*it));
}

void
KGpgRootNode::addKeys(const QStringList &ids)
{
	KgpgInterface *interface = new KgpgInterface();

	KgpgKeyList publiclist = interface->readPublicKeys(true, ids);
	KgpgKeyList secretlist = interface->readSecretKeys();

	delete interface;

	QStringList issec = secretlist;

	for (int i = 0; i < publiclist.size(); ++i) {
		KgpgKey key = publiclist.at(i);

		int index = issec.indexOf(key.fullId());
		if (index != -1) {
			key.setSecret(true);
			issec.removeAt(index);
			secretlist.removeAt(index);
		}

		KGpgKeyNode *nd = new KGpgKeyNode(this, key);
		emit newKeyNode(nd);
	}

	for (int i = 0; i < secretlist.count(); ++i) {
		KgpgKey key = secretlist.at(i);

		new KGpgOrphanNode(this, key);
	}
}

void
KGpgRootNode::refreshKeys(KGpgKeyNodeList nodes)
{
	QStringList ids;

	for (int i = 0; i < nodes.count(); i++)
		ids << nodes.at(i)->getId();

	KgpgInterface *interface = new KgpgInterface();

	KgpgKeyList publiclist = interface->readPublicKeys(true, ids);
	QStringList issec = interface->readSecretKeys(ids);

	delete interface;

	for (int i = 0; i < publiclist.size(); ++i) {
		KgpgKey key = publiclist.at(i);

		int index = issec.indexOf(key.fullId());
		if (index != -1) {
			key.setSecret(true);
			issec.removeAt(index);
		}

		for (int j = 0; j < nodes.count(); j++) {
			KGpgKeyNode *nd = nodes.at(j);

			if (nd->getId() == key.fingerprint()) {
				nodes.removeAt(j);
				nd->setKey(key);
				break;
			}
		}
	}
}

KGpgKeyNode *
KGpgRootNode::findKey(const QString &keyId)
{
	int i = findKeyRow(keyId);
	if (i >= 0) {
		return children[i]->toKeyNode();
	}

	return NULL;
}

int
KGpgRootNode::findKeyRow(const QString &keyId)
{
	for (int i = 0; i < children.count(); i++) {
		KGpgNode *node = children[i];
		if ((node->getType() & ITYPE_PAIR) == 0)
			continue;

		KGpgKeyNode *key = node->toKeyNode();

		if (keyId == key->getId().right(keyId.length()))
			return i;
	}
	return -1;
}

KGpgKeyNode::KGpgKeyNode(KGpgExpandableNode *parent, const KgpgKey &k)
	: KGpgExpandableNode(parent), m_key(new KgpgKey(k))
{
	m_signs = 0;
}

KGpgKeyNode::~KGpgKeyNode()
{
	emit updated(NULL);
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
	return i18nc("size of signing key / size of encryption key",
			"%1 / %2", QString::number(getSignKeySize()),
			QString::number(getEncryptionKeySize()));
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
	return m_key->fingerprint();
}

void
KGpgKeyNode::readChildren()
{
	KgpgInterface *interface = new KgpgInterface();
	KgpgKeyList keys = interface->readPublicKeys(true, m_key->fingerprint(), true);
	delete interface;

	if (keys.count() == 0)
		return;
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

	/******** insertion of signature ********/
	insertSigns(this, key.signList());

	m_signs = key.signList().size();
}

void KGpgKeyNode::insertSigns(KGpgExpandableNode *node, const KgpgKeySignList &list)
{
	for (int i = 0; i < list.size(); ++i)
	{
		(void) new KGpgSignNode(node, list.at(i));
	}
}

QString KGpgKeyNode::getSignCount() const
{
	if (!wasExpanded())
		return QString();
	return i18np("1 signature", "%1 signatures", m_signs);
}

KgpgKey *KGpgKeyNode::copyKey() const
{
	return new KgpgKey(*m_key);
}

void KGpgKeyNode::setKey(const KgpgKey &key)
{
	Q_ASSERT(m_key->fingerprint() == key.fingerprint());
	delete m_key;

	for (int i = 0; i < children.count(); i++)
		delete children.at(i);
	children.clear();

	m_key = new KgpgKey(key);
}

const KgpgKey *KGpgKeyNode::getKey() const
{
	return m_key;
}

unsigned int
KGpgKeyNode::getSignKeySize() const
{
	return m_key->size();
}

unsigned int
KGpgKeyNode::getEncryptionKeySize() const
{
	return m_key->encryptionSize();
}

void
KGpgKeyNode::addRef(KGpgRefNode *node)
{
	Q_ASSERT(m_refs.indexOf(node) == -1);
	m_refs.append(node);
}

void
KGpgKeyNode::delRef(KGpgRefNode *node)
{
	Q_ASSERT(m_refs.indexOf(node) != -1);
	m_refs.removeOne(node);
}

QList <KGpgGroupNode *>
KGpgKeyNode::getGroups(void) const
{
	QList<KGpgGroupNode *> ret;
	QList<KGpgGroupMemberNode *> gnodes = getGroupRefs();

	for (int i = 0; i < gnodes.count(); i++)
		ret.append(gnodes.at(i)->getParentKeyNode());
	return ret;
}

QList <KGpgGroupMemberNode *>
KGpgKeyNode::getGroupRefs(void) const
{
	QList<KGpgGroupMemberNode *> ret;

	for (int i = 0; i < m_refs.count(); i++) {
		KGpgRefNode *nd = m_refs.at(i);
		if (nd->getType() & ITYPE_GROUP)
			ret.append(nd->toGroupMemberNode());
	}
	return ret;
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
KGpgUidNode::getSignCount() const
{
	return i18np("1 signature", "%1 signatures", children.count());
}

QString
KGpgUidNode::getId() const
{
	return QString::number(m_uid->index());
}

KGpgKeyNode *
KGpgUidNode::getParentKeyNode() const
{
	return m_parent->toKeyNode();
}

KGpgSignNode::KGpgSignNode(KGpgExpandableNode *parent, const KgpgKeySign &s)
	: KGpgRefNode(parent, s.fullId()), m_sign(new KgpgKeySign(s))
{
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

QString
KGpgSubkeyNode::getSignCount() const
{
	return i18np("1 signature", "%1 signatures", children.count());
}

KGpgKeyNode *
KGpgSubkeyNode::getParentKeyNode() const
{
	return m_parent->toKeyNode();
}

KGpgUatNode::KGpgUatNode(KGpgKeyNode *parent, const KgpgKeyUat &k, const QString &index)
	: KGpgExpandableNode(parent), m_uat(k), m_idx(index)
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
	return QString::number(m_pic.width()) + 'x' + QString::number(m_pic.height());
}

QDate
KGpgUatNode::getCreation() const
{
	return m_uat.creationDate();
}

QString
KGpgUatNode::getSignCount() const
{
	return i18np("1 signature", "%1 signatures", children.count());
}

KGpgKeyNode *
KGpgUatNode::getParentKeyNode() const
{
	return m_parent->toKeyNode();
}

KGpgGroupNode::KGpgGroupNode(KGpgRootNode *parent, const QString &name)
	: KGpgExpandableNode(parent), m_name(name)
{
	readChildren();
	parent->m_groups++;
}

KGpgGroupNode::KGpgGroupNode(KGpgRootNode *parent, const QString &name, const KGpgKeyNodeList &members)
	: KGpgExpandableNode(parent), m_name(name)
{
	Q_ASSERT(members.count() > 0);
	for (int i = 0; i < members.count(); i++)
		new KGpgGroupMemberNode(this, members.at(i));
	parent->m_groups++;
}

KGpgGroupNode::~KGpgGroupNode()
{
	m_parent->toRootNode()->m_groups--;
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
	const QStringList keys = KgpgInterface::getGpgGroupSetting(m_name, KGpgSettings::gpgConfigPath());

	children.clear();

	for (QStringList::const_iterator it = keys.begin(); it != keys.end(); ++it)
		new KGpgGroupMemberNode(this, QString(*it));
}

KGpgRefNode::KGpgRefNode(KGpgExpandableNode *parent, const QString &keyid)
	: KGpgNode(parent)
{
	KGpgExpandableNode *pt = parent->getParentKeyNode();
	while (pt->getParentKeyNode() != NULL)
		pt = pt->getParentKeyNode();

	KGpgRootNode *root = pt->toRootNode();

	m_keynode = root->findKey(keyid);
	if (m_keynode != NULL) {
		connect(m_keynode, SIGNAL(updated(KGpgKeyNode *)), this, SLOT(keyUpdated(KGpgKeyNode *)));
		m_keynode->addRef(this);
	} else {
		m_id = keyid;
		connect(root, SIGNAL(newKeyNode(KGpgKeyNode *)), this, SLOT(keyUpdated(KGpgKeyNode *)));
	}

	parent->children.append(this);
}

KGpgRefNode::~KGpgRefNode()
{
	if (m_keynode)
		m_keynode->delRef(this);
}

KGpgRefNode::KGpgRefNode(KGpgExpandableNode *parent, KGpgKeyNode *key)
	: KGpgNode(parent)
{
	Q_ASSERT(key != NULL);
	Q_ASSERT(parent != NULL);
	m_keynode = key;
	connect(m_keynode, SIGNAL(updated(KGpgKeyNode *)), SLOT(keyUpdated(KGpgKeyNode *)));
	m_keynode->addRef(this);

	parent->children.append(this);
}

void
KGpgRefNode::keyUpdated(KGpgKeyNode *nkey)
{
	KGpgExpandableNode *pt = m_parent->getParentKeyNode();
	while (pt->getParentKeyNode() != NULL)
		pt = pt->getParentKeyNode();

	KGpgRootNode *root = pt->toRootNode();

	if (nkey == NULL) {
		m_id = m_keynode->getId();
		disconnect(this, SLOT(keyUpdated(KGpgKeyNode *)));
		connect(root, SIGNAL(newKeyNode(KGpgKeyNode *)), this, SLOT(keyUpdated(KGpgKeyNode *)));
		m_keynode = NULL;
	} else if ((m_keynode == NULL) && (nkey->getId().right(m_id.length()) == m_id)) {
		m_id.clear();
		disconnect(this, SLOT(keyUpdated(KGpgKeyNode *)));
		connect(nkey, SIGNAL(updated(KGpgKeyNode *)), this, SLOT(keyUpdated(KGpgKeyNode *)));
		m_keynode = nkey;
		m_keynode->addRef(this);
	}
}

QString
KGpgRefNode::getId() const
{
	if (m_keynode != NULL)
		return m_keynode->getId();
	else
		return m_id;
}

QString
KGpgRefNode::getName() const
{
	if (m_keynode != NULL)
		return m_keynode->getName();
	return i18n("[No user id found]");
}

QString
KGpgRefNode::getEmail() const
{
	if (m_keynode != NULL)
		return m_keynode->getEmail();
	return QString();
}

bool
KGpgRefNode::isUnknown() const
{
	return (m_keynode == NULL);
}

KGpgGroupMemberNode::KGpgGroupMemberNode(KGpgGroupNode *parent, const QString &k)
	: KGpgRefNode(parent, k)
{
}

KGpgGroupMemberNode::KGpgGroupMemberNode(KGpgGroupNode *parent, KGpgKeyNode *k)
	: KGpgRefNode(parent, k)
{
}

KgpgKeyTrust
KGpgGroupMemberNode::getTrust() const
{
	if (m_keynode != NULL)
		return m_keynode->getTrust();
	return TRUST_NOKEY;
}

KgpgItemType
KGpgGroupMemberNode::getType() const
{
	if (m_keynode != NULL)
		return m_keynode->getType() | ITYPE_GROUP;
	return ITYPE_PUBLIC | ITYPE_GROUP;
}

QString
KGpgGroupMemberNode::getSize() const
{
	if (m_keynode != NULL)
		return m_keynode->getSize();
	return QString();
}

QDate
KGpgGroupMemberNode::getExpiration() const
{
	if (m_keynode != NULL)
		return m_keynode->getExpiration();
	return QDate();
}

QDate
KGpgGroupMemberNode::getCreation() const
{
	if (m_keynode != NULL)
		return m_keynode->getCreation();
	return QDate();
}

unsigned int
KGpgGroupMemberNode::getSignKeySize() const
{
	if (m_keynode != NULL)
		return m_keynode->getSignKeySize();
	return 0;
}

unsigned int
KGpgGroupMemberNode::getEncryptionKeySize() const
{
	if (m_keynode != NULL)
		return m_keynode->getEncryptionKeySize();
	return 0;
}

KGpgGroupNode *
KGpgGroupMemberNode::getParentKeyNode() const
{
	return m_parent->toGroupNode();
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

