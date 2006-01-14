/***************************************************************************
                          keygen.h  -  description
                             -------------------
    begin                : Mon Jul 8 2002
    copyright          : (C) 2002 by Jean-Baptiste Mardelle
    email                : bj@altern.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef KGPGKEYGENERATE_H
#define KGPGKEYGENERATE_H

#include <QString>

#include <kdialog.h>

#include "kgpgkey.h"

class KComboBox;
class KLineEdit;

class KgpgKeyGenerate : public KDialog
{
    Q_OBJECT

public:
    KgpgKeyGenerate(QWidget *parent = 0, const char *name = 0);

    bool getMode() const;
    QString getKeyName() const;
    QString getKeyEmail() const;
    QString getKeyComment() const;
    Kgpg::KeyAlgo getKeyAlgo() const;
    uint getKeySize() const;
    uint getKeyExp() const;
    uint getKeyNumber() const;

public slots:
    void slotOk();
    void slotUser1();
    void slotActivateExp(const int &state);

private slots:
    void slotEnableOk();

private:
    KComboBox *m_keykind;
    KComboBox *m_keysize;
    KComboBox *m_keyexp;
    KLineEdit *m_numb;
    KLineEdit *m_comment;
    KLineEdit *m_kname;
    KLineEdit *m_mail;
    bool m_expert;
};

#endif // KGPGKEYGENERATE_H
