/**
 * Copyright (C) 2008 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kgpgchangekey.h"

#include "kgpgitemnode.h"
#include "kgpgchangetrust.h"
#include "kgpgchangeexpire.h"
#include "kgpgchangedisable.h"

KGpgChangeKey::KGpgChangeKey(KGpgKeyNode *node)
{
	m_node = node;
	m_key = *node->copyKey();
	init();
}

KGpgChangeKey::KGpgChangeKey(KgpgCore::KgpgKey *key)
{
	m_node = NULL;
	m_key = *key;
	init();
}

void KGpgChangeKey::init()
{
	m_changetrust = NULL;
	m_changeexpire = NULL;
	m_changedisable = NULL;
	m_expiration = m_key.expirationDate();
	m_disable = !m_key.valid();
	m_owtrust = m_key.ownerTrust();
	m_autodestroy = false;
	m_step = 0;
}

KGpgChangeKey::~KGpgChangeKey()
{
	delete m_changetrust;
	delete m_changeexpire;
	delete m_changedisable;
}

void KGpgChangeKey::setExpiration(const QDate &date)
{
	m_expiration = date;
}

void KGpgChangeKey::setDisable(const bool &disable)
{
	m_disable = disable;
}

void KGpgChangeKey::setOwTrust(const KgpgCore::KgpgKeyOwnerTrust &trust)
{
	m_owtrust = trust;
}

bool KGpgChangeKey::apply()
{
	if (!wasChanged()) {
		emit done(0);
		return true;
	}

	if (m_step != 0)
		return false;

	m_step = 0;
	m_failed = 0;

	nextStep(0);

	return true;
}

void KGpgChangeKey::nextStep(int result)
{
	m_step++;

	switch (m_step) {
	case 1:
		if (m_expiration != m_key.expirationDate()) {
			if (m_changeexpire == NULL) {
				m_changeexpire = new KGpgChangeExpire(this, m_key.fingerprint(), m_expiration);

				connect(m_changeexpire, SIGNAL(done(int)), SLOT(nextStep(int)));
			} else {
				m_changeexpire->setDate(m_expiration);
			}
			m_changeexpire->start();
			break;
		} else {
			m_step++;
		}
		// fall through
	case 2:
		if (result == 0) {
			m_key.setExpiration(m_expiration);
		} else {
			m_failed |= 1;
		}
		if (m_owtrust != m_key.ownerTrust()) {
			if (m_changetrust == NULL) {
				m_changetrust = new KGpgChangeTrust(this, m_key.fingerprint(), m_owtrust);

				connect(m_changetrust, SIGNAL(done(int)), SLOT(nextStep(int)));
			} else {
				m_changetrust->setTrust(m_owtrust);
			}
			m_changetrust->start();
			break;
		} else {
			m_step++;
		}
		// fall through
	case 3:
		if (result == 0) {
			m_key.setOwnerTrust(m_owtrust);
		} else {
			m_failed |= 2;
		}
		if (m_key.valid() == m_disable) {
			if (m_changedisable == NULL) {
				m_changedisable = new KGpgChangeDisable(this, m_key.fingerprint(), m_disable);

				connect(m_changedisable, SIGNAL(done(int)), SLOT(nextStep(int)));
			} else {
				m_changedisable->setDisable(m_disable);
			}
			m_changedisable->start();
			break;
		} else {
			m_step++;
		}
		// fall through
	default:
		if (result == 0) {
			m_key.setValid(!m_disable);
		} else {
			m_failed |= 4;
		}
		m_step = 0;
		emit done(m_failed);
		if (m_autodestroy) {
			if (m_node)
				emit keyNeedsRefresh(m_node);
			deleteLater();
		}
	}
}

bool KGpgChangeKey::wasChanged()
{
	if (m_key.expirationDate() != m_expiration)
		return true;

	if (m_key.ownerTrust() != m_owtrust)
		return true;

	if (m_key.valid() == m_disable)
		return true;

	return false;
}

void KGpgChangeKey::selfdestruct(const bool &applyChanges)
{
	m_autodestroy = true;

	// if apply is already running it will take care of everything
	if (m_step != 0)
		return;

	if (applyChanges && wasChanged())
		apply();
	else
		deleteLater();
}

#include "kgpgchangekey.moc"
