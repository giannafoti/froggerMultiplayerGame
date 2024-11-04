#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <QDialog>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QMap>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QUdpSocket>
#include "player.h"

class GraphicsDialog : public QDialog {
    Q_OBJECT

public:
    explicit GraphicsDialog(QWidget *parent = nullptr, QUdpSocket *socket = nullptr);
    ~GraphicsDialog();

protected:
    void keyPressEvent(QKeyEvent *e) override;
    void closeEvent(QCloseEvent *event) override;

private:
    QGraphicsScene *scene;
    Player *player;
    QMap<int, Player*> clientPlayers;
    QUdpSocket *socket;
    QString serverIp;
    quint16 serverPort;

public slots:
    void addPlayer(int clientId, const QColor &color);
    void removePlayer(int clientId);

signals:
    void requestClose();
};


#endif // GRAPHICS_H
