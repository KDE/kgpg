/**
 * Copyright (C) 2008 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KGPGTRANSACTION_H
#define KGPGTRANSACTION_H

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QStringList>

class GPGProc;
class KGpgTransactionPrivate;

/**
 * \brief Process one GnuPG operation
 *
 * This class encapsulates one GnuPG operation. It will care for all
 * interaction with the gpg process. Everything you have to care about
 * is to set up the object properly, call start() and catch the done signal.
 *
 * This is an abstract base class for specific operations that implements
 * the basic I/O loop, the process setup and interaction and some convenience
 * members to set extra arguments for the process.
 *
 * If you want to add a new operation create a child class that implements
 * nextLine(). Ususally you also need a constructor that takes some information
 * like the id of the key to modify.
 *
 * @author Rolf Eike Beer
 */
class KGpgTransaction: public QObject {
	Q_OBJECT
	friend class KGpgTransactionPrivate;

public:
	/**
	 * \brief KGpgTransaction constructor
	 */
	explicit KGpgTransaction(QObject *parent = 0);
	/**
	 * \brief KGpgTransaction destructor
	 */
	virtual ~KGpgTransaction();

	/**
	 * \brief Start the operation.
	 */
	void start();

Q_SIGNALS:
	/**
	 * \brief Emitted when the operation was completed. On success result is 0.
	 */
	void done(int result);

protected:
	/**
	 * \brief Called before the gpg process is started.
	 *
	 * You may reimplement this member if you need to do some special
	 * operations or cleanups before the process is started. Keep in mind
	 * that start() may be called several times. If you have some internal
	 * state you probably want to reset it here.
	 */
	virtual void preStart();
	/**
	 * \brief Called for every line the gpg process writes.
	 * @param line the input from the process
	 * @return true if "quit" should be sent to process
	 *
	 * You need to implement this member to get a usable subclass.
	 *
	 * When this function returns true "quit" is written to the process.
	 */
	virtual bool nextLine(const QString &line) = 0;
	/**
	 * \brief Called when the gpg process finishes.
	 *
	 * You may reimplement this member if you need to do some special
	 * operations after process completion. The provided one simply
	 * does nothing which should be enough for most cases.
	 */
	virtual void finish();

private:
	KGpgTransactionPrivate* const d;

	Q_PRIVATE_SLOT(d, void slotReadReady(GPGProc *))
	Q_PRIVATE_SLOT(d, void slotProcessExited(GPGProc *))

protected:
	/**
	 * \brief Ask user for passphrase and send it to gpg process.
	 *
	 * If the gpg process asks for a passphrase for authorization this
	 * function will do all necessary steps for you: ask the user for the
	 * password and write it to the gpg process. If the password is wrong
	 * the user is prompted again for the correct password.
	 *
	 * @return 0 if password was correct
	 *
	 * @see KgpgInterface::sendPassphrase
	 */
	int sendPassphrase(const QString &text, const bool isnew = false);

	/**
	 * \brief get the success value that will be returned with the done signal
	 */
	int getSuccess() const;
	/**
	 * \brief set the success value that will be returned with the done signal
	 * @param v the new success value
	 *
	 * You should use 0 as success value. Other values can be defined as needed.
	 */
	void setSuccess(const int &v);

	/**
	 * \brief add a userid hint
	 * @param txt userid description
	 *
	 * Before GnuPG asks for a passphrase it usually sends out a hint message
	 * for which key the passphrase will be needed. There may be several hint
	 * messages, e.g. if a text was encrypted with multiple keys.
	 */
	void addIdHint(QString txt);
	/**
	 * \brief get string of all userid hints
	 * @returns concatenation of all ids previously added with addIdHint().
	 */
	QString getIdHints() const;

	/**
	 * \brief get a reference to the gpg process object
	 * @returns gpg process object
	 *
	 * This returns a reference to the gpg process object used internally.
	 * In case you need to do some special things (e.g. changing the output
	 * mode) you can modify this object.
	 *
	 * Usually you will not need this.
	 *
	 * \warning Never free this object!
	 */
	GPGProc *getProcess();
	/**
	 * \brief add a command line argument to gpg process
	 * This is a convenience function that allows adding one additional
	 * argument to the command line of the process. This must be called
	 * before start() is called. Usually you will call this from your
	 * constructor.
	 */
	void addArgument(const QString &arg);
	/**
	 * \brief write data to standard input of gpg process
	 * @param a data to write
	 *
	 * Use this function to interact with the gpg process. A carriage
	 * return is appended to the data automatically. Usually you will
	 * call this function from nextLine().
	 */
	void write(const QByteArray &a);
	/**
	 * \brief ask user for password
	 * @param message message to display to the user. If message is empty
	 * "Enter password for [UID]" will be used.
	 * @return true if the authorization was successful
	 *
	 * This function handles user authorization for key changes. It will
	 * take care to display the message asking the user for the password
	 * and the number of tries left.
	 */
	 bool askPassphrase(const QString &message = QString());
};

#endif // KGPGTRANSACTION_H