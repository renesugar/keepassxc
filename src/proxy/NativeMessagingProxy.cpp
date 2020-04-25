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

#include "NativeMessagingProxy.h"
#include "browser/BrowserShared.h"

#include <QCoreApplication>
#include <QtConcurrent/QtConcurrent>

#include <iostream>

//#if defined(Q_OS_UNIX) && !defined(Q_OS_LINUX)
//#include <sys/event.h>
//#include <sys/time.h>
//#include <sys/types.h>
//#include <unistd.h>
//#endif

//#ifdef Q_OS_LINUX
//#include <sys/epoll.h>
//#include <unistd.h>
//#endif

#ifdef Q_OS_WIN
#include <fcntl.h>
#include <windows.h>
#endif

NativeMessagingProxy::NativeMessagingProxy()
    : QObject()
{
    connect(this,
            &NativeMessagingProxy::stdinMessage,
            this,
            &NativeMessagingProxy::processStandardInput,
            Qt::QueuedConnection);

    setupStandardInput();
    setupLocalSocket();
}

NativeMessagingProxy::~NativeMessagingProxy()
{

}

void NativeMessagingProxy::newLocalMessage()
{
    if (!m_localSocket || m_localSocket->bytesAvailable() <= 0) {
        return;
    }

    auto msg = m_localSocket->readAll();
    if (!msg.isEmpty()) {
        uint len = msg.size();
        std::cout << char(((len >> 0) & 0xFF)) << char(((len >> 8) & 0xFF)) << char(((len >> 16) & 0xFF))
                  << char(((len >> 24) & 0xFF));
        std::cout << msg.toStdString() << std::flush;
    }
}

void NativeMessagingProxy::deleteSocket()
{
    if (m_notifier) {
        m_notifier->setEnabled(false);
    }
    m_localSocket->deleteLater();
    QCoreApplication::quit();
}

void NativeMessagingProxy::processStandardInput(const QString& msg)
{
    if (m_localSocket && m_localSocket->state() == QLocalSocket::ConnectedState) {
        m_localSocket->write(msg.toUtf8(), msg.length());
        m_localSocket->flush();
    }
}

void NativeMessagingProxy::readStdin()
{
    unsigned int length = 0;
    for (int i = 0; i < 4; ++i) {
        length |= getchar() << (i * 8);
    }

    QString msg;
    for (unsigned int i = 0; i < length; ++i) {
        msg += getchar();
    }
    emit stdinMessage(msg);
}

void NativeMessagingProxy::setupStandardInput()
{
    if (m_running) {
        return;
    }
    m_running = true;

#ifdef Q_OS_WIN
    setmode(fileno(stdin), _O_BINARY);
    setmode(fileno(stdout), _O_BINARY);
#endif

    QtConcurrent::run([this] {
        while (m_running && std::cin.good()) {
            if (std::cin.peek() != EOF) {
                readStdin();
            }
            QThread::msleep(100);
        }
        QCoreApplication::quit();
    });

    auto fd = new QFile();
    fd->open(stdout, QIODevice::WriteOnly);
    m_stdout.setDevice(fd);
}

void NativeMessagingProxy::setupLocalSocket()
{
    m_localSocket = new QLocalSocket();
    m_localSocket->connectToServer(Browser::localServerPath());
    m_localSocket->setReadBufferSize(Browser::NATIVEMSG_MAX_LENGTH);

    connect(m_localSocket, SIGNAL(readyRead()), this, SLOT(newLocalMessage()));
    connect(m_localSocket, SIGNAL(disconnected()), this, SLOT(deleteSocket()));
}
