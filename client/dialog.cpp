#include "dialog.h"
#include "ui_dialog.h"
#include "graphicsdialog.h"
#include "defs.h"

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);

    setLocalIpAddress();
    ui->portEdit->setText(QString::number(DEFAULT_PORT));  // default port set manually

    ui->textBrowser->append("Server should send a welcome message and clear the inputs on successful connection to active game server.");

    connect(ui->usernameEdit, &QLineEdit::returnPressed, this, &Dialog::onSubmitButtonClick);
    connect(ui->messageEdit, &QLineEdit::returnPressed, this, &Dialog::onSendButtonClick);
    connect(ui->connectButton, &QPushButton::clicked, this, &Dialog::connectToServer);
    connect(ui->submitButton, &QPushButton::clicked, this, &Dialog::submitUsername);

    connect(ui->greenButton, &QPushButton::clicked, this, &Dialog::onColorButtonClick);
    connect(ui->blueButton, &QPushButton::clicked, this, &Dialog::onColorButtonClick);
    connect(ui->yellowButton, &QPushButton::clicked, this, &Dialog::onColorButtonClick);
    connect(ui->redButton, &QPushButton::clicked, this, &Dialog::onColorButtonClick);

    connect(ui->allPlayersButton, &QPushButton::clicked, this, &Dialog::showAllSessions);
    connect(ui->currentUserButton, &QPushButton::clicked, this, &Dialog::showSessionsForUser);

    connect(ui->tabWidget, &QTabWidget::tabBarClicked, this, &Dialog::onTabClicked);

    db = QSqlDatabase::addDatabase("QSQLITE");  //creates a DB connection
    db.setDatabaseName("../froggy_game_data.db");
    if (!db.open()) {
        qDebug() << "Error opening database: " << db.lastError().text();
    } else {
        showAllSessions();
        showLeaderboard();
    }
}

void Dialog::onTabClicked()
{
    if (ui->allPlayersButton->isChecked())
    {
        qmHistory->setQuery("SELECT timestamp, winner_username, high_score, max_level FROM games ORDER BY timestamp DESC");
    }
    else if (ui->currentUserButton->isChecked() && !playerUsername.isEmpty())
    {
        QString filteredQuery = QString("SELECT "
                                        "g.timestamp, "
                                        "s.score, "
                                        "s.levels_played, "
                                        "CASE "
                                        "    WHEN g.winner_username = s.player_username THEN 'true' "
                                        "    ELSE 'false' "
                                        "END AS is_winner, "
                                        "g.id AS game_id "
                                        "FROM games g "
                                        "JOIN sessions s ON g.id = s.game_id "
                                        "WHERE s.player_username = '%1' "
                                        "ORDER BY g.timestamp DESC").arg(playerUsername);
        qmHistory->setQuery(filteredQuery);
    }
    qmLeaderboard->setQuery("SELECT player_username, score, levels_played, game_id FROM sessions ORDER BY score DESC LIMIT 10");
    ui->leaderboardTableView->resizeColumnsToContents();
    ui->historyTableView->resizeColumnsToContents();
}

void Dialog::showLeaderboard()
{
    qmLeaderboard = new QSqlQueryModel;
    qmLeaderboard->setQuery("SELECT player_username, score, levels_played, game_id FROM sessions ORDER BY score DESC LIMIT 10");

    if (qmLeaderboard->lastError().isValid())
    {
        qDebug() << "Error in query: " << qmLeaderboard->lastError().text();
    }
    ui->leaderboardTableView->setModel(qmLeaderboard);
    ui->leaderboardTableView->resizeColumnsToContents();
    ui->historyTableView->resizeColumnsToContents();
}

void Dialog::showAllSessions()
{
    qmHistory = new QSqlQueryModel;
    qmHistory->setQuery("SELECT timestamp, winner_username, high_score, max_level FROM games ORDER BY timestamp DESC");

    if (qmHistory->lastError().isValid())
    {
        qDebug() << "Error in query: " << qmHistory->lastError().text();
    }

    ui->historyTableView->setModel(qmHistory);
    ui->historyTableView->resizeColumnsToContents();
}

