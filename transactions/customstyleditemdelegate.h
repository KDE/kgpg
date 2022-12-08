/*
    SPDX-FileCopyrightText: 2022 Dmitrii Fomchenkov <fomchenkovda@basealt.ru>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef CUSTOMSTYLEDITEMDELEGATE_H
#define CUSTOMSTYLEDITEMDELEGATE_H

#include <QStyledItemDelegate>

class CustomStyledItemDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	explicit CustomStyledItemDelegate(QWidget *parent = nullptr)
		: QStyledItemDelegate(parent) {}

	void paint(QPainter *painter, const QStyleOptionViewItem &option,
			   const QModelIndex &index) const override;
};

#endif // CUSTOMSTYLEDITEMDELEGATE_H
