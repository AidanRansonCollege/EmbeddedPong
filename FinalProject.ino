#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LiquidCrystal.h>


const char* ssid     = "Homemade";
const char* password = "Ranson11!";

AsyncWebServer server(80);

#define ROWS 10
#define COLS 10
#define EMPTY  "."
#define BALL   "O"
#define PADDLE "X"

#define rs 33
#define enable 25
#define d0 26
#define d1 27
#define d2 14
#define d3 23
#define d4 22
#define d5 21
#define d6 19
#define d7 18

LiquidCrystal lcd(rs, enable, d0, d1, d2, d3, d4, d5, d6, d7);

int ballRow = 5, ballCol = 5;
int ballDRow = 1, ballDCol = 1;
int paddle1Row = 4;
int paddle2Row = 4;
int score1 = 0, score2 = 0;

String board[ROWS][COLS];

void buildBoard() {
  for (int r = 0; r < ROWS; r++)
    for (int c = 0; c < COLS; c++)
      board[r][c] = EMPTY;

  if (paddle1Row >= 0 && paddle1Row < ROWS)   board[paddle1Row][0]   = PADDLE;
  if (paddle1Row+1 < ROWS)                    board[paddle1Row+1][0] = PADDLE;
  if (paddle2Row >= 0 && paddle2Row < ROWS)   board[paddle2Row][9]   = PADDLE;
  if (paddle2Row+1 < ROWS)                    board[paddle2Row+1][9] = PADDLE;

  if (ballRow >= 0 && ballRow < ROWS && ballCol >= 0 && ballCol < COLS)
    board[ballRow][ballCol] = BALL;
}

String boardToJSON() {
  String json = "[";
  for (int r = 0; r < ROWS; r++) {
    json += "[";
    for (int c = 0; c < COLS; c++) {
      json += "\"" + board[r][c] + "\"";
      if (c < COLS - 1) json += ",";
    }
    json += "]";
    if (r < ROWS - 1) json += ",";
  }
  json += "]";
  return json;
}

void resetBall() {
  ballRow = 4; ballCol = 4;
  ballDRow = (random(2) == 0) ? 1 : -1;
  ballDCol = (random(2) == 0) ? 1 : -1;
  buildBoard();
}

void stepGame() {
  ballRow += ballDRow;
  ballCol += ballDCol;

  if (ballRow <= 0)        { ballRow = 0;      ballDRow =  1; }
  if (ballRow >= ROWS - 1) { ballRow = ROWS-1; ballDRow = -1; }

  if (ballCol <= 0) {
    ballCol = 0;
    buildBoard();
    if (ballRow == paddle1Row || ballRow == paddle1Row + 1) {
      ballDCol = 1;
    } else {
      delay(300);
      score2++; resetBall(); return;
    }
  }

  
  if (ballCol >= COLS - 1) {
    ballCol = COLS - 1;
    buildBoard();
    if (ballRow == paddle2Row || ballRow == paddle2Row + 1) {
      ballDCol = -1;
    } else {
      delay(300);
      score1++; resetBall(); return;
    }
  }

  buildBoard();
}