void Dialog::showSessionsForUser()
{
    if (!playerUsername.isEmpty())
    {
        QString filteredQuery = QString("SELECT "
                                        "g.timestamp, "
                                        "s.score, "
                                        "s.levels_played, "
                                        "CASE "
                                        "    WHEN g.winner_username = s.player_username THEN 'true' "
                                        "    ELSE 'false' "
                                        "END AS is_winner, "
                                        "g.id AS game_id "
                                        "FROM games g "
                                        "JOIN sessions s ON g.id = s.game_id "
                                        "WHERE s.player_username = '%1' "
                                        "ORDER BY g.timestamp DESC").arg(playerUsername);
        qmHistory->setQuery(filteredQuery);

        if (qmHistory->lastError().isValid())
        {
            qDebug() << "Error in query: " << qmHistory->lastError().text();
        }
        ui->historyTableView->resizeColumnsToContents();
    }
}

void Dialog::connectToServer()
{
    bool ipHasText = !ui->ipEdit->text().isEmpty();
    bool portHasText = !ui->portEdit->text().isEmpty();

    if (!ipHasText || !portHasText)
    {
        return;
    }

    socket = new QUdpSocket(this);

    ip = ui->ipEdit->text();
    port = ui->portEdit->text().toUInt();

    socket->connectToHost(QHostAddress(ip), port);
    connect(socket, &QUdpSocket::readyRead, this, &Dialog::processMsg);

    if (socket->isValid())
    {
        QJsonObject connectMessage;
        connectMessage["type"] = "CONNECT";

        sendJson(connectMessage);

        connect(ui->sendButton, &QPushButton::clicked, this, &Dialog::sendMsg);
    }
}

void Dialog::closeEvent(QCloseEvent *event)
{
    if (db.isOpen())
    {
        db.close();
    }

    if (validConnection)
    {
        leaveGame();
        disconnectFromServer();
    }

    event->accept();
}

void Dialog::disconnectFromServer()
{
    QJsonObject disconnectMessage;

    disconnectMessage["type"] = "DISCONNECT";
    sendJson(disconnectMessage);

    socket->disconnectFromHost();
    delete socket;
}

void Dialog::leaveGame()
{ 
    QJsonObject disconnectMessage;

    disconnectMessage["type"] = "LEAVE";
    sendJson(disconnectMessage);

    if (graphicsDialog)
    {
        graphicsDialog->removePlayer(activeClientId);
        graphicsDialog->close();
        delete graphicsDialog;
        graphicsDialog = nullptr;
    }
    ui->submitButton->setEnabled(true);
}

void Dialog::onSubmitButtonClick()
{
    ui->submitButton->click();
}

void Dialog::onSendButtonClick()
{
    ui->sendButton->click();
}

void Dialog::sendMsg()
{
    QString msg = ui->messageEdit->text();
    QJsonObject jsonMessage;
    jsonMessage["type"] = "MESSAGE";
    jsonMessage["message"] = msg;
    sendJson(jsonMessage);
    ui->messageEdit->clear();
}

void Dialog::sendJson(QJsonObject data)
{
//    qDebug() << data;
    QJsonDocument doc(data);
    QByteArray ba = doc.toJson();

    socket->writeDatagram(ba, QHostAddress(ip), port);
}

