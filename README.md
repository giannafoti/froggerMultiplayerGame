# Fast and Froggy: User Manual

## Initial Setup
Fast and Froggy utilizes a client-server architecture. Either app may be executed first, revealing the server/client dialogs, but a server must be configured prior to accepting any client connections. Up to 4 client players may connect to a single server game host.

### Server Configuration
- In QtCreator, open the `server.pro` project file within the server directory.
- Clean and run the program.
- The IP address and port fields are filled automatically with your local information.
- The IP address and port fields may be edited and the server reconfigured.
- Press the configure button to start the server at the specified IP address and port.

### Client Configuration
- In QtCreator, open the `client.pro` project file within the client directory.
- Clean and run the program.
- The IP address and port fields are filled automatically with your local information.
- The IP address and port fields may be edited and the client reconfigured.
- Press the configure button to connect to the server at the specified IP address and port.

## Game Objectives
- Score the most points to win! Doesn’t matter if it's by one point or one hundred points. Winning is winning.
- Each player has three lives.
- Reach the lily pad without hitting an obstacle or falling in the water to finish the level.
- Player resets to the initial position and the level increments with obstacle speed increasing.
- The higher you place in the level, the more points you earn.
- Levels continue indefinitely until all players have lost all of their lives.
- Players with the top 10 highest scores of all time will be featured on the leaderboard.

## Scoring Points
- Forward movement: 10 points per movement.
- Finishing the level: 250, 200, 150, 100 points for first, second, third, and fourth placement.

## Features

### Server
- Disconnecting the server by closing the server dialog or configuring a new server results in the disconnection of all connected clients, prompting them to reconnect.
- The Setup/Log tab shows a running log of game events and client activity (client connection/disconnection, messages, level increment, game over, etc.).
- The Manage tab displays the connected clients as enabled buttons, their username, and their color.
- The buttons on the Manage tab allow the server host to kick clients off the game server, helping handle crashed clients or clients away from their keyboard.
- The server may also forcefully start a game for all connected clients if the ready-up feature is ignored.

### Client
- The Game tab shows the log of game activities updated from the server and can act as an in-game chat, sending messages by typing in the line edit and pressing the Enter key.
- Player Profile is also displayed on the Game tab, allowing the client to submit a username and select their frog color.
  - The default username is `Client #` and the default color is white if none is selected.
- The Leaderboard tab shows the game information (timestamp, score, levels played, etc.) for the top 10 highest scores of all time from the SQL database.
- The History tab:
  - With the **All Players** radio button selected, shows game information from most recent to least recent.
  - With the **Current User** radio button selected, shows game information from most recent to least recent matching the client’s username (if applicable).
- Closing the client dialog closes the game dialog (if active), removes the player from the game, and disconnects them from the server.
- Closing the game dialog removes the player from the game but keeps them connected to the server in the game lobby.
- Selected colors by other clients will be displayed as disabled buttons so that no two players are the same color.
- Submitting a username of another active client will be rejected.
- Once all clients connected to a server check the `Ready?` checkbox, a game will start automatically.
