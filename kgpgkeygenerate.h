/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2007,2009,2012,2013 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

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

#include "ui_kgpgkeygenerate.h"

#include "core/kgpgkey.h"

#include <KDialog>

class KComboBox;
class KLineEdit;

class KgpgKeyGenerate : public KDialog, public Ui::kgpgKeyGenerate
{
    Q_OBJECT

public:
    explicit KgpgKeyGenerate(QWidget *parent = 0);

    bool isExpertMode() const;
    QString name() const;
    QString email() const;
    QString comment() const;
    KgpgCore::KgpgKeyAlgo algo() const;

    /**
     * @brief return the selected capabilities for the new key
     * @retval 0 default capabilities of the selected algorithm should be used
     */
    KgpgCore::KgpgSubKeyType caps() const;
    uint size() const;
    char expiration() const;
    uint days() const;

private slots:
    void slotOk();
    void slotUser1();
    void slotButtonClicked(int button);
    void slotEnableOk();
    void slotEnableDays(const int state);
    void slotEnableCaps(const int state);

private:
    KComboBox *m_keyexp;
    bool m_expert;
};

#endif // KGPGKEYGENERATE_H