void Dialog::processMsg()
{
//    qDebug() << "processing message";
    while (socket->hasPendingDatagrams())
    {
        QByteArray ba;
        ba.resize(socket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        socket->readDatagram(ba.data(), ba.size(), &sender, &senderPort);

        QJsonDocument doc = QJsonDocument::fromJson(ba);
        if (!doc.isObject()) return;

        QJsonObject jsonObj = doc.object();
        QString type = jsonObj["type"].toString();
        QString message = jsonObj["message"].toString();

        qDebug() << jsonObj;

        if (type == "WELCOME")
        {
            validConnection = true;
            ui->textBrowser->setText("Connected to Server: " + ip + ":" + QString::number(port));
            ui->ipEdit->clear();
            ui->portEdit->clear();
            activeClientId = parseClientIdFromMsg(message);
            ui->textBrowser->append("Server: " + message);
            ui->sendButton->setEnabled(true);
            ui->connectButton->setEnabled(false);
            ui->submitButton->setEnabled(true);
            ui->greenButton->setEnabled(true);
            ui->blueButton->setEnabled(true);
            ui->yellowButton->setEnabled(true);
            ui->redButton->setEnabled(true);
            ui->greenButton->setStyleSheet("background-color: green");
            ui->blueButton->setStyleSheet("background-color: blue");
            ui->yellowButton->setStyleSheet("background-color: yellow");
            ui->redButton->setStyleSheet("background-color: red");
            ui->currentUserButton->setEnabled(true);
        }
        else if (type == "START")
        {
            if (!graphicsDialog)
            {
                graphicsDialog = new GraphicsDialog(this);
                graphicsDialog->show();
                connect(graphicsDialog, &GraphicsDialog::requestClose, this, &Dialog::leaveGame);
            }

            if (activeClientId != -1 && graphicsDialog)
            {
                QColor color;
                if (!playerColor.isEmpty())
                {
                    color = QColor(playerColor);
                }
                else
                {
                    color = QColor("white");
                }

                graphicsDialog->addActivePlayer(activeClientId, playerUsername, color);
//                qDebug() << "add player " << activeClientId;
            }
            ui->submitButton->setEnabled(false);
            ui->textBrowser->append(message);
        }
        else if (type == "DISCONNECT_ALL")
        {
            validConnection = false;
            leaveGame();
            ui->textBrowser->clear();
            ui->connectButton->setEnabled(true);
            ui->sendButton->setEnabled(false);
            ui->submitButton->setEnabled(false);
            ui->greenButton->setEnabled(false);
            ui->blueButton->setEnabled(false);
            ui->yellowButton->setEnabled(false);
            ui->redButton->setEnabled(false);
            ui->greenButton->setStyleSheet("");
            ui->blueButton->setStyleSheet("");
            ui->yellowButton->setStyleSheet("");
            ui->redButton->setStyleSheet("");
            ui->textBrowser->append(message);
            setLocalIpAddress();
            ui->portEdit->setText(QString::number(DEFAULT_PORT));
            ui->currentUserButton->setEnabled(false);
        }
        else if (type == "REJECTION")
        {
            qDebug() << "rejection";
            ui->textBrowser->append("Server: " + message);
//            disconnectFromServer();
        }
        else if (type == "ACTIVE_CLIENTS")
        {
            // process the list of active client IDs received from the server
            QJsonArray clientIdsArray = jsonObj["clientIdsInGame"].toArray();
            clientDataArray = jsonObj["clientData"].toArray();

            // set of current active client IDs received from the server
            QSet<int> newActiveClients;

            QMap<QString, QPushButton*> colorButtonMap = {
                {"green", ui->greenButton},
                {"blue", ui->blueButton},
                {"yellow", ui->yellowButton},
                {"red", ui->redButton},
            };

            for (const QJsonValue &value : clientDataArray)
            {
                clientData = value.toObject();
                int clientId = clientData["clientId"].toInt();
                QString username = clientData["username"].toString();
                QString colorString = clientData["color"].toString();

                if (clientId == activeClientId)
                {
                    playerUsername = username;
                }

                // track taken colors
                if (clientId != activeClientId && !colorString.isEmpty())
                {
                    clientColors[clientId] = colorString;
                }

                if (clientIdsArray.contains(clientId))
                {
                    if (colorString.isEmpty())
                    {
                        colorString = "white";
                    }

                    newActiveClients.insert(clientId);
                    if (graphicsDialog)
                    {
                        if (!activeClients.contains(clientId) && clientData["isAlive"].toBool())
                        {
                            graphicsDialog->addPlayer(clientId, username, QColor(colorString));
                        }
                        graphicsDialog->setPlayerState(clientData);
                        graphicsDialog->checkRoundOver();
                    }
                }

            }

            // enable/disable color buttons based on whether the color is taken
            for (const QString &color : colorButtonMap.keys())
            {
               bool colorTaken = false;

               for (int clientId : clientColors.keys())
               {
                   if (clientColors[clientId] == color)
                   {
                       colorTaken = true;
                       break;
                   }
               }

               if (colorTaken)
               {
                   colorButtonMap[color]->setEnabled(false);
                   colorButtonMap[color]->setStyleSheet("");
               }
               else
               {
                   colorButtonMap[color]->setEnabled(true);
                   colorButtonMap[color]->setStyleSheet("background-color: " + color);
               }
            }

            // check for players that are no longer active
            for (int clientId : activeClients)
            {
                if (graphicsDialog)
                {
                    if (!newActiveClients.contains(clientId))
                    {
                        qDebug() << "removing player " << clientId;
                        graphicsDialog->removePlayer(clientId);
                        graphicsDialog->setPlayerState(clientData);
                        graphicsDialog->checkRoundOver();
                    }
                    else if (!isPlayerAlive(clientId))
                    {
                        qDebug() << "removing dead player " << clientId;
                        graphicsDialog->removePlayerFromScene(clientId);
                        graphicsDialog->setPlayerState(clientData);
                        graphicsDialog->checkRoundOver();
                    }
                }
            }

            activeClients = newActiveClients;

//            ui->textBrowser->append("Server: Active clients updated.");
        }
        else if (type == "POSITION")
        {
//            qDebug() << "updating player positions";
            QJsonArray playersArray = jsonObj["players"].toArray();
            if (graphicsDialog)
            {
                graphicsDialog->updatePlayerPositions(playersArray);
            }
        }
        else if (type == "LEVEL_OVER")
        {
            if (graphicsDialog)
            {
                graphicsDialog->handleLevelOver();
            }
            ui->textBrowser->append(message);
        }
        else
        {
            ui->textBrowser->append(message);
        }
    }
}

bool Dialog::isPlayerAlive(int clientId)
{
    for (const QJsonValue &value : clientDataArray)
    {
        QJsonObject clientData = value.toObject();

        if (clientData["clientId"].toInt() == clientId)
        {
            return clientData["isAlive"].toBool();
        }
    }

    return false;
}

int Dialog::parseClientIdFromMsg(const QString &msg)
{
    QRegExp regex("You are Client (\\d+)");
    if (regex.indexIn(msg) != -1)
    {
        return regex.cap(1).toInt();
    }
    return -1;
}

void Dialog::onColorButtonClick()
{
    QPushButton* button = static_cast<QPushButton*>(sender());

    if (!button) return;

    QString color;

    if (button == ui->greenButton) {
        color = "green";
    } else if (button == ui->blueButton) {
        color = "blue";
    } else if (button == ui->yellowButton) {
        color = "yellow";
    } else if (button == ui->redButton) {
        color = "red";
    }

    setPlayerColor(color);
}

void Dialog::setPlayerColor(QString &color)
{
    playerColor = color;

    QJsonObject colorMessage;

    colorMessage["type"] = "PLAYER_COLOR";
    colorMessage["clientId"] = activeClientId;
    colorMessage["color"] = playerColor;

    sendJson(colorMessage);

    ui->textBrowser->append("You selected the color: " + playerColor);
}

void Dialog::submitUsername()
{
    playerUsername = ui->usernameEdit->text();

    if (playerUsername.isEmpty())
    {
        return;
    }

    QJsonObject usernameMessage;
    usernameMessage["type"] = "USERNAME";
    usernameMessage["clientId"] = activeClientId;
    usernameMessage["username"] = playerUsername;

    sendJson(usernameMessage);

    ui->textBrowser->append("You submitted the username: " + playerUsername);
}

void Dialog::sendPlayerPosition(int clientId, qreal x, qreal y)
{
//    qDebug() << "sending player position";

    QJsonObject message;
    message["type"] = "POSITION";
    QJsonArray playerPosArray;

    QJsonObject playerPosData;
    playerPosData["clientId"] = clientId;
    playerPosData["x"] = x;
    playerPosData["y"] = y;

    playerPosArray.append(playerPosData);
    message["players"] = playerPosArray;

    sendJson(message);
}

QString Dialog::getLocalIpAddress() {
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

    for (const QNetworkInterface &interface : interfaces) {
        if (interface.flags() & QNetworkInterface::IsUp) {
            QList<QNetworkAddressEntry> entries = interface.addressEntries();

            for (const QNetworkAddressEntry &entry : entries) {
                QHostAddress ip = entry.ip();

                if (ip.protocol() == QAbstractSocket::IPv4Protocol && ip != QHostAddress("127.0.0.1")) {
                    return ip.toString();
                }
            }
        }
    }
    return QString();
}

void Dialog::setLocalIpAddress() {
    QString localIp = getLocalIpAddress();
    if (!localIp.isEmpty()) {
        ui->ipEdit->setText(localIp);
    }
}

Dialog::~Dialog()
{
    delete ui;
}
