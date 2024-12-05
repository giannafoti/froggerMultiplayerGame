#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <QDialog>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QMap>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QUdpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include "player.h"
#include "dialog.h"
#include "obstacles.h"

class GraphicsDialog : public QDialog {
    Q_OBJECT

public:
    explicit GraphicsDialog(QWidget *parent = nullptr, QUdpSocket *socket = nullptr);
    void closeEvent(QCloseEvent *event) override;
    ~GraphicsDialog();
    void addActivePlayer(int clientId, QString username, const QColor &color);
    void addPlayer(int clientId, QString username, const QColor &color);
    void removePlayer(int clientId);
    void removePlayerFromScene(int clientId);
    void updatePlayerPositions(QJsonArray &playersArray);
    void setPlayerState(QJsonObject clientData);
    void handleLevelOver();
    void handleGameOver();
    int score = 0;
    QGraphicsTextItem *header,*display, *endText;

private:
    void keyPressEvent(QKeyEvent *e) override;
    QGraphicsScene *scene;
    Player *activePlayer;
    QMap<int, Player*> clientPlayers;
    QUdpSocket *socket;
    QString serverIp;
    quint16 serverPort;
    QMap<int, Obstacle*> obstacles;
    QList<QGraphicsRectItem*> boundingLines;
    QList<QGraphicsItem *> obstacleList;
    QList<QGraphicsPixmapItem*> hearts;
    QGraphicsRectItem *overlay;

    void createBoundingLine(int x, int y, int width, int height);
    int levelCount = 0;
    void initializeHearts();
    void removeHeart();
    void sendScoreToServer();
    int obstacleId = 0;

    QGraphicsPixmapItem* endScreen;

    bool activeGameState=true;

    QGraphicsRectItem *overlay;
    int level = 1;
    int winnerClientId = 0;

public slots:
    void addActivePlayer(int clientId, QString username, const QColor &color);
    void addPlayer(int clientId, QString username, const QColor &color);
    void removePlayer(int clientId);
    void removePlayerFromScene(int clientId);
    void updatePlayerPositions(QJsonArray &playersArray);
    void createObstacle(Obstacle::ObstacleType type, int x, int y, int speed, bool facingLeft = false);
    void drawScoreDisplay();
    void reachGoalScreen();
    void handleWaterDeath();
    void showWaterDeathScreen();
    void showEndScreen();
    void handlePlayerDeath();
    void startNextLevel();
    
    int numLives = 3;
    int obstacleId = 0;
    int level = 1;

    bool levelOver;
    bool activeGameState=true;

private slots:
    void checkCollisions();

    int numLives = 3;
    int obstacleId = 0;
    int level = 1;

    bool levelOver;
    bool activeGameState=true;

private slots:
    void checkCollisions();

signals:
    void requestClose();
};

#endif // GRAPHICS_H
