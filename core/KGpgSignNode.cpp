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
#include "KGpgSignNode.h"

#include <KLocale>

KGpgSignNode::KGpgSignNode(KGpgExpandableNode *parent, const KgpgCore::KgpgKeySign &s)
	: KGpgRefNode(parent, s.fullId()),
	m_sign(new KgpgCore::KgpgKeySign(s))
{
}

KGpgSignNode::~KGpgSignNode()
{
	delete m_sign;
}

QDateTime
KGpgSignNode::getExpiration() const
{
	return m_sign->expirationDate();
}

QDateTime
KGpgSignNode::getCreation() const
{
	return m_sign->creationDate();
}

QString
KGpgSignNode::getId() const
{
	return m_sign->fullId();
}

QString
KGpgSignNode::getName() const
{
	QString name(KGpgRefNode::getName());

	if (m_keynode == NULL)
		return name;

	if (!m_sign->local())
		return name;

	return i18n("%1 [local signature]", name);
}

KgpgCore::KgpgItemType
KGpgSignNode::getType() const
{
	return KgpgCore::ITYPE_SIGN;
}

QString
KGpgSignNode::getComment() const
{
	return m_sign->comment();
}
