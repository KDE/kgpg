/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2007 Jimmy Gilles <jimmygilles@gmail.com>
 * Copyright (C) 2008,2009,2010,2012,2014,2016,2017 Rolf Eike Beer <kde@opensource.sf-tec.de>
 * Copyright (C) 2011 Philip G. Lee <rocketman768@gmail.com>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KGPGKEYINFODIALOG_H
#define KGPGKEYINFODIALOG_H

#include <QColor>
#include <QDialog>
#include <QLabel>
#include <QString>

#include "ui_kgpgKeyInfo.h"

class QDialogButtonBox;

class KGpgItemModel;
class KGpgKeyNode;
class KGpgChangeKey;

class KgpgTrustLabel : public QWidget
{
    Q_OBJECT

public:
    explicit KgpgTrustLabel(QWidget *parent = nullptr);

    void setText(const QString &text);
    void setColor(const QColor &color);

    QString text() const;
    QColor color() const;

private:
    void change();

    QLabel *m_text_w;
    QLabel *m_color_w;

    QString m_text;
    QColor m_color;
};

class KgpgKeyInfo : public QDialog, public Ui::kgpgKeyInfo
{
	Q_OBJECT

	KgpgKeyInfo() = delete;
	Q_DISABLE_COPY(KgpgKeyInfo)
public:
	KgpgKeyInfo(KGpgKeyNode *node, KGpgItemModel *model, QWidget *parent);
	~KgpgKeyInfo();

	KGpgChangeKey *keychange;

Q_SIGNALS:
    void keyNeedsRefresh(KGpgKeyNode *node);

private:
    void reloadKey();
    void reloadNode();
    void displayKey();
    void setControlEnable(const bool b);
    void okButtonClicked();
    void applyButtonClicked();
    void cancelButtonClicked();

private Q_SLOTS:
    void slotOpenUrl(const QString &url) const;

    void slotChangeDate();

    void slotDisableKey(const bool ison);

    void slotChangePass();
    void slotInfoPasswordChanged(int result);

    void slotChangeTrust(const int newtrust);

    void slotLoadPhoto(const QString &uid);

    void slotApplied(int result);

    void slotKeyExpanded();

private:
	KGpgKeyNode *m_node;
	KGpgItemModel * const m_model;

	KgpgTrustLabel * const m_trust;

    QDialogButtonBox *buttonBox;

    bool m_keywaschanged;
    bool m_closewhendone;
};

#endif // KGPGKEYINFODIALOG_H
