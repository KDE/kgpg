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
#include "KGpgRefNode.h"

#include <KLocale>

#include "KGpgExpandableNode.h"
#include "KGpgRootNode.h"

KGpgRefNode::KGpgRefNode(KGpgExpandableNode *parent, const QString &keyid)
	: KGpgNode(parent)
{
	Q_ASSERT(!keyid.isEmpty());

	KGpgRootNode *root = getRootNode();
	KGpgExpandableNode *pnd = parent;

	do {
		m_selfsig = (pnd->getId().right(keyid.length()) == keyid);
		if (m_selfsig)
			m_keynode = pnd->toKeyNode();
		else
			pnd = pnd->getParentKeyNode();
	} while (!m_selfsig && (pnd != root));

	// Self signatures do net need to get notified by their key: if the key is changed
	// the key node is deleted, then those refnode would be deleted anyway. This avoids
	// crashes when they would try to find the root node by iterating over their parent
	// when the parents destructor is already called (see bug 208659).
	if (!m_selfsig) {
		m_keynode = root->findKey(keyid);
		if (m_keynode != NULL) {
			connect(m_keynode, SIGNAL(updated(KGpgKeyNode *)), this, SLOT(keyUpdated(KGpgKeyNode *)));
			m_keynode->addRef(this);
		} else {
			m_id = keyid;
			connect(root, SIGNAL(newKeyNode(KGpgKeyNode *)), this, SLOT(keyUpdated(KGpgKeyNode *)));
		}
	}

	parent->children.append(this);
}

KGpgRefNode::~KGpgRefNode()
{
	if (m_keynode && !m_selfsig)
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

KGpgRootNode *
KGpgRefNode::getRootNode() const
{
	KGpgExpandableNode *root;
	KGpgExpandableNode *pt = m_parent;

	do {
		root = pt;
		pt = pt->getParentKeyNode();
	} while (pt != NULL);

	return root->toRootNode();
}

void
KGpgRefNode::keyUpdated(KGpgKeyNode *nkey)
{
	disconnect(this, SLOT(keyUpdated(KGpgKeyNode *)));

	if (nkey == NULL) {
		KGpgRootNode *root = getRootNode();

		m_id = m_keynode->getId();
		if (root != NULL)
			connect(root, SIGNAL(newKeyNode(KGpgKeyNode *)), this, SLOT(keyUpdated(KGpgKeyNode *)));
		m_keynode = NULL;
	} else if ((m_keynode == NULL) && (nkey->getId().right(m_id.length()) == m_id)) {
		m_id.clear();
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

KGpgKeyNode *
KGpgRefNode::getRefNode() const
{
	return m_keynode;
}
