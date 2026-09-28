#ifndef ANTIGRAVITY_SERVER_H
#define ANTIGRAVITY_SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace lmms {

class AntigravityServer : public QObject {
    Q_OBJECT
public:
    explicit AntigravityServer(QObject *parent = nullptr);
    ~AntigravityServer();

    void startServer(quint16 port = 4040);

signals:
    // Signal emitted when a command is received, to be processed on the main thread
    void commandReceived(const QJsonObject& command);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onClientDisconnected();
    
    // Slot to handle the command safely on the main GUI thread
    void processCommandMainThread(const QJsonObject& command);

private:
    QTcpServer* m_server;
    QList<QTcpSocket*> m_clients;

    // Command handlers
    QJsonObject handleCreateTrack(const QJsonObject& args);
    QJsonObject handleSetTempo(const QJsonObject& args);
    QJsonObject handlePlay(const QJsonObject& args);
    QJsonObject handleStop(const QJsonObject& args);
    void sendResponse(QTcpSocket* client, const QJsonObject& response);
};

} // namespace lmms

#endif // ANTIGRAVITY_SERVER_H
