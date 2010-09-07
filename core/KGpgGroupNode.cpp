/* Copyright 2008,2009,2010 Rolf Eike Beer <kde@opensource.sf-tec.de>
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
#include "KGpgGroupNode.h"

#include <KLocale>

#include "KGpgGroupMemberNode.h"
#include "kgpginterface.h"
#include "KGpgRootNode.h"
#include "kgpgsettings.h"

KGpgGroupNode::KGpgGroupNode(KGpgRootNode *parent, const QString &name)
	: KGpgExpandableNode(parent),
	m_name(name)
{
	readChildren();
	parent->m_groups++;
}

KGpgGroupNode::KGpgGroupNode(KGpgRootNode *parent, const QString &name, const KGpgKeyNode::List &members)
	: KGpgExpandableNode(parent), m_name(name)
{
	Q_ASSERT(members.count() > 0);

	foreach (KGpgKeyNode *nd, members)
		new KGpgGroupMemberNode(this, nd);

	parent->m_groups++;
}

KGpgGroupNode::~KGpgGroupNode()
{
	KGpgRootNode *root = m_parent->toRootNode();

	if (root != NULL)
		root->m_groups--;
}

KgpgCore::KgpgItemType
KGpgGroupNode::getType() const
{
	return ITYPE_GROUP;
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

	foreach (const QString &id, keys)
		new KGpgGroupMemberNode(this, id);
}

void
KGpgGroupNode::rename(const QString& newName)
{
	if (KgpgInterface::renameGroup(m_name, newName, KGpgSettings::gpgConfigPath()))
		m_name = newName;
}
