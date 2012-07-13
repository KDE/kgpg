/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2007 Jimmy Gilles <jimmygilles@gmail.com>
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

#include "selectexpirydate.h"

#include <QCheckBox>
#include <QVBoxLayout>

#include <KDatePicker>
#include <KLocale>

SelectExpiryDate::SelectExpiryDate(QWidget* parent, QDateTime date)
	: KDialog(parent)
{
	setCaption(i18n("Choose New Expiration"));
	setButtons(Ok | Cancel);
	setDefaultButton(Ok);
	setModal(true);

	QWidget *page = new QWidget(this);
	m_unlimited = new QCheckBox(i18nc("Key has unlimited lifetime", "Unlimited"), page);
	m_unlimited->setChecked(date.isNull());

	if (date.isNull())
		date = QDateTime::currentDateTime();

	m_datepicker = new KDatePicker(date.date(), page);
	if (date.isNull()) {
		m_datepicker->setEnabled(false);
		m_unlimited->setChecked(true);
	}

	QVBoxLayout *layout = new QVBoxLayout(page);
	layout->setSpacing(3);
	layout->addWidget(m_datepicker);
	layout->addWidget(m_unlimited);

	connect(m_unlimited, SIGNAL(toggled(bool)), this, SLOT(slotEnableDate(bool)));
	connect(m_datepicker, SIGNAL(dateChanged(QDate)), this, SLOT(slotCheckDate(QDate)));
	connect(m_datepicker, SIGNAL(dateEntered(QDate)), this, SLOT(slotCheckDate(QDate)));

	setMainWidget(page);
	show();

	slotEnableDate(m_unlimited->isChecked());
}

QDateTime SelectExpiryDate::date() const
{
	if (m_unlimited->isChecked())
		return QDateTime();
	else
		return QDateTime(m_datepicker->date());
}

void SelectExpiryDate::slotCheckDate(const QDate& date)
{
	enableButtonOk(QDateTime(date) >= QDateTime::currentDateTime());
}

void SelectExpiryDate::slotEnableDate(const bool ison)
{
	m_datepicker->setEnabled(!ison);
	if (ison)
		enableButtonOk(true);
	else
		slotCheckDate(m_datepicker->date());
}

#include "selectexpirydate.moc"
