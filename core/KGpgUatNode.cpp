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
#include "KGpgUatNode.h"

#include <KLocale>

#include "kgpginterface.h"
#include "KGpgKeyNode.h"

KGpgUatNode::KGpgUatNode(KGpgKeyNode *parent, const KgpgCore::KgpgKeyUat &k, const QString &index)
	: KGpgSignableNode(parent),
	m_uat(k),
	m_idx(index)
{
	KgpgInterface iface;

	m_pic = iface.loadPhoto(parent->getKeyId(), index, true);
}

KGpgUatNode::~KGpgUatNode()
{
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

KGpgKeyNode *
KGpgUatNode::getParentKeyNode() const
{
	return m_parent->toKeyNode();
}

void
KGpgUatNode::readChildren()
{
}


KgpgCore::KgpgItemType
KGpgUatNode::getType() const
{
	return KgpgCore::ITYPE_UAT;
}

KgpgCore::KgpgKeyTrust
KGpgUatNode::getTrust() const
{
	return KgpgCore::TRUST_NOKEY;
}

QPixmap
KGpgUatNode::getPixmap() const
{
	return m_pic;
}

QString
KGpgUatNode::getId() const
{
	return m_idx;
}

KGpgKeyNode *
KGpgUatNode::getKeyNode(void)
{
	return getParentKeyNode()->toKeyNode();
}

const KGpgKeyNode *
KGpgUatNode::getKeyNode(void) const
{
	return getParentKeyNode()->toKeyNode();
}
