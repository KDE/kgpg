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
#include "kgpgitemnode.h"

#include <KLocale>
#include "kgpginterface.h"
#include "kgpgsettings.h"
#include "convert.h"
#include "kgpgitemmodel.h"

KGpgKeyNode::KGpgKeyNode(KGpgExpandableNode *parent, const KgpgKey &k)
	: KGpgSignableNode(parent),
	m_key(new KgpgKey(k)),
	m_signs(0)
{
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

bool
KGpgKeyNode::hasChildren() const
{
	return true;
}

KgpgItemType
KGpgKeyNode::getType(const KgpgKey *k)
{
	if (k->secret())
		return ITYPE_PAIR;

	return ITYPE_PUBLIC;
}

KgpgCore::KgpgKeyTrust
KGpgKeyNode::getTrust() const
{
	return m_key->trust();
}

QString
KGpgKeyNode::getKeyId() const
{
	return m_key->fullId();
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

QString
KGpgKeyNode::getBeautifiedFingerprint() const
{
	return m_key->fingerprintBeautified();
}

QString
KGpgKeyNode::getComment() const
{
	return m_key->comment();
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

void
KGpgKeyNode::insertSigns(KGpgExpandableNode *node, const KgpgKeySignList &list)
{
	for (int i = 0; i < list.size(); ++i)
	{
		(void) new KGpgSignNode(node, list.at(i));
	}
}

QString
KGpgKeyNode::getSignCount() const
{
	if (!wasExpanded())
		return QString();
	return i18np("1 signature", "%1 signatures", m_signs);
}

KgpgKey *
KGpgKeyNode::copyKey() const
{
	return new KgpgKey(*m_key);
}

void
KGpgKeyNode::setKey(const KgpgKey &key)
{
	Q_ASSERT(m_key->fingerprint() == key.fingerprint());
	delete m_key;

	for (int i = 0; i < children.count(); i++)
		delete children.at(i);
	children.clear();

	m_key = new KgpgKey(key);
}

const KgpgKey *
KGpgKeyNode::getKey() const
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

QList<KGpgGroupNode *>
KGpgKeyNode::getGroups(void) const
{
	QList<KGpgGroupNode *> ret;
	QList<KGpgGroupMemberNode *> gnodes = getGroupRefs();

	for (int i = 0; i < gnodes.count(); i++)
		ret.append(gnodes.at(i)->getParentKeyNode());
	return ret;
}

QList<KGpgRefNode *>
KGpgKeyNode::getRefsOfType(const KgpgItemType &type) const
{
	QList<KGpgRefNode *> ret;

	for (int i = 0; i < m_refs.count(); i++) {
		KGpgRefNode *nd = m_refs.at(i);
		if (nd->getType() & type)
			ret.append(nd);
	}
	return ret;
}

QList<KGpgGroupMemberNode *>
KGpgKeyNode::getGroupRefs(void) const
{
	QList<KGpgGroupMemberNode *> ret;

	QList<KGpgRefNode *> refs = getRefsOfType(ITYPE_GROUP);

	for (int i = 0; i < refs.count(); i++)
		ret.append(refs.at(i)->toGroupMemberNode());

	return ret;
}

KGpgSignNode::List
KGpgKeyNode::getSignRefs(void) const
{
	KGpgSignNode::List ret;

	QList<KGpgRefNode *> refs = getRefsOfType(ITYPE_SIGN);

	for (int i = 0; i < refs.count(); i++)
		ret.append(refs.at(i)->toSignNode());

	return ret;
}

KGpgSignNode::List
KGpgKeyNode::getSignatures(const bool subkeys) const
{
	KGpgSignNode::List ret = KGpgSignableNode::getSignatures();

	if (!subkeys)
		return ret;

	for (int i = 0; i < children.count(); i++) {
		KGpgNode *child = children.at(i);
		KGpgSignNode::List tmp;

		switch (child->getType()) {
		case ITYPE_UID:
		case ITYPE_UAT:
			tmp = child->toSignableNode()->getSignatures();
			break;
		default:
			continue;
		}

		for (int j = 0; j < tmp.count(); j++) {
			bool found = false;
			KGpgSignNode *sn = tmp.at(j);

			for (int k = 0; k < ret.count(); k++) {
				found = (ret.at(k)->getId() == sn->getId());
				if (found)
					break;
			}

			if (!found)
				ret << sn;
		}
	}

	return ret;
}

const KGpgSignableNode *
KGpgKeyNode::getUid(const unsigned int index) const
{
	Q_ASSERT(index > 0);

	if (index == 1)
		return this;

	const QString idxstr(QString::number(index));

	for (unsigned int i = 0; i < static_cast<unsigned int>(children.count()); i++) {
		const KGpgNode *child = children.at(i);
		KGpgSignNode::List tmp;

		switch (child->getType()) {
		case ITYPE_UID:
		case ITYPE_UAT:
			if (child->getId() == idxstr)
				return child->toSignableNode();
			break;
		default:
			continue;
		}
	}

	return NULL;
}

#include "KGpgKeyNode.moc"
