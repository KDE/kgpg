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

#include "kgpginterface.h"

KGpgChangeKey::KGpgChangeKey(KgpgCore::KgpgKey *key)
{
	m_key = *key;
	iface = NULL;
	m_expiration = key->expirationDate();
	m_disable = !key->valid();
	m_owtrust = key->ownerTrust();
	m_autodestroy = false;
	m_step = 0;
}

KGpgChangeKey::~KGpgChangeKey()
{
	delete iface;
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

	if (iface == NULL) {
		iface = new KgpgInterface();
		connect(iface, SIGNAL(changeDisableFinished(int, KgpgInterface *)), SLOT(nextStep(int, KgpgInterface *)));
		connect(iface, SIGNAL(changeTrustFinished(int, KgpgInterface *)), SLOT(nextStep(int, KgpgInterface *)));
		connect(iface, SIGNAL(keyExpireFinished(int, KgpgInterface *)), SLOT(nextStep(int, KgpgInterface *)));
	}

	m_step = 0;
	m_failed = 0;

	nextStep(0, iface);

	return true;
}

void KGpgChangeKey::nextStep(int result, KgpgInterface *)
{
	m_step++;

	switch (m_step) {
	case 1:
		if (m_expiration != m_key.expirationDate()) {
			iface->keyExpire(m_key.fullId(), m_expiration);
			break;
		} else {
			m_step++;
		}
		result = 2;
		// fall through
	case 2:
		if (result == 2) {
			m_key.setExpiration(m_expiration);
		} else {
			m_failed |= 1;
		}
		if (m_owtrust != m_key.ownerTrust()) {
			iface->changeTrust(m_key.fullId(), m_owtrust);
			break;
		} else {
			m_step++;
		}
		result = 0;
		// fall through
	case 3:
		if (result == 0) {
			m_key.setOwnerTrust(m_owtrust);
		} else {
			m_failed |= 2;
		}
		if (m_key.valid() == m_disable) {
			iface->changeDisable(m_key.fullId(), m_disable);
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
			emit keyNeedsRefresh(m_key.fullId());
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
