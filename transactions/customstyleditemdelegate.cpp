/*
    SPDX-FileCopyrightText: 2022 Dmitrii Fomchenkov <fomchenkovda@basealt.ru>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "customstyleditemdelegate.h"
#include "model/kgpgitemmodel.h"

void CustomStyledItemDelegate::paint(QPainter *painter,
									 const QStyleOptionViewItem &option,
									 const QModelIndex &index) const
{
	QStyleOptionViewItem opt(option);

	if (option.state & QStyle::State_Selected &&
			index.column() == KEYCOLUMN_TRUST) {
		QColor bdata = index.data(Qt::BackgroundRole).value<QColor>();

		opt.palette.setBrush(QPalette::Highlight, QBrush(bdata));
	}

	QStyledItemDelegate::paint(painter, opt, index);
}
