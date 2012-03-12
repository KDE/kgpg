/* Copyright 2008,2010,2012  Rolf Eike Beer <kde@opensource.sf-tec.de>
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

#include "kgpgfirstassistant.h"

#include "gpgproc.h"
#include "kgpginterface.h"
#include "core/kgpgkey.h"

#include <KComboBox>
#include <KLocale>
#include <KMessageBox>
#include <KStandardDirs>
#include <KUrlRequester>
#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpacerItem>
#include <QTextStream>
#include <QWidget>

using namespace KgpgCore;

KGpgFirstAssistant::KGpgFirstAssistant(QWidget *parent)
		: KAssistantDialog(parent)
{
	setCaption(i18n("KGpg Assistant"));

	QWidget *page = new QWidget(this);
	QGridLayout *gridLayout = new QGridLayout(page);
	gridLayout->setSpacing(6);
	gridLayout->setMargin(11);
	gridLayout->setContentsMargins(0, 0, 0, 0);

	QLabel *label = new QLabel(page);
	QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
	sizePolicy.setHorizontalStretch(0);
	sizePolicy.setVerticalStretch(0);
	sizePolicy.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
	label->setSizePolicy(sizePolicy);
	label->setFrameShape(QFrame::NoFrame);
	label->setFrameShadow(QFrame::Plain);
	label->setScaledContents(false);
	label->setAlignment(Qt::AlignTop);
	label->setWordWrap(false);

	gridLayout->addWidget(label, 0, 0, 3, 1);

	label = new QLabel(page);
	label->setAlignment(Qt::AlignTop);
	label->setWordWrap(true);
	label->setText(i18n("This assistant will first setup some basic configuration options required for KGpg to work properly. Next, it will allow you to create your own key pair, enabling you to encrypt your files and emails."));

	gridLayout->addWidget(label, 0, 1, 1, 1);

	QSpacerItem *spacerItem = new QSpacerItem(20, 41, QSizePolicy::Minimum, QSizePolicy::Expanding);

	gridLayout->addItem(spacerItem, 1, 1, 1, 1);

	page_welcome = addPage(page, i18n("Welcome to the KGpg Assistant"));

	page = new QWidget(this);

	gridLayout = new QGridLayout(page);
	gridLayout->setSpacing(6);
	gridLayout->setMargin(11);
	gridLayout->setContentsMargins(0, 0, 0, 0);

	label = new QLabel(page);
	label->setAlignment(Qt::AlignTop);
	label->setWordWrap(true);
	label->setText(i18n("KGpg needs to know which GnuPG binary to use."));

	gridLayout->addWidget(label, 0, 1, 1, 1);

	label = new QLabel(page);
	label->setAlignment(Qt::AlignTop);
	label->setWordWrap(true);
	label->setText(i18n("Unless you want to try some unusual settings, just click on the \"next\" button."));

	gridLayout->addWidget(label, 1, 1, 1, 1);

	spacerItem = new QSpacerItem(20, 60, QSizePolicy::Minimum, QSizePolicy::Expanding);

	gridLayout->addItem(spacerItem, 4, 1, 1, 1);

	txtGpgVersion = new QLabel(page);
	txtGpgVersion->setWordWrap(true);

	gridLayout->addWidget(txtGpgVersion, 3, 1, 1, 1);

	binURL = new KUrlRequester(page);
	binURL->setFilter(i18nc("search filter for gpg binary", "gpg|GnuPG binary\n*|All files"));
	QString gpgBin = KStandardDirs::findExe(QLatin1String("gpg2"));
	if (gpgBin.isEmpty())
		gpgBin = KStandardDirs::findExe(QLatin1String("gpg"));
	if (gpgBin.isEmpty())
		gpgBin = QLatin1String("gpg");
	binURL->setUrl(KUrl::fromPath(gpgBin));

	connect(binURL, SIGNAL(textChanged(QString)), SLOT(slotBinaryChanged(QString)));
	slotBinaryChanged(gpgBin);

	gridLayout->addWidget(binURL, 2, 1, 1, 1);

	page_binary = addPage(page, i18n("GnuPG Binary"));

	page = new QWidget(this);

	gridLayout = new QGridLayout(page);
	gridLayout->setSpacing(6);
	gridLayout->setMargin(11);
	gridLayout->setContentsMargins(0, 0, 0, 0);
	text_optionsfound = new QLabel(page);
	text_optionsfound->setAlignment(Qt::AlignTop);
	text_optionsfound->setWordWrap(true);
	text_optionsfound->setText(i18n("Unless you want to try some unusual settings, just click on the \"next\" button."));

	gridLayout->addWidget(text_optionsfound, 1, 1, 1, 1);

	label = new QLabel(page);
	label->setAlignment(Qt::AlignTop);
	label->setWordWrap(true);
	label->setText(i18n("KGpg needs to know where your GnuPG configuration file is stored."));

	gridLayout->addWidget(label, 0, 1, 1, 1);

	spacerItem = new QSpacerItem(20, 60, QSizePolicy::Minimum, QSizePolicy::Expanding);

	gridLayout->addItem(spacerItem, 4, 1, 1, 1);

	pathURL = new KUrlRequester(page);

	gridLayout->addWidget(pathURL, 3, 1, 1, 1);

	label = new QLabel(page);
	label->setAlignment(Qt::AlignVCenter);
	label->setWordWrap(true);
	label->setText(i18n("Path to your GnuPG configuration file:"));

	gridLayout->addWidget(label, 2, 1, 1, 1);

	label = new QLabel(page);
	sizePolicy.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
	label->setSizePolicy(sizePolicy);
	label->setFrameShape(QFrame::NoFrame);
	label->setFrameShadow(QFrame::Plain);
	label->setScaledContents(false);
	label->setAlignment(Qt::AlignTop);
	label->setWordWrap(false);

	gridLayout->addWidget(label, 0, 0, 5, 1);
	page_config = addPage(page, i18n("Configuration File"));

	page = new QWidget(this);
	gridLayout = new QGridLayout(page);
	gridLayout->setSpacing(6);
	gridLayout->setMargin(11);
	gridLayout->setContentsMargins(0, 0, 0, 0);

	QHBoxLayout *hboxLayout = new QHBoxLayout();
	hboxLayout->setSpacing(6);
	label = new QLabel(page);
	label->setText(i18n("Your default key:"));

	hboxLayout->addWidget(label);

	CBdefault = new KComboBox(page);
	QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Fixed);
	sizePolicy1.setHorizontalStretch(0);
	sizePolicy1.setVerticalStretch(0);
	sizePolicy1.setHeightForWidth(CBdefault->sizePolicy().hasHeightForWidth());
	CBdefault->setSizePolicy(sizePolicy1);

	hboxLayout->addWidget(CBdefault);

	gridLayout->addLayout(hboxLayout, 0, 1, 1, 1);

	spacerItem = new QSpacerItem(20, 30, QSizePolicy::Minimum, QSizePolicy::Expanding);

	gridLayout->addItem(spacerItem, 1, 1, 1, 1);

	page_defaultkey = addPage(page, i18n("Default Key"));

	page = new QWidget(this);

	gridLayout = new QGridLayout(page);
	gridLayout->setSpacing(6);
	gridLayout->setMargin(11);
	gridLayout->setContentsMargins(0, 0, 0, 0);

	binlabel = new QLabel(page);

	gridLayout->addWidget(binlabel, 0, 1, 1, 1);

	versionLabel = new QLabel(page);

	gridLayout->addWidget(versionLabel, 1, 1, 1, 1);

	defaultkeylabel = new QLabel(page);

	gridLayout->addWidget(defaultkeylabel, 2, 1, 1, 1);

	generateCB = new QCheckBox(page);
	generateCB->setText(i18n("Generate new key"));

	gridLayout->addWidget(generateCB, 3, 1, 1, 1);

	spacerItem = new QSpacerItem(20, 30, QSizePolicy::Minimum, QSizePolicy::Expanding);

	gridLayout->addItem(spacerItem, 4, 1, 1, 1);

	autostartCB = new QCheckBox(page);
	autostartCB->setChecked(true);
	autostartCB->setText(i18n("Start KGpg automatically at KDE startup."));

	gridLayout->addWidget(autostartCB, 5, 1, 1, 1);

	page_done = addPage(page, i18n("Done"));
}

void
KGpgFirstAssistant::findConfigPath()
{
	const QString gpgHome = GPGProc::getGpgHome(binURL->url().path());
	QString confPath = gpgHome + QLatin1String( "gpg.conf" );

	if (!QFile(confPath).exists()) {
		confPath = gpgHome + QLatin1String( "options" );
		if (!QFile(confPath).exists()) {
			if (KMessageBox::questionYesNo(0, i18n("<qt><b>The GnuPG configuration file was not found</b>. Should KGpg try to create a config file ?</qt>"), QString(), KGuiItem(i18n("Create Config")), KGuiItem(i18n("Do Not Create"))) == KMessageBox::Yes) {
				confPath = gpgHome + QLatin1String( "gpg.conf" );
				QFile file(confPath);
				if (file.open(QIODevice::WriteOnly)) {
				    QTextStream stream(&file);
				    stream << "# GnuPG config file created by KGpg" << "\n";
				    file.close();
				}
			} else {
				text_optionsfound->setText(i18n("<qt><b>The GnuPG configuration file was not found</b>.</qt>"));
				confPath.clear();
			}
		}
	}

	pathURL->setUrl(confPath);

	QStringList secids = KgpgInterface::readSecretKeys();
	if (secids.isEmpty()) {
		setAppropriate(page_defaultkey, false);
		generateCB->setChecked(true);
		defaultkeylabel->setVisible(false);
		return;
	}

	KgpgKeyList publiclist = KgpgInterface::readPublicKeys(secids);

	generateCB->setChecked(false);
	setAppropriate(page_defaultkey, true);

	CBdefault->clear();

	foreach (const KgpgKey &k, publiclist) {
		QString s;

		if (k.email().isEmpty())
			s = i18nc("Name: ID", "%1: %2", k.name(), k.id());
		else
			s = i18nc("Name (Email): ID", "%1 (%2): %3", k.name(), k.email(), k.id());

		CBdefault->addItem(s, k.fingerprint());
	}

	CBdefault->setCurrentIndex(0);
}

void
KGpgFirstAssistant::next()
{
	if (currentPage() == page_binary) {
		binlabel->setText(i18n("Your GnuPG binary is: %1", binURL->url().path()));
		findConfigPath();
	} else if (currentPage() == page_config) {
		QString tst, name;
		m_confPath = pathURL->url().path();

		QString defaultID = KgpgInterface::getGpgSetting(QLatin1String( "default-key" ), m_confPath);

		if (!defaultID.isEmpty()) {
			for (int i = 0; i < CBdefault->count(); i++) {
				if (defaultID == CBdefault->itemData(i).toString().right(defaultID.length())) {
					CBdefault->setCurrentIndex(i);
					break;
				}
			}
		}
		versionLabel->setText(i18n("You have GnuPG version: %1", m_gpgVersion));
	} else if (currentPage() == page_defaultkey) {
		defaultkeylabel->setVisible(true);
		defaultkeylabel->setText(i18n("Your default key is: %1", CBdefault->currentText()));
		int i = CBdefault->currentIndex();
		if (i >= 0) {
			defaultkeylabel->setToolTip(CBdefault->itemData(i).toString());
		}
	}
	KAssistantDialog::next();
}

bool
KGpgFirstAssistant::runKeyGenerate() const
{
	return generateCB->isChecked();
}

QString
KGpgFirstAssistant::getConfigPath() const
{
	return m_confPath;
}

QString
KGpgFirstAssistant::getDefaultKey() const
{
	int i = CBdefault->currentIndex();
	if (i < 0)
		return QString();
	else
		return CBdefault->itemData(i).toString();
}

bool
KGpgFirstAssistant::getAutoStart() const
{
	return autostartCB->isChecked();
}

void
KGpgFirstAssistant::slotBinaryChanged(const QString &binary)
{
	if (binary.isEmpty()) {
		setValid(page_binary, false);
		return;
	}

	m_gpgVersion = GPGProc::gpgVersionString(binary);
	setValid(page_binary, !m_gpgVersion.isEmpty());
	if (!m_gpgVersion.isEmpty()) {
		const int gpgver = GPGProc::gpgVersion(m_gpgVersion);

		if (gpgver < 0x10400) {
			txtGpgVersion->setText(i18n("Your GnuPG version (%1) seems to be too old.<br />Compatibility with versions before 1.4.0 is no longer guaranteed.", m_gpgVersion));
		} else {
			txtGpgVersion->setText(i18n("You have GnuPG version: %1", m_gpgVersion));
		}
	}
}

#include "kgpgfirstassistant.moc"
