/*
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NATIVEMESSAGINGPROXY_H
#define NATIVEMESSAGINGPROXY_H

#include <QDataStream>
#include <QObject>
#include <QLocalSocket>
#include <QPointer>

class QWinEventNotifier;
class QSocketNotifier;

class NativeMessagingProxy : public QObject
{
    Q_OBJECT
public:
    NativeMessagingProxy();
    ~NativeMessagingProxy() override;

signals:
    void stdinMessage(QString msg);

public slots:
    void newLocalMessage();
    void deleteSocket();
    void processStandardInput(const QString& msg);

private:
    void setupStandardInput();
    void setupLocalSocket();
    void readStdin();

private:
    bool m_running = false;
    QPointer<QLocalSocket> m_localSocket;
    QDataStream m_stdout;

#ifdef Q_OS_WIN
    QPointer<QWinEventNotifier> m_notifier;
#else
    QPointer<QSocketNotifier> m_notifier;
#endif

    Q_DISABLE_COPY(NativeMessagingProxy)
};

#endif // NATIVEMESSAGINGPROXY_H
