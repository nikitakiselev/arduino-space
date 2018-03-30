#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>

// Display settings.
// Attach DIN to MOSI and CLK to SCK (cf http://arduino.cc/en/Reference/SPI )
#define MAX_DISPLAY_WIDTH 8
#define MAX_DISPLAY_HEIGHT 8
#define DISPLAY_INTENSITY 1 // Use a value between 0 and 15 for brightness
#define DISPLAY_ROTATION 2
#define PIN_CS 10 // Attach CS to this pin

// Encoder settings.
#define ENCODER_PIN_DT 2
#define ENCODER_PIN_CLK 7
#define ENCODER_PIN_SW 3
#define ENCODER_POLLING_FREQUENCY 5 // microseconds

// Text settings.
#define TEXT_SCROLL_SPEED 100
#define LETTER_SPACE 1
#define LETTER_WIDTH (5 + LETTER_SPACE)

#define DEFAULT_ENEMY_FREEZING 500

bool startGame;
bool isGameOver;
unsigned char score;

unsigned char playerX;
unsigned char playerY;
unsigned char playerWidth;

unsigned long enemySpawnTime;
unsigned long lastEnemySpawnTime;
unsigned long lastEnemyMoveDownTime;
char enemyX;
unsigned char enemyY;
unsigned int enemyFreezing;

unsigned long lastEncoderChangeTime;
unsigned long encoderChangeTime;
char lastDtState;
char dtState;
char clkState;

unsigned long now;

Max72xxPanel display = Max72xxPanel(PIN_CS, 1, 1);


void setup() {
  display.setIntensity(DISPLAY_INTENSITY);
  display.setRotation(DISPLAY_ROTATION);

  pinMode(ENCODER_PIN_DT, INPUT);
  pinMode(ENCODER_PIN_CLK, INPUT);
  pinMode(ENCODER_PIN_SW, INPUT_PULLUP);

  attachInterrupt(0, encoderChangeHandler, CHANGE);
  attachInterrupt(1, buttonClickHandler, LOW);

  lastEncoderChangeTime = 0;

  initGame();
}

void loop() {
  // Show intro.
  if (isFirstRun()) {
    displayText("Press button to play");
  }

  // Show "game over" screen.
  if (gameIsOver()) {
    char buf[30];
    sprintf(buf, "Game Over. Your Score: %d", score);
    displayText(buf);
  }

  if (gameIsRunning()) {
    cleanDisplay();
    now = millis();

    // Draw player
    drawPlayer();

    if (enemyX == -1 && now - lastEnemySpawnTime > enemySpawnTime) {
      lastEnemySpawnTime = now;
      spawnEnemy();
    }

    // Draw enemy
    if (enemyX > -1) {
      display.drawPixel(enemyX, enemyY, HIGH);

      moveEnemyDown();

      checkForCollisions();
    }

    display.write();
  }
}

void drawPlayer() {
  display.drawPixel(playerX, playerY, HIGH);
  display.drawPixel(playerX + (playerWidth - 1), playerY, HIGH);
}

/*
   Check for collisions with enemies.
*/
void checkForCollisions() {
  if (enemyY == playerY && (enemyX == playerX || enemyX == playerX + (playerWidth - 1))) {
    gameOver();
  }
}

/**
   Spawn random enemy
*/
void spawnEnemy() {
  enemyX = random(0, MAX_DISPLAY_WIDTH - 1);
  enemyY = -1;
}

/*
   Move enemy down and increase score.
*/
void moveEnemyDown() {
  if (now - lastEnemyMoveDownTime > enemyFreezing) {
    enemyY++;

    // Remove enemy
    if (enemyY > MAX_DISPLAY_HEIGHT) {
      enemyY = -1;
      enemyX = -1;

      score++;
      if (enemyFreezing > 100 && score % 2) {
        enemyFreezing -= 50;
      }
    }

    lastEnemyMoveDownTime = now;
  }
}
/*
   Click button handler.
*/
void buttonClickHandler() {
  if (digitalRead(ENCODER_PIN_SW) == LOW) {
    startTheGame();
  }
}

/*
   Change encoder handler.
*/
void encoderChangeHandler() {
  encoderChangeTime = micros();
  if (encoderChangeTime - lastEncoderChangeTime > ENCODER_POLLING_FREQUENCY) {
    lastEncoderChangeTime = encoderChangeTime;

    dtState = digitalRead(ENCODER_PIN_DT);
    clkState = digitalRead(ENCODER_PIN_CLK);

    if (dtState != lastDtState) {
      if (clkState == dtState) {
        if (playerX + 1 < (MAX_DISPLAY_WIDTH - playerWidth)) {
          playerX++;
        } else {
          playerX = (MAX_DISPLAY_WIDTH - playerWidth);
        }
      } else {
        if (playerX - 1 > 0) {
          playerX--;
        } else {
          playerX = 0;
        }
      }

      lastDtState = dtState;
    }
  }
}

/*
   Init game.
*/
void initGame() {
  score = 0;
  enemySpawnTime = 1000;
  enemyFreezing = DEFAULT_ENEMY_FREEZING;
  enemyX = -1; // hide enemy
  enemyY = -1;
  startGame = false;
  isGameOver = false;
  playerWidth = 2;
  cleanDisplay();
  randomSeed(analogRead(0));
}

/*
 * Start the game.
 */
void startTheGame() {
  initGame();
  startGame = true;

  // spawn player
  playerX = 0;
  playerY = MAX_DISPLAY_HEIGHT - 1;

  // detach button interrupts
  detachInterrupt(1);
}

/**
   Trigger game over screen.
*/
void gameOver() {
  startGame = false;
  isGameOver = true;

  // return back button handler
  attachInterrupt(1, buttonClickHandler, LOW);
}

/**
   Check the game is running firstly.
*/
bool isFirstRun() {
  return ! startGame && ! isGameOver;
}

/*
   Check for game is running.
*/
bool gameIsRunning() {
  return startGame && ! isGameOver;
}

/*
   Check the game is over.
*/
bool gameIsOver() {
  return ! startGame && isGameOver;
}

/**
   Clear the display.
*/
void cleanDisplay() {
  display.fillScreen(LOW);
}

/*
   Display text on screen.
*/
void displayText(String text) {
  for ( int i = 0 ; i < LETTER_WIDTH * text.length() + display.width() - 1 - LETTER_SPACE; i++ ) {
    // Stop when game is start.
    if (startGame) {
      break;
    }

    cleanDisplay();

    int letter = i / LETTER_WIDTH;
    int x = (display.width() - 1) - i % LETTER_WIDTH;
    int y = (display.height() - 8) / 2; // center the text vertically

    while ( x + LETTER_WIDTH - LETTER_SPACE >= 0 && letter >= 0 ) {
      if ( letter < text.length() ) {
        display.drawChar(x, y, text[letter], HIGH, LOW, 1);
      }
      letter--;
      x -= LETTER_WIDTH;
    }

    display.write();
    delay(TEXT_SCROLL_SPEED);
  }
}

