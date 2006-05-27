/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KGPGMD5WIDGET_H
#define KGPGMD5WIDGET_H

#include <QString>
#include <QLabel>

#include <kdialog.h>
#include <kurl.h>

class KLed;

class Md5Widget : public KDialog
{
    Q_OBJECT

public:
    Md5Widget(QWidget *parent = 0, const KUrl &url = KUrl());

public slots:
    void slotApply();

private:
    QString m_mdsum;
    KLed *m_kled;
    QLabel *m_textlabel;
};

#endif // KGPGMD5WIDGET_H
