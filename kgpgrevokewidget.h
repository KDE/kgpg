/***************************************************************************
    begin                : Thu Jul 4 2002
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


#ifndef KGPGREVOKEWIDGET_H
#define KGPGREVOKEWIDGET_H

#include "ui_kgpgrevokewidget.h"
#include <KDialog>

class KGpgKeyNode;

class KgpgRevokeWidget : public QWidget, public Ui_KgpgRevokeWidget
{
    Q_OBJECT

public:
    explicit KgpgRevokeWidget(QWidget* parent = 0);


public slots:
   virtual void cbSave_toggled(bool isOn);
};

class KGpgRevokeDialog: public KDialog
{
	Q_OBJECT

	Q_DISABLE_COPY(KGpgRevokeDialog)
	KGpgRevokeDialog(); // = delete C++0x
public:
	KGpgRevokeDialog(QWidget* parent, const KGpgKeyNode* node);

	QString getDescription() const;
	int getReason() const;
	KUrl saveUrl() const;
	QString getId() const;
	bool printChecked();
	bool importChecked();

private:
	KgpgRevokeWidget *m_revWidget;
	const QString m_id;
};

#endif
