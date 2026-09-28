#include "AntigravityServer.h"
#include "Engine.h"
#include "Song.h"
#include "InstrumentTrack.h"
#include "Mixer.h"
#include <QDebug>

namespace lmms {

AntigravityServer::AntigravityServer(QObject *parent) : QObject(parent), m_server(new QTcpServer(this)) {
    connect(m_server, &QTcpServer::newConnection, this, &AntigravityServer::onNewConnection);
    // Connect the signal to the main thread slot using QueuedConnection
    connect(this, &AntigravityServer::commandReceived, this, &AntigravityServer::processCommandMainThread, Qt::QueuedConnection);
}

AntigravityServer::~AntigravityServer() {
    m_server->close();
}

void AntigravityServer::startServer(quint16 port) {
    if (m_server->listen(QHostAddress::Any, port)) {
        qDebug() << "Antigravity AI Server listening on port" << port;
    } else {
        qDebug() << "Antigravity AI Server failed to start!";
    }
}

void AntigravityServer::onNewConnection() {
    QTcpSocket* client = m_server->nextPendingConnection();
    connect(client, &QTcpSocket::readyRead, this, &AntigravityServer::onReadyRead);
    connect(client, &QTcpSocket::disconnected, this, &AntigravityServer::onClientDisconnected);
    m_clients.append(client);
    qDebug() << "Antigravity Agent Connected!";
}

void AntigravityServer::onClientDisconnected() {
    QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
    if (client) {
        m_clients.removeOne(client);
        client->deleteLater();
        qDebug() << "Antigravity Agent Disconnected!";
    }
}

void AntigravityServer::onReadyRead() {
    QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    QByteArray data = client->readAll();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qDebug() << "Antigravity Server JSON Error:" << error.errorString();
        return;
    }

    if (doc.isObject()) {
        QJsonObject cmd = doc.object();
        // Since we are reading from socket, we must execute LMMS state changes on the main thread.
        // We emit the signal which is queued to processCommandMainThread
        
        // We can temporarily attach the client pointer to send response back
        cmd["_client_ptr"] = reinterpret_cast<qint64>(client);
        emit commandReceived(cmd);
    }
}

void AntigravityServer::processCommandMainThread(const QJsonObject& command) {
    QString action = command["action"].toString();
    QJsonObject args = command["args"].toObject();
    
    QJsonObject response;
    response["action"] = action;
    response["status"] = "success";

    if (action == "set_tempo") {
        response = handleSetTempo(args);
    } else if (action == "create_track") {
        response = handleCreateTrack(args);
    } else if (action == "play") {
        response = handlePlay(args);
    } else if (action == "stop") {
        response = handleStop(args);
    } else {
        response["status"] = "error";
        response["message"] = "Unknown action";
    }

    qint64 clientPtr = command["_client_ptr"].toVariant().toLongLong();
    QTcpSocket* client = reinterpret_cast<QTcpSocket*>(clientPtr);
    if (client && m_clients.contains(client)) {
        sendResponse(client, response);
    }
}

void AntigravityServer::sendResponse(QTcpSocket* client, const QJsonObject& response) {
    QJsonDocument doc(response);
    client->write(doc.toJson(QJsonDocument::Compact) + "\n");
}

QJsonObject AntigravityServer::handleSetTempo(const QJsonObject& args) {
    QJsonObject res;
    int bpm = args["bpm"].toInt(140);
    Engine::getSong()->getTempoModel()->setValue(bpm);
    res["status"] = "success";
    res["message"] = QString("Tempo set to %1").arg(bpm);
    return res;
}

QJsonObject AntigravityServer::handleCreateTrack(const QJsonObject& args) {
    QJsonObject res;
    QString type = args["type"].toString("instrument");
    
    if (type == "instrument") {
        InstrumentTrack* track = new InstrumentTrack(Engine::getSong());
        if (args.contains("name")) {
            track->setName(args["name"].toString());
        }
        Engine::getSong()->addTrack(track);
        res["status"] = "success";
        res["message"] = "Instrument track created";
    } else {
        res["status"] = "error";
        res["message"] = "Unsupported track type";
    }
    return res;
}

QJsonObject AntigravityServer::handlePlay(const QJsonObject& args) {
    QJsonObject res;
    Engine::getSong()->playSong();
    res["status"] = "success";
    return res;
}

QJsonObject AntigravityServer::handleStop(const QJsonObject& args) {
    QJsonObject res;
    Engine::getSong()->stop();
    res["status"] = "success";
    return res;
}

} // namespace lmms