String getPage() {
  return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Pong</title>
  <style>
    @import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&display=swap');

    :root {
      --bg:     #0a0a0f;
      --ball:   #00ffcc;
      --paddle: #ff4466;
      --border: #1a1a3a;
      --text:   #c0c0e0;
      --empty:  #0d0d1a;
      --btn:    #12122a;
    }

    * { box-sizing: border-box; margin: 0; padding: 0; }

    body {
      background: var(--bg);
      color: var(--text);
      font-family: 'Share Tech Mono', monospace;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      min-height: 100vh;
      gap: 16px;
    }

    h1 {
      font-size: 1.8rem;
      letter-spacing: 0.3em;
      color: var(--ball);
      text-shadow: 0 0 20px #00ffcc66;
    }

    .scores {
      font-size: 1.2rem;
      letter-spacing: 0.15em;
    }
    .scores span { color: var(--paddle); }

    .game-row {
      display: flex;
      align-items: center;
      gap: 16px;
    }

    #grid {
      display: grid;
      grid-template-columns: repeat(10, 40px);
      grid-template-rows:    repeat(10, 40px);
      gap: 3px;
      background: var(--border);
      padding: 3px;
      border-radius: 6px;
      box-shadow: 0 0 40px #00ffcc18;
    }

    .cell {
      width: 40px; height: 40px;
      border-radius: 3px;
      background: var(--empty);
      transition: background 0.07s;
    }
    .cell.ball   { background: var(--ball);   box-shadow: 0 0 10px var(--ball); }
    .cell.paddle { background: var(--paddle); box-shadow: 0 0 8px  var(--paddle); }

    .player-ctrl {
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 10px;
    }

    .player-label {
      font-size: 0.7rem;
      letter-spacing: 0.12em;
      color: #555;
    }

    .btn-col {
      display: flex;
      flex-direction: column;
      gap: 8px;
    }

    button {
      width: 64px;
      height: 64px;
      background: var(--btn);
      color: var(--text);
      border: 1px solid #2a2a5a;
      border-radius: 8px;
      font-family: 'Share Tech Mono', monospace;
      font-size: 1.6rem;
      cursor: pointer;
      transition: background 0.12s, box-shadow 0.12s, transform 0.08s;
      user-select: none;
      -webkit-user-select: none;
    }

    button:hover  { background: #1e1e40; box-shadow: 0 0 12px #00ffcc33; }
    button:active { background: #2a2a55; transform: scale(0.94); }

    #status {
      font-size: 0.75rem;
      color: #333;
      letter-spacing: 0.1em;
    }
  </style>
</head>
<body>

  <h1>// PONG //</h1>

  <div class="scores">
    P1: <span id="s1">0</span> &nbsp;|&nbsp; P2: <span id="s2">0</span>
  </div>

  <div class="game-row">

    <div class="player-ctrl">
      <div class="btn-col">
        <button onclick="move('p1','up')">▲</button>
        <button onclick="move('p1','dn')">▼</button>
      </div>
      <div class="player-label">P1</div>
    </div>

    <div id="grid"></div>

    <div class="player-ctrl">
      <div class="btn-col">
        <button onclick="move('p2','up')">▲</button>
        <button onclick="move('p2','dn')">▼</button>
      </div>
      <div class="player-label">P2</div>
    </div>

  </div>

  <div id="status">CONNECTING...</div>

  <script>
    function renderBoard(board) {
      const grid = document.getElementById('grid');
      grid.innerHTML = '';
      for (let r = 0; r < board.length; r++) {
        for (let c = 0; c < board[r].length; c++) {
          const cell = document.createElement('div');
          cell.className = 'cell';
          const val = board[r][c];
          if (val === 'O') cell.classList.add('ball');
          if (val === 'X') cell.classList.add('paddle');
          grid.appendChild(cell);
        }
      }
    }

    function poll() {
      fetch('/state')
        .then(r => r.json())
        .then(data => {
          renderBoard(data.board);
          document.getElementById('s1').textContent = data.score1;
          document.getElementById('s2').textContent = data.score2;
          document.getElementById('status').textContent = 'LIVE';
        })
        .catch(() => {
          document.getElementById('status').textContent = 'RECONNECTING...';
        });
    }

    function move(player, dir) {
      fetch('/move?player=' + player + '&dir=' + dir);
    }

    setInterval(poll, 300);
    poll();
  </script>
</body>
</html>
)rawliteral";
}

void setup() {
  lcd.begin(16,2);
  lcd.setCursor(0,0);
  Serial.begin(9600);
  randomSeed(analogRead(0));
  buildBoard();

  WiFi.begin(ssid, password);
  lcd.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); lcd.print(".");
  }
  lcd.clear();
  lcd.println("Connected, join:");
  lcd.setCursor(0, 1);
  lcd.println(WiFi.localIP().toString());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send(200, "text/html", getPage());
  });

  server.on("/state", HTTP_GET, [](AsyncWebServerRequest* req) {
    String json = "{\"board\":" + boardToJSON()
                + ",\"score1\":" + score1
                + ",\"score2\":" + score2 + "}";
    req->send(200, "application/json", json);
  });

  server.on("/move", HTTP_GET, [](AsyncWebServerRequest* req) {
    if (req->hasParam("player") && req->hasParam("dir")) {
      String player = req->getParam("player")->value();
      String dir    = req->getParam("dir")->value();
      if (player == "p1") {
        if (dir == "up" && paddle1Row > 0)        paddle1Row--;
        if (dir == "dn" && paddle1Row < ROWS - 2) paddle1Row++;
      } else if (player == "p2") {
        if (dir == "up" && paddle2Row > 0)        paddle2Row--;
        if (dir == "dn" && paddle2Row < ROWS - 2) paddle2Row++;
      }
      buildBoard();
    }
    req->send(200, "text/plain", "ok");
  });

  server.begin();

  delay(1000);
  lcd.clear();
  lcd.setCursor(0,0);
}

void loop() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("P1 ");
  lcd.print(score1);
  lcd.setCursor(0, 1);
  lcd.print("P2 ");
  lcd.print(score2);
  static unsigned long lastStep = 0;
  if (millis() - lastStep > 800) {
    lastStep = millis();
    stepGame();
  }
}