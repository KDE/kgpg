/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KGPGSELECTSECRETKEY_H
#define KGPGSELECTSECRETKEY_H

#include <QString>

#include <kdialog.h>

class QCheckBox;

class Q3ListViewItem;

class KComboBox;
class K3ListView;

class KgpgSelectSecretKey : public KDialog
{
    Q_OBJECT

public:
    /**
     * Constructor.
     * @param parent is the parent object
     * @param signkey set to \em true if you are going to sign a key, set
     * to \em false if you are going to sign a message. Default is \em false
     * @param countkey if \em signkey is set to \em true, \em countkey is the
     * number of keys that you are going to sign. Default is 1.
     */
    KgpgSelectSecretKey(QWidget *parent = 0, const bool &signkey = false, const int &countkey = 1);

    QString getKeyID() const;
    QString getKeyMail() const;
    int getSignTrust() const;
    bool isLocalSign() const;
    bool isTerminalSign() const;

private slots:
    void slotOk(Q3ListViewItem *item);
    void slotSelectionChanged();

private:
    QCheckBox *m_localsign;
    QCheckBox *m_terminalsign;

    KComboBox *m_signtrust;
    K3ListView *m_keyslist;

    bool m_signkey;
};

#endif // KGPGSELECTSECRETKEY_H
