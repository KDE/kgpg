/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __KGPGMD5WIDGET_H__
#define __KGPGMD5WIDGET_H__

#include <QString>
#include <QLabel>

#include <kdialogbase.h>
#include <kurl.h>

class KLed;

class  Md5Widget : public KDialogBase
{
    Q_OBJECT

public:
    Md5Widget(QWidget *parent = 0, const char *name = 0, const KURL &url = KURL());

public slots:
    void slotApply();

private:
    QString m_mdsum;
    KLed *m_kled;
    QLabel *m_textlabel;
};

#endif // __KGPGMD5WIDGET_H__
