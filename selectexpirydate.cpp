#include "selectexpirydate.h"

#include <QCheckBox>
#include <QVBoxLayout>

#include <KDatePicker>
#include <KLocale>

SelectExpiryDate::SelectExpiryDate(QWidget *parent, QDate date)
	: KDialog(parent)
{
	setCaption(i18n("Choose New Expiration"));
	setButtons(Ok | Cancel);
	setDefaultButton(Ok);
	setModal(true);

	QWidget *page = new QWidget(this);
	m_unlimited = new QCheckBox(i18nc("Key has unlimited lifetime", "Unlimited"), page);

	if (date.isNull())
		date = QDate::currentDate();

	m_datepicker = new KDatePicker(date, page);
	if (date.isNull()) {
		m_datepicker->setEnabled(false);
		m_unlimited->setChecked(true);
	}

	QVBoxLayout *layout = new QVBoxLayout(page);
	layout->setSpacing(3);
	layout->addWidget(m_datepicker);
	layout->addWidget(m_unlimited);

	connect(m_unlimited, SIGNAL(toggled(bool)), this, SLOT(slotEnableDate(bool)));
	connect(m_datepicker, SIGNAL(dateChanged(QDate)), this, SLOT(slotCheckDate(QDate)));
	connect(m_datepicker, SIGNAL(dateEntered(QDate)), this, SLOT(slotCheckDate(QDate)));

	setMainWidget(page);
	show();
}

QDate SelectExpiryDate::date() const
{
	if (m_unlimited->isChecked())
		return QDate();
	else
		return m_datepicker->date();
}

void SelectExpiryDate::slotCheckDate(const QDate &date)
{
	enableButtonOk(date >= QDate::currentDate());
}

void SelectExpiryDate::slotEnableDate(const bool ison)
{
	m_datepicker->setEnabled(!ison);
	if (ison)
		enableButtonOk(true);
	else
		slotCheckDate(m_datepicker->date());
}

#include "selectexpirydate.moc"
