/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2007 Jimmy Gilles <jimmygilles@gmail.com>
 * Copyright (C) 2008,2014 Rolf Eike Beer <kde@opensource.sf-tec.de>
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

#include <QString>
#include <QLabel>
#include <QColor>

#include <KDialog>

#include "ui_kgpgKeyInfo.h"

class QCheckBox;
class QGroupBox;

class KPushButton;
class KUrlLabel;
class KComboBox;

class KGpgItemModel;
class KGpgKeyNode;
class KGpgChangeKey;
class KGpgChangePass;

class KgpgTrustLabel : public QWidget
{
    Q_OBJECT

public:
    explicit KgpgTrustLabel(QWidget *parent = 0, const QString &text = QString(), const QColor &color = QColor());

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

class KgpgKeyInfo : public KDialog, public Ui::kgpgKeyInfo
{
	Q_OBJECT

	KgpgKeyInfo(); // = delete C++0x
	Q_DISABLE_COPY(KgpgKeyInfo)
public:
	KgpgKeyInfo(KGpgKeyNode *node, KGpgItemModel *model, QWidget *parent);
	~KgpgKeyInfo();

	KGpgChangeKey *keychange;

signals:
    void keyNeedsRefresh(KGpgKeyNode *node);

protected slots:
    void slotButtonClicked(int button);

private:
    void reloadKey();
    void reloadNode();
    void displayKey();
    void setControlEnable(const bool b);

private slots:
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
	KGpgItemModel *m_model;

    KgpgTrustLabel *m_trust;

    bool m_keywaschanged;
    bool m_closewhendone;
};

#endif // KGPGKEYINFODIALOG_H
