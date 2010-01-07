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
#include "KGpgUidNode.h"

#include "KGpgKeyNode.h"

KGpgUidNode::KGpgUidNode(KGpgKeyNode *parent, const KgpgCore::KgpgKeyUid &u)
	: KGpgSignableNode(parent),
	m_uid(new KgpgCore::KgpgKeyUid(u))
{
}

KGpgUidNode::~KGpgUidNode()
{
	delete m_uid;
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
KGpgUidNode::getId() const
{
	return QString::number(m_uid->index());
}

KGpgKeyNode *
KGpgUidNode::getKeyNode(void)
{
	return getParentKeyNode()->toKeyNode();
}

const KGpgKeyNode *
KGpgUidNode::getKeyNode(void) const
{
	return getParentKeyNode()->toKeyNode();
}

KGpgKeyNode *
KGpgUidNode::getParentKeyNode() const
{
	return m_parent->toKeyNode();
}

void
KGpgUidNode::readChildren()
{
}

KgpgCore::KgpgItemType
KGpgUidNode::getType() const
{
	return KgpgCore::ITYPE_UID;
}

KgpgCore::KgpgKeyTrust
KGpgUidNode::getTrust() const
{
	return m_uid->trust();
}

QString
KGpgUidNode::getComment() const
{
	return m_uid->comment();
}
