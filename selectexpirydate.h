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

#ifndef SELECTEXPIRYDATE_H
#define SELECTEXPIRYDATE_H

#include <QDateTime>
#include <QDialog>

class KDatePicker;
class QCheckBox;
class QPushButton;

/**
 * @brief shows a dialog to choose expiry date
 *
 * This dialog allows the user to choose a expiry date or set a
 * checkbox to select that the given item will never expire.
 */
class SelectExpiryDate : public QDialog
{
	Q_OBJECT

public:
	explicit SelectExpiryDate(QWidget *parent = Q_NULLPTR, QDateTime date = QDateTime());

	QDateTime date() const;

private slots:
	void slotCheckDate(const QDate &date);
	void slotEnableDate(const bool ison);

private:
	QCheckBox *m_unlimited;
	QPushButton *okButton;
	KDatePicker *m_datepicker;
};

#endif /* SELECTEXPIRYDATE_H */
