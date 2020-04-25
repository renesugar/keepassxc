/*
 *  Copyright (C) 2017 Sami Vänttinen <sami.vanttinen@protonmail.com>
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

#include "NativeMessagingHost.h"
#include "BrowserSettings.h"
#include "BrowserShared.h"

#include <QJsonDocument>
#include <QMutexLocker>
#include <QtNetwork>

#include "sodium.h"
#include <iostream>

NativeMessagingHost::NativeMessagingHost(DatabaseTabWidget* parent)
    : QObject(parent)
    , m_browserService(parent)
    , m_browserClients(m_browserService)
{
    m_localServer = new QLocalServer(this);
    m_localServer->setSocketOptions(QLocalServer::UserAccessOption);
    connect(m_localServer.data(), SIGNAL(newConnection()), this, SLOT(newLocalConnection()));

    if (browserSettings()->isEnabled()) {
        start();
    }

    connect(&m_browserService, SIGNAL(databaseLocked()), this, SLOT(databaseLocked()));
    connect(&m_browserService, SIGNAL(databaseUnlocked()), this, SLOT(databaseUnlocked()));
}

NativeMessagingHost::~NativeMessagingHost()
{
    stop();
}

void NativeMessagingHost::start()
{
    if (sodium_init() == -1) {
        qWarning() << "Failed to start browser service: libsodium failed to initialize!";
        return;
    }

    // Update KeePassXC/keepassxc-proxy binary paths to Native Messaging scripts
    if (browserSettings()->updateBinaryPath()) {
        browserSettings()->updateBinaryPaths(
            browserSettings()->useCustomProxy() ? browserSettings()->customProxyLocation() : "");
    }

    m_localServer->listen(Browser::localServerPath());
}

void NativeMessagingHost::stop()
{
    databaseLocked();
    m_socketList.clear();
    m_localServer->close();
}

void NativeMessagingHost::newLocalConnection()
{
    QLocalSocket* socket = m_localServer->nextPendingConnection();
    if (socket) {
        connect(socket, SIGNAL(readyRead()), this, SLOT(newLocalMessage()));
        connect(socket, SIGNAL(disconnected()), this, SLOT(disconnectSocket()));
    }
}

void NativeMessagingHost::newLocalMessage()
{
    QLocalSocket* socket = qobject_cast<QLocalSocket*>(QObject::sender());
    if (!socket || socket->bytesAvailable() <= 0) {
        return;
    }

    socket->setReadBufferSize(Browser::NATIVEMSG_MAX_LENGTH);

    QByteArray arr = socket->readAll();
    if (arr.isEmpty()) {
        return;
    }

    if (!m_socketList.contains(socket)) {
        m_socketList.push_back(socket);
    }

    QString reply(QJsonDocument(m_browserClients.readResponse(arr)).toJson(QJsonDocument::Compact));
    if (socket && socket->isValid() && socket->state() == QLocalSocket::ConnectedState) {
        arr = reply.toUtf8();
        socket->write(arr.constData(), arr.length());
        socket->flush();
    }
}

void NativeMessagingHost::sendReplyToAllClients(const QJsonObject& json)
{
    QString reply(QJsonDocument(json).toJson(QJsonDocument::Compact));
    for (const auto socket : m_socketList) {
        if (socket && socket->isValid() && socket->state() == QLocalSocket::ConnectedState) {
            QByteArray arr = reply.toUtf8();
            socket->write(arr.constData(), arr.length());
            socket->flush();
        }
    }
}

void NativeMessagingHost::disconnectSocket()
{
    QLocalSocket* socket(qobject_cast<QLocalSocket*>(QObject::sender()));
    for (auto s : m_socketList) {
        if (s == socket) {
            m_socketList.removeOne(s);
        }
    }
}

void NativeMessagingHost::databaseLocked()
{
    QJsonObject response;
    response["action"] = QString("database-locked");
    sendReplyToAllClients(response);
}

void NativeMessagingHost::databaseUnlocked()
{
    QJsonObject response;
    response["action"] = QString("database-unlocked");
    sendReplyToAllClients(response);
}
