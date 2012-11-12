/* Copyright 2008  Rolf Eike Beer <kde@opensource.sf-tec.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef KGPGFIRSTASSISTANT_H
#define KGPGFIRSTASSISTANT_H

#include <KAssistantDialog>

class QCheckBox;
class QLabel;
class KPageWidgetItem;
class KUrlRequester;
class KComboBox;

class KGpgFirstAssistant : public KAssistantDialog
{
	Q_OBJECT

private:
	KPageWidgetItem *page_welcome;
	KPageWidgetItem *page_binary;
	KPageWidgetItem *page_config;
	KPageWidgetItem *page_defaultkey;
	KPageWidgetItem *page_done;

	QLabel *defaultkeylabel;
	QLabel *txtGpgVersion;
	QLabel *text_optionsfound;
	QLabel *versionLabel;
	QLabel *binlabel;

	QCheckBox *generateCB;
	QCheckBox *autostartCB;

	KComboBox *CBdefault;

	KUrlRequester *binURL;
	KUrlRequester *pathURL;
	
	QString m_gpgVersion;
	QString m_confPath;

	void findConfigPath();

public:
	/**
	 * @brief constructor of KGpgFirstAssistant
	 */
	explicit KGpgFirstAssistant(QWidget *parent = 0);

	/**
	 * @brief check if key generation dialog should be started
	 * @return if user requests dialog to be started
	 */
	bool runKeyGenerate() const;
	/**
	 * @brief get user selected GnuPG home directory
	 * @return path to users GnuPG directory
	 */
	QString getConfigPath() const;
	/**
	 * @brief get fingerprint of default key
	 * @return full fingerprint or empty string if user has not selected a default key
	 */
	QString getDefaultKey() const;
	/**
	 * @brief check if KGpg autostart should be activated
	 * @return if user requests autostart or not
	 */
	bool getAutoStart() const;

public Q_SLOTS:
	/**
	 * @brief called when "next" button is pressed
	 */
	virtual void next();

private Q_SLOTS:
	void slotBinaryChanged(const QString &binary);
};

#endif
