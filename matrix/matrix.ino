#include "LedControl.h"
#include <EEPROM.h>
#include <LiquidCrystal.h>

#define LEFT  0
#define RIGHT 1
#define UP    2
#define DOWN  3

#define NO_MOVEMENT 255

#define PLAY_GAME   0
#define SETTINGS    1
#define HIGHSCORES  2
#define ABOUT       3
#define HOW_TO_PLAY 4

#define FIRST_MENU      -10
#define NO_SELECTION    -1
#define EXIT_MENU_STATE -2

const int dinPin   = 12;
const int clockPin = 11;
const int loadPin  = 10;

const byte rs = 9;
const byte en = 8;
const byte d4 = 7;
const byte d5 = 3;
const byte d6 = 5;
const byte d7 = 4;

const byte backLight = 6;

const byte buzzerPin = 13;

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

const int matrixSize = 8;

LedControl lc = LedControl(dinPin, clockPin, loadPin, 1); // DIN, CLK, LOAD, No. DRIVER
const int  n  = 8;

const int pinX  = A0;
const int pinY  = A1;
const int pinSW = 2;

const int minThreshold = 100;
const int maxThreshold = 900;

const unsigned long blinkingDelayPlayer = 700;
const unsigned long blinkingDelayBullet = 75;

const unsigned long debounceDelayMs = 200;
const unsigned long travelTimeMs    = 200;

const unsigned long timeForInitialImageDisplayMs = 2000;
const unsigned long debounceDelayFunctionMs      = 800;

const unsigned long waitUntilScrollingTextMs = 2500;

const unsigned long delayBlinkCursor = 500;

unsigned long lastBlinkPlayer;
unsigned long lastBlinkBullet;
unsigned long lastBulletTravel;
unsigned long lastRecordStart;
unsigned long lastCallToFunction;
unsigned long lastBlinkCursor;
unsigned long timeTheGameStartedMs;

volatile bool bulletRequested;
volatile bool buttonPressed;
volatile bool playNote;

bool showStartImage = true;
bool skipImage;
bool chooseName;
bool stateSelect;

bool cursorState;
bool playGame;

int numberOfWalls;
int initialNumberOfWalls;

int navMenu      = FIRST_MENU;
int settingsMenu = NO_SELECTION;

char username[16] = {'A' - 1, 'A' - 1, 'A' - 1, 'A' - 1, 'A' - 1, 'A' - 1, 'A' - 1, 'A' - 1,
                     'A' - 1, 'A' - 1, 'A' - 1, 'A' - 1, 'A' - 1, 'A' - 1, 'A' - 1, 'A' - 1};

const char *optionStrings[]  = {"1. Start Game", "2. Settings", "3. Highscores", "4. About",
                                "5. How to play"};
const char *settingStrings[] = {"1.LCD Brightness",  "2.Matrix Brightness",
                                "3.Change username", "4.Reset highscores",
                                "5.Sound",           "6.Back"};
const char *about[]          = {
             "Game Name:", "Battle Tank", "Author:", "Radu Buzas", "GitHub user:", "radubuzas"};

const int sizeOfLCD    = 16;
const int maxLenght1_3 = 19; // "2.Matrix Brightness"
const int maxLenght2_3 = 18; // "4.Reset highscores"

const int diff1_3 = maxLenght1_3 - sizeOfLCD;
const int diff2_3 = maxLenght2_3 - sizeOfLCD;

const byte idxBestUser  = 0;  //  0-16
const byte idxHighScore = 16; // 16-18

const byte idxSecondBestUser  = 18; // 18-34
const byte idxSecondHighScore = 34; // 34-36

const byte idxThirdBestUser  = 36; // 36-52
const byte idxThirdHighScore = 52; // 52-54

const byte idxLCDBrightness    = 54;
const byte idxMatrixBrightness = 55;
const byte idxSoundControl     = 56;

int score;

bool resetState;
bool playSong;

struct position
{
    byte row;
    byte column;
};

struct movingObject
{
    struct position position;

    byte headingTo;
};

struct movingObject player;

struct movingObject *bullet;

// int TAMPON;

bool matrix[matrixSize * 2][matrixSize * 2];

const byte IMAGE[matrixSize] = {0b00000000, 0b10010111, 0b10010010, 0b11110010,
                                0b11110010, 0b10010010, 0b10010111, 0b00000000};

const byte HEART[matrixSize] = {0b01100110, 0b11111111, 0b11111111, 0b11111111,
                                0b11111111, 0b01111110, 0b00111100, 0b00011000};

const byte HAMMER[matrixSize] = {0b00010000, 0b01111000, 0b01111100, 0b11111000,
                                 0b01111000, 0b00100100, 0b00000010, 0b00000001};

const byte CHALLICE[matrixSize] = {0b10000001, 0b11000011, 0b01000010, 0b01100110,
                                   0b00111100, 0b00011000, 0b00011000, 0b00111100};

byte trophyChar[8] = {B10001, B10001, B11011, B01010, B00100, B01110, B00100, B11111};

void setup()
{
    // the zero refers to the MAX7219 number, it is zero for 1 chip
    lc.shutdown(0, false); // turn off power saving, enables display
    lc.setIntensity(0, 2); // sets brightness (0~15 possible values)
    lc.clearDisplay(0);    // clear screen

    pinMode(pinSW, INPUT_PULLUP);
    pinMode(pinX, INPUT);
    pinMode(pinY, INPUT);

    randomSeed(analogRead(0));

    attachInterrupt(digitalPinToInterrupt(pinSW), handleInterrupt, FALLING);

    lastRecordStart = millis();

    lcd.begin(16, 2);
    lcd.print(F("Hello, Welcome"));
    lcd.setCursor(0, 1);
    lcd.print((F("To TANK BATTLE!!")));

    lcd.createChar(0, trophyChar);

    pinMode(backLight, OUTPUT);

    byte brightness = EEPROM.read(idxLCDBrightness);
    analogWrite(backLight, brightness);

    brightness = EEPROM.read(idxMatrixBrightness);
    lc.setIntensity(0, brightness);

    EEPROM.get(idxSoundControl, playSong);

    Serial.begin(9600);
}

void loop()
{
    if (playSong)
        playTune();

    if (playGame == false)
    {
        showImage();
        setupGame();
        return;
    }

    showCurrentStatus();

    if (numberOfWalls == 0)
    {
        reset();
        return;
    }

    blinkPlayer();
    showMatrix();

    byte ret = checkMovement();
    if (ret != NO_MOVEMENT)
        movePlayer(ret);

    if (bullet != nullptr)
    {
        bulletTravel();
        blinkBullet();
    }
    if (bulletRequested)
        generateBullet();
}

//--------------------------------------------------------------------//--------------------------------------------------------------------//
// SETUP GAME LCD

void setupGame()
{
    if (showStartImage)
    {
        if (buttonPressed || millis() > timeForInitialImageDisplayMs)
        {
            showStartImage     = false;
            lastCallToFunction = millis();
            chooseName         = true;
        }
        return;
    }

    if (chooseName)
    {
        chooseName_function();
        return;
    }

    if (stateSelect)
    {
        displayOptions();
        return;
    }
}

//--------------------------------------------------------------------//--------------------------------------------------------------------//
// SHOW LIVE GAME STATS

void showCurrentStatus()
{
    static bool      initialized     = false;
    static const int messageLenLine0 = 7;
    static const int messageLenLine1 = 6;
    static int       numberOfWallsLocal;
    if (resetState)
        return;
    if (initialized == false)
    {
        initialized        = true;
        numberOfWallsLocal = numberOfWalls;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("Score: "));
        lcd.setCursor(0, 1);
        lcd.print(F("Time: "));
    }

    if (numberOfWalls == 0)
    {
        initialized = false;
        score += getScore();
        resetState = true;
    }

    if (numberOfWalls + 1 == numberOfWallsLocal)
        score += getScore(), --numberOfWallsLocal;

    lcd.setCursor(messageLenLine0, 0);
    lcd.print(score);
    lcd.setCursor(messageLenLine1, 1);

    unsigned long timeInGameMs = millis() - timeTheGameStartedMs;

    lcd.print(timeInGameMs / 1000);
    lcd.write('.');
    lcd.print(timeInGameMs % 1000 / 10);
}

//--------------------------------------------------------------------//--------------------------------------------------------------------//
// PRINT LCD

void printEndOfLevel()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(username);
    lcd.setCursor(0, 1);
    lcd.print(F("Score: "));
    lcd.print(score);
}

void printCongrats(byte place)
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(username);
    lcd.setCursor(0, 1);
    if (place == 1)
    {
        lcd.print(F("1st"));
    }
    else if (place == 2)
    {
        lcd.print(F("2nd"));
    }
    else
    {
        lcd.print(F("3rd"));
    }
    lcd.print(F(" place"));

    lcd.write(' ');
    lcd.write(byte(0));
}

void printOptions1_3()
{
    lcd.clear();
    for (byte i = 0; i < 2; ++i)
    {
        lcd.setCursor(0, i);
        lcd.print(optionStrings[i]);
        lcd.setCursor(0, 0);
    }
}

void printOptions2_3()
{
    lcd.clear();
    for (byte i = 2; i <= 3; ++i)
    {
        lcd.setCursor(0, i - 2);
        lcd.print(optionStrings[i]);
        lcd.setCursor(0, 0);
    }
}

void printOptions3_3()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(optionStrings[4]);
    lcd.setCursor(0, 0);
}

void printHighscore1_3()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("1st place: "));
    int scoreEEPROM;
    EEPROM.get(idxHighScore, scoreEEPROM);
    lcd.print(scoreEEPROM);
    lcd.setCursor(0, 1);
    for (byte i = 0; i < 16; ++i)
    {
        char c = EEPROM.read(idxBestUser + i);
        lcd.write(c);
    }
}

void printHighscore2_3()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("2nd place: "));
    int score;
    EEPROM.get(idxSecondHighScore, score);
    lcd.print(score);
    lcd.setCursor(0, 1);
    for (byte i = 0; i < 16; ++i)
    {
        char c = EEPROM.read(idxSecondBestUser + i);
        lcd.write(c);
    }
}

void printHighscore3_3()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("3rd place: "));
    int score;
    EEPROM.get(idxThirdHighScore, score);
    lcd.print(score);
    lcd.setCursor(0, 1);
    for (byte i = 0; i < 16; ++i)
    {
        char c = EEPROM.read(idxThirdBestUser + i);
        lcd.write(c);
    }
}

void printHowToPlay1_3()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Objective: "));
    lcd.setCursor(0, 1);
    lcd.print(F("You need to destroy all walls")); // 28
}

void printHowToPlay2_3()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Press joystick to fire a bullet")); // 30
    lcd.setCursor(0, 1);
    lcd.print(F("Walls are destroyed by bullets")); // 31
}

void printHowToPlay3_3()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("You win when the map is empty"));
    lcd.setCursor(0, 1);
    lcd.print(F("Move fast to earn more points")); // 29
}

void printAbout1_4()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(about[0]);
    lcd.setCursor(0, 1);
    lcd.print(about[1]);
}

void printAbout2_4()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("HighScore: "));
    int scoreEEPROM;
    EEPROM.get(idxHighScore, scoreEEPROM);
    lcd.print(scoreEEPROM);
    lcd.setCursor(0, 1);
    for (int i = 0; i < 16; ++i)
    {
        char c = EEPROM.read(idxBestUser + i);
        if (c == '\0')
            break;
        lcd.write(c);
        Serial.print(c);
    }
}

void printAbout3_4()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(about[2]);
    lcd.setCursor(0, 1);
    lcd.print(about[3]);
}

void printAbout4_4()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(about[4]);
    lcd.setCursor(0, 1);
    lcd.print(about[5]);
}

void printChooseName()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Choose username:"));
    lcd.setCursor(0, 1);
}

void printSettings1_3()
{
    lcd.clear();
    for (byte i = 0; i < 2; ++i)
    {
        lcd.setCursor(0, i);
        lcd.print(settingStrings[i]);
    }
    lcd.setCursor(0, 0);
}

void printSettings2_3()
{
    lcd.clear();
    for (byte i = 2; i <= 3; ++i)
    {
        lcd.setCursor(0, i - 2);
        lcd.print(settingStrings[i]);
    }
    lcd.setCursor(0, 0);
}

void printSettings3_3()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    for (byte i = 4; i <= 5; ++i)
    {
        lcd.setCursor(0, i - 4);
        lcd.print(settingStrings[i]);
    }
    lcd.setCursor(0, 0);
}

//--------------------------------------------------------------------//--------------------------------------------------------------------//
// CHOOSE USERNAME

void chooseName_function()
{
    static byte thisColumn  = 0;
    static bool initialized = false;
    if (initialized == false)
    {
        initialized = true;
        for (byte i = 0; i < 16; ++i)
        {
            username[i] = 'A' - 1;
        }
        printChooseName();
    }

    if (buttonPressed)
    {
        if (millis() - lastCallToFunction > debounceDelayFunctionMs)
        {
            chooseName = false;
            for (byte i = 0; i < 16; ++i)
            {
                if (username[i] == 'A' - 1)
                    username[i] = ' ';
                // EEPROM.update(0, username[i]);
            }
            cursorState = false;
            lcd.noCursor();

            thisColumn  = 0;
            initialized = false;

            lastCallToFunction = millis();
            if (settingsMenu == 2)
            {
                settingsMenu = EXIT_MENU_STATE;
                return;
            }
            stateSelect = true;
            printOptions1_3();
            return;
        }
        buttonPressed = false;
    }

    if (millis() - lastBlinkCursor > delayBlinkCursor)
    {
        lastBlinkCursor = millis();

        if (cursorState)
        {
            lcd.noCursor();
            cursorState = false;
        }
        else
        {
            lcd.cursor();
            cursorState = true;
        }
    }
    byte direction = checkMovement();

    if (direction == NO_MOVEMENT)
        return;

    char *thisChar = &username[thisColumn];

    if (direction == RIGHT)
    {
        if (thisColumn + 1 > 15)
            return;
        ++thisColumn;
        if (cursorState)
            lcd.noCursor();
        lcd.setCursor(thisColumn, 1);
        lcd.cursor();
        return;
    }
    else if (direction == LEFT)
    {
        if (thisColumn - 1 < 0)
            return;
        --thisColumn;
        if (cursorState)
            lcd.noCursor();
        lcd.setCursor(thisColumn, 1);
        lcd.cursor();
        return;
    }
    else if (direction == UP)
    {
        *thisChar = *thisChar - 1;
        if (*thisChar < 'A')
            *thisChar = 'z';
        else if (*thisChar < 'a' && *thisChar > 'Z')
            *thisChar = 'Z';
    }
    else
    {
        *thisChar = *thisChar + 1;
        if (*thisChar > 'z')
            *thisChar = 'A';
        else if (*thisChar > 'Z' && *thisChar < 'a')
            *thisChar = 'a';
    }

    lcd.write(*thisChar);
    lcd.setCursor(thisColumn, 1);
}

//--------------------------------------------------------------------//--------------------------------------------------------------------//
// FIRST LEVEL MENU

// Displays the first level of options and routes the subsections
void displayOptions()
{
    static byte option = 0;

    if (navMenu != FIRST_MENU)
    {
        switch (navMenu)
        {
        case PLAY_GAME:
            return;
        case SETTINGS:
            handleSettings();
            return;
        case HIGHSCORES:
            handleHighscores();
            return;
        case ABOUT:
            handleAbout();
            return;
        case HOW_TO_PLAY:
            handleHowToPlay();
            return;
        case EXIT_MENU_STATE:
            option  = 0;
            navMenu = FIRST_MENU;
            Serial.println(F("Returned to settings menu"));
            break;
        default:
            Serial.println(F("Bad memory leak!"));
            return;
        }
    }

    lcd.blink();

    if (buttonPressed)
    {
        buttonPressed = false;
        if (millis() - lastCallToFunction > debounceDelayFunctionMs)
        {
            unsigned long temp = lastCallToFunction;
            lastCallToFunction = millis();
            switch (option)
            {
            case PLAY_GAME:
                playGame = true;
                initMap();
                timeTheGameStartedMs = millis();
                return;
            case SETTINGS:
                navMenu = SETTINGS;
                return;
            case HIGHSCORES:
                navMenu = HIGHSCORES;
                return;
            case ABOUT:
                navMenu = ABOUT;
                return;
            case HOW_TO_PLAY:
                navMenu = HOW_TO_PLAY;
                return;
            default:
                Serial.println(F("Option has wrong value!"));
                lastCallToFunction = temp;
                return;
            }
        }
    }

    byte direction = checkMovement();

    if (direction == NO_MOVEMENT || direction == LEFT || direction == RIGHT)
        return;

    if (direction == UP)
    {
        if (++option > 4)
            option = 4;
    }
    else
    {
        if (option-- == 0)
            option = 0;
    }

    if (option == 0 || option == 1)
    {
        printOptions1_3();
        lcd.setCursor(0, option);
        return;
    }
    if (option == 2 || option == 3)
    {
        printOptions2_3();
        lcd.setCursor(0, option - 2);
        return;
    }
    printOptions3_3();
}

//--------------------------------------------------------------------//--------------------------------------------------------------------//
// handle MENU

void handleHowToPlay()
{
    static byte thisColumn;
    static byte option      = 0;
    static bool initialized = false;
    if (initialized == false)
    {
        initialized = true;
        thisColumn  = 16;
        printHowToPlay1_3();
    }

    if (buttonPressed)
    {
        buttonPressed = false;
        if (millis() - lastCallToFunction > debounceDelayFunctionMs)
        {
            lastCallToFunction = millis();

            initialized = false;
            option      = 0;
            thisColumn  = 16;
            navMenu     = EXIT_MENU_STATE;
            printOptions1_3();
            return;
        }
    }

    byte direction = checkMovement();

    if (direction == NO_MOVEMENT)
        return;

    if (direction == LEFT || direction == RIGHT)
    {
        if (direction == LEFT)
        {
            if (--thisColumn < 16)
                thisColumn = 16;
            else
                lcd.scrollDisplayRight();
            return;
        }

        if (option == 0)
        { //  28
            if (++thisColumn > 29)
                thisColumn = 29;
            else
                lcd.scrollDisplayLeft();
        }
        else if (option == 1) // 31
        {
            if (++thisColumn > 31)
                thisColumn = 31;
            else
                lcd.scrollDisplayLeft();
        }
        else
        { // 29
            if (++thisColumn > 29)
                thisColumn = 29;
            else
                lcd.scrollDisplayLeft();
        }
        return;
    }

    if (direction == UP)
    {
        if (++option > 2)
            option = 2;
    }
    else
    {
        if (option-- == 0)
            option = 0;
    }

    switch (option)
    {
    case 0:
        printHowToPlay1_3();
        return;
    case 1:
        printHowToPlay2_3();
        return;
    case 2:
        printHowToPlay3_3();
        return;
    default:
        Serial.println(F("ERROR in handleHowToPlay()"));
        return;
    }
}

void handleAbout()
{
    static byte option      = 0;
    static bool initialized = false;
    if (initialized == false)
    {
        initialized = true;
        printAbout1_4();
    }

    if (buttonPressed)
    {
        buttonPressed = false;
        if (millis() - lastCallToFunction > debounceDelayFunctionMs)
        {
            lastCallToFunction = millis();

            initialized = false;
            option      = 0;
            navMenu     = EXIT_MENU_STATE;
            printOptions1_3();
            return;
        }
    }

    byte direction = checkMovement();

    if (direction == NO_MOVEMENT || direction == LEFT || direction == RIGHT)
        return;

    if (direction == UP)
    {
        if (++option > 3)
            option = 3;
    }
    else
    {
        if (option-- == 0)
            option = 0;
    }

    switch (option)
    {
    case 0:
        printAbout1_4();
        return;
    case 1:
        printAbout2_4();
        return;
    case 2:
        printAbout3_4();
        return;
    case 3:
        printAbout4_4();
        return;
    default:
        Serial.println(F("ERROR in handleAbout()"));
        return;
    }
}

void handleHighscores()
{
    static byte option      = 0;
    static bool initialized = false;
    if (initialized == false)
    {
        initialized = true;
        printHighscore1_3();
    }

    if (buttonPressed)
    {
        buttonPressed = false;
        if (millis() - lastCallToFunction > debounceDelayFunctionMs)
        {
            lastCallToFunction = millis();

            initialized = false;
            option      = 0;
            navMenu     = EXIT_MENU_STATE;
            printOptions1_3();
            return;
        }
    }

    byte direction = checkMovement();

    if (direction == NO_MOVEMENT || direction == LEFT || direction == RIGHT)
        return;

    if (direction == UP)
    {
        if (++option > 2)
            option = 2;
    }
    else
    {
        if (option-- == 0)
            option = 0;
    }

    switch (option)
    {
    case 0:
        printHighscore1_3();
        return;
    case 1:
        printHighscore2_3();
        return;
    case 2:
        printHighscore3_3();
        return;
    default:
        Serial.println(F("ERROR in handleHighscores()"));
        return;
    }
}

void handleSettings()
{
    static unsigned long lastMove = millis();

    static byte option      = 0;
    static bool check       = false;
    static byte counter     = 0;
    static bool initialized = false;
    if (initialized == false)
    {
        initialized = true;
        lastMove    = millis();
        printSettings1_3();
    }

    if (check)
    {
        if (settingsMenu == NO_SELECTION)
        {
            settingsMenu = option;
        }

        switch (settingsMenu)
        {
        case 0:
            settingsMenu = 0;
            LCDBrightness();
            return;
        case 1:
            settingsMenu = 1;
            matrixBrightness();
            return;
        case 2:
            settingsMenu = 2;
            chooseName_function();
            return;
        case 3: //
            settingsMenu = 3;
            resetHighscore();
            return;
        case 4:
            settingsMenu = 4;
            changeSound();
            return;
        case 5: //  back
            check = false;
            printOptions1_3();
            navMenu      = EXIT_MENU_STATE;
            settingsMenu = NO_SELECTION;
            initialized  = false;
            option       = 0;
            return;
        case EXIT_MENU_STATE:
            Serial.println(F("Back to settings!"));
            check = false;
            printSettings1_3();
            settingsMenu = NO_SELECTION;
            initialized  = false;
            option       = 0;
            return;
        default:
            Serial.println(F("WHAT?!"));
            return;
        }
    }

    if (buttonPressed)
    {
        buttonPressed = false;
        if (millis() - lastCallToFunction > debounceDelayFunctionMs)
        {
            lastCallToFunction = millis();
            check              = true;
            return;
        }
    }

    moveText(lastMove, counter, option / 2);

    byte direction = checkMovement();

    if (direction == NO_MOVEMENT || direction == LEFT || direction == RIGHT)
        return;

    if (direction == UP)
    {
        if (++option > 5)
            option = 5;
    }
    else
    {
        if (option-- == 0)
            option = 0;
    }

    if (counter <= 3)
    {
        for (; counter > 0; --counter)
            lcd.scrollDisplayRight();
    }
    else
    {
        for (; counter < 6; ++counter)
            lcd.scrollDisplayRight();
        counter = 0;
    }

    if (option == 0 || option == 1)
    {
        printSettings1_3();
        lcd.setCursor(0, option);
    }
    else if (option == 2 || option == 3)
    {
        printSettings2_3();
        lcd.setCursor(0, option - 2);
    }
    else if (option == 4 || option == 5)
    {
        printSettings3_3();
        lcd.setCursor(0, option - 4);
    }

    lastMove = millis();
}

//--------------------------------------------------------------------//--------------------------------------------------------------------//
// SETTINGS SUBMENU

void LCDBrightness()
{
    static byte brightness  = EEPROM.read(idxLCDBrightness);
    static byte no_chars    = (brightness >= 100) ? 3 : (brightness >= 10 ? 2 : 1);
    static bool initialized = false;
    if (initialized == false)
    {
        initialized = true;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("LCD"));
        lcd.setCursor(0, 1);
        lcd.print(F("Brightness: "));
        lcd.print(brightness);
        lcd.setCursor(0 + 12 + no_chars - 1, 1);
    }

    if (buttonPressed)
    {
        buttonPressed = false;
        if (millis() - lastCallToFunction > debounceDelayFunctionMs)
        {
            unsigned long temp = lastCallToFunction;
            lastCallToFunction = millis();
            EEPROM.update(idxLCDBrightness, brightness);
            analogWrite(backLight, brightness);
            settingsMenu = EXIT_MENU_STATE;
            initialized  = false;
            return;
        }
    }

    byte direction = checkMovement();

    if (direction == NO_MOVEMENT)
        return;

    if (direction == DOWN)
    {
        if (brightness >= 255 - 15)
            brightness = 255;
        else
            brightness += 15;
    }
    else if (direction == RIGHT)
    {
        if (brightness++ == 255)
            brightness = 255;
    }
    else if (direction == UP)
    {
        if (brightness <= 15)
            brightness = 0;
        else
            brightness -= 15;
    }
    else
    {
        if (brightness-- == 0)
            brightness = 0;
    }

    byte lastNo_chars = no_chars;
    no_chars          = (brightness >= 100) ? 3 : (brightness >= 10 ? 2 : 1);

    analogWrite(backLight, brightness);

    lcd.setCursor(0 + 12, 1);
    lcd.print(brightness);
    lcd.setCursor(0 + 12 + no_chars, 1);
    if (lastNo_chars - 1 == no_chars)
        lcd.write(' ');
    lcd.setCursor(0 + 12 + no_chars - 1, 1);
}

void matrixBrightness()
{
    static byte brightness = EEPROM.read(idxMatrixBrightness);
    static byte no_chars   = (brightness >= 10) ? 2 : 1;
    static bool initialized;
    if (initialized == false)
    {
        initialized = true;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("Matrix"));
        lcd.setCursor(0, 1);
        lcd.print(F("Brightness: "));
        lcd.print(brightness);
        lcd.setCursor(0 + 12 + no_chars - 1, 1);
    }

    if (buttonPressed)
    {
        buttonPressed = false;
        if (millis() - lastCallToFunction > debounceDelayFunctionMs)
        {
            unsigned long temp = lastCallToFunction;
            lastCallToFunction = millis();
            EEPROM.update(idxMatrixBrightness, brightness);
            lc.setIntensity(0, brightness);
            settingsMenu = EXIT_MENU_STATE;
            initialized  = false;
            return;
        }
    }

    byte direction = checkMovement();

    if (direction == NO_MOVEMENT)
        return;

    if (direction == DOWN || direction == RIGHT)
    {
        if (brightness++ == 15)
            brightness = 15;
    }
    else
    {
        if (brightness-- == 0)
            brightness = 0;
    }

    no_chars = (brightness >= 10) ? 2 : 1;

    lc.setIntensity(0, brightness);

    lcd.setCursor(0 + 12, 1);
    lcd.print(brightness);
    lcd.setCursor(0 + 12 + no_chars, 1);
    if (brightness == 9)
        lcd.write(' ');
    lcd.setCursor(0 + 12 + no_chars - 1, 1);
}

//--------------------------------------------------------------------//--------------------------------------------------------------------//
// CALCULATE SCORE

int getScore()
{
    static const int   initialScore = 150;
    static const float maxTimeSec   = 150;

    // Linear interpolation using integer arithmetic
    int timeSec = (millis() - timeTheGameStartedMs) / 1000;

    if (maxTimeSec - timeSec < 0)
        return 0;

    Serial.println((maxTimeSec - timeSec) / maxTimeSec);

    return initialScore * ((maxTimeSec - timeSec) / maxTimeSec);
}

void resetHighscore()
{
    static bool state       = true;
    static bool initialized = false;
    if (initialized == false)
    {
        initialized = true;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Are you sure?");
        lcd.setCursor(0, 1);
        lcd.print("YES / NO");
        lcd.setCursor(0, 1);
    }

    if (buttonPressed)
    {
        buttonPressed = false;
        if (millis() - lastCallToFunction > debounceDelayFunctionMs)
        {
            initialized  = false;
            settingsMenu = EXIT_MENU_STATE;
            if (state)
            {
                const int scoreNull = 0;
                EEPROM.put(idxHighScore, scoreNull);
                EEPROM.put(idxSecondHighScore, scoreNull);
                EEPROM.put(idxThirdHighScore, scoreNull);

                for (byte i = 0; i < 16; ++i)
                {
                    EEPROM.update(idxBestUser + i, ' ');
                    EEPROM.update(idxSecondBestUser + i, ' ');
                    EEPROM.update(idxThirdBestUser + i, ' ');
                }
            }
            state = true;
            return;
        }
    }

    byte direction = checkMovement();

    if (direction == NO_MOVEMENT || direction == UP || direction == DOWN)
        return;

    if (direction == LEFT)
    {
        lcd.setCursor(0, 1);
        state = true;
    }
    else
    {
        lcd.setCursor(6, 1);
        state = false;
    }
}

//--------------------------------------------------------------------//--------------------------------------------------------------------//
// ENVIRONMENT

//  Displays initial image
void showImage()
{

    if (settingsMenu == 1)
    {
        for (byte row = 0; row < matrixSize; ++row)
        {
            lc.setRow(0, row, 0b11111111);
        }
        return;
    }

    if (navMenu == SETTINGS)
    {
        for (byte row = 0; row < matrixSize; ++row)
        {
            lc.setRow(0, row, HAMMER[row]);
        }
        return;
    }

    if (navMenu == HIGHSCORES)
    {
        for (byte row = 0; row < matrixSize; ++row)
        {
            lc.setRow(0, row, CHALLICE[row]);
        }
        return;
    }

    for (int row = 0; row < matrixSize; ++row)
    {
        lc.setRow(0, row, IMAGE[row]);
    }
}

//  Upon pressing the button, the map will reset
void reset()
{

    static unsigned long timeEnteredMs;
    static byte          place;
    static bool          initialized = false;
    if (initialized == false)
    {
        initialized   = true;
        timeEnteredMs = millis();
        place         = 4;
        int thirdPlace;
        EEPROM.get(idxThirdHighScore, thirdPlace);
        if (score > thirdPlace)
        {
            place = 3;
            int secondPlace;
            EEPROM.get(idxSecondHighScore, secondPlace);
            if (score > secondPlace)
            {
                place = 2;
                { //  Move 2nd to 3rd place
                    for (byte i = 0; i < 16; ++i)
                    {
                        char c = EEPROM.read(idxSecondBestUser + i);
                        EEPROM.update(idxThirdBestUser + i, c);
                    }
                    EEPROM.put(idxThirdHighScore, secondPlace);
                }
                int firstPlace;
                EEPROM.get(idxHighScore, firstPlace);
                if (score > firstPlace)
                {
                    place = 1;
                    { //  Move 1st to 2nd place
                        for (byte i = 0; i < 16; ++i)
                        {
                            char c = EEPROM.read(idxBestUser + i);
                            EEPROM.update(idxSecondBestUser + i, c);
                        }
                        EEPROM.put(idxSecondHighScore, firstPlace);
                    }

                    {
                        //  Move current user to 1st place
                        for (byte i = 0; i < 16; ++i)
                        {
                            EEPROM.update(idxBestUser + i, username[i]);
                        }
                        EEPROM.put(idxHighScore, score);
                    }
                }
                else
                {
                    //  Move current user to 2nd place
                    for (byte i = 0; i < 16; ++i)
                    {
                        EEPROM.update(idxSecondBestUser + i, username[i]);
                    }
                    EEPROM.put(idxSecondHighScore, score);
                }
            }
            else
            {
                //  Move current user to 3rd place
                for (byte i = 0; i < 16; ++i)
                {
                    EEPROM.update(idxThirdBestUser + i, username[i]);
                }
                EEPROM.put(idxThirdHighScore, score);
            }
        }
        printEndOfLevel();
    }

    if (buttonPressed)
    {
        buttonPressed = false;
        if (millis() - timeEnteredMs > debounceDelayFunctionMs)
        {
            if (place != 4)
            {
                printCongrats(place);
                place = 4;
                return;
            }
            playGame    = false;
            initialized = false;
            resetState  = false;
            score       = 0;
            printOptions1_3();
            return;
        }
    }

    for (int row = 0; row < matrixSize; ++row)
    {
        lc.setRow(0, row, HEART[row]);
    }
}

// Generates random structures
void initMap()
{
    for (int i = 0; i < matrixSize * 2; ++i)
    {
        for (int j = 0; j < matrixSize * 2; ++j)
        {
            matrix[i][j] = random(10) % 2;
            numberOfWalls += matrix[i][j];
        }
    }

    player.position.row    = random(40) % 8;
    player.position.column = random(40) % 8;

    player.headingTo = RIGHT;

    numberOfWalls -= matrix[player.position.row][player.position.column];

    initialNumberOfWalls = numberOfWalls;

    matrix[player.position.row][player.position.column] = 1;
}

//  Will display to current configuration of the matrix
void showMatrix()
{
    byte column = player.position.column;
    byte row    = player.position.row;

    if (column >= matrixSize)
        column = matrixSize;
    else
        column = 0;

    if (row >= matrixSize)
        row = matrixSize;
    else
        row = 0;

    for (int i = 0; i < matrixSize; i++)
    {
        for (int j = 0; j < matrixSize; j++)
        {
            lc.setLed(0, i, j, matrix[row + i][column + j]);
        }
    }
}

//--------------------------------------------------------------------//--------------------------------------------------------------------//
// BLINKING

//  Makes the player blink ON and OFF
void blinkPlayer()
{
    if (millis() - lastBlinkPlayer > blinkingDelayPlayer)
    {
        int playerX              = player.position.row;
        int playerY              = player.position.column;
        matrix[playerX][playerY] = !matrix[playerX][playerY];

        lastBlinkPlayer = millis();
    }
}

//  Makes the bullet blink ON and OFF. Remember that there is only one bullet
void blinkBullet()
{
    if (bullet == nullptr)
        return;

    if (millis() - lastBlinkBullet > blinkingDelayBullet)
    {
        int bulletX              = bullet->position.row;
        int bulletY              = bullet->position.column;
        matrix[bulletX][bulletY] = !matrix[bulletX][bulletY];

        lastBlinkBullet = millis();
    }
}

//--------------------------------------------------------------------//--------------------------------------------------------------------//

// Will go according to the controller
void movePlayer(byte move)
{
    player.headingTo = move;

    byte newPosRow    = player.position.row + (move == UP) - (move == DOWN);
    byte newPosColumn = player.position.column + (move == RIGHT) - (move == LEFT);

    if (newPosRow < 0 || newPosRow > 2 * matrixSize - 1 || newPosColumn < 0 ||
        newPosColumn > 2 * matrixSize - 1) //  invalid action
        return;

    if (playerCollision(newPosRow, newPosColumn))
        return;

    matrix[player.position.row][player.position.column] = 0;

    player.position.row    = newPosRow;
    player.position.column = newPosColumn;

    matrix[player.position.row][player.position.column] = 1; //  smooth transition

    lastBlinkPlayer = millis(); //  smooth transition
}

//  check is the player will hit a wall
bool playerCollision(byte row, byte column)
{
    if (matrix[row][column] == 1) //  invalid action
    {
        return true;
    }
    return false;
}

// creating a bullet object based on the player's position and orientation
void generateBullet()
{
    bulletRequested = false;
    if (bullet != nullptr)
        return;

    bullet = (movingObject *)malloc(sizeof(movingObject));

    byte headingTo    = player.headingTo;
    byte tempRow      = player.position.row + (headingTo == UP) - (headingTo == DOWN);
    byte tempColumn   = player.position.column + (headingTo == RIGHT) - (headingTo == LEFT);
    bullet->headingTo = headingTo;

    if (tempRow < 0 || tempRow > 2 * matrixSize - 1 || tempColumn < 0 ||
        tempColumn > 2 * matrixSize - 1) //  invalid action
    {
        free(bullet);
        bullet = nullptr;
        return;
    }

    if (bulletCollision(tempRow, tempColumn))
        return;

    bullet->position.row    = tempRow;
    bullet->position.column = tempColumn;

    matrix[bullet->position.row][bullet->position.column] = 1;
}

void bulletTravel()
{
    if (millis() - lastBulletTravel > travelTimeMs)
    {

        byte headingTo = bullet->headingTo;

        matrix[bullet->position.row][bullet->position.column] = 0;

        byte tempRow    = bullet->position.row + (headingTo == UP) - (headingTo == DOWN);
        byte tempColumn = bullet->position.column + (headingTo == RIGHT) - (headingTo == LEFT);

        lastBulletTravel = millis();

        if (tempRow < 0 || tempRow > 2 * matrixSize - 1 || tempColumn < 0 ||
            tempColumn > 2 * matrixSize - 1) //  invalid action
        {
            free(bullet);
            bullet = nullptr;
            return;
        }

        if (bulletCollision(tempRow, tempColumn))
            return;

        matrix[bullet->position.row][bullet->position.column] = 0;

        bullet->position.row    = tempRow;
        bullet->position.column = tempColumn;

        matrix[bullet->position.row][bullet->position.column] = 1;
    }
}

// the bullet will hit a wall
bool bulletCollision(byte row, byte column)
{
    if (matrix[row][column] == 1) //  invalid action
    {
        --numberOfWalls; //  trying to scount the broken walls
        free(bullet);
        bullet = nullptr;

        matrix[row][column] = 0;
        return true;
    }

    return false;
}

//--------------------------------------------------------------------//--------------------------------------------------------------------//

// Animation for text scrolling
void moveText(unsigned long &lastMove, byte &counter, byte menuPoint)
{
    if (menuPoint == 2)
        return;
    const int diff = (menuPoint ? diff2_3 : diff1_3);
    if (millis() - lastMove > waitUntilScrollingTextMs)
    {
        const unsigned long smoothness = 1000;
        if (millis() - lastMove > smoothness * counter + waitUntilScrollingTextMs)
        {
            if (++counter <= diff)
            {
                lcd.scrollDisplayLeft();
            }
            else
            {
                lcd.scrollDisplayRight();
                if (counter == 2 * diff)
                {
                    counter  = 0;
                    lastMove = millis();
                }
            }
        }
    }
}

//--------------------------------------------------------------------//--------------------------------------------------------------------//
// JOYSTICK

//  This function checks the movement of the joystick
byte checkMovement()
{
    static bool xMoved = false;
    static bool yMoved = false;

    int xRead = analogRead(pinY);
    int yRead = analogRead(pinX);

    if (xRead < minThreshold && xMoved == false)
    {
        xMoved = true;
        return LEFT;
    }

    if (xRead > maxThreshold && xMoved == false)
    {
        xMoved = true;
        return RIGHT;
    }

    if (yRead < minThreshold && yMoved == false)
    {
        yMoved = true;
        return UP;
    }

    if (yRead > maxThreshold && yMoved == false)
    {
        yMoved = true;
        return DOWN;
    }

    if (xRead >= minThreshold && xRead <= maxThreshold)
    {
        xMoved = false;
    }

    if (yRead >= minThreshold && yRead <= maxThreshold)
    {
        yMoved = false;
    }

    delay(1);
    return NO_MOVEMENT;
}

//--------------------------------------------------------------------//--------------------------------------------------------------------//

// Sound
void changeSound()
{
    static bool state       = true;
    static bool initialized = false;
    if (initialized == false)
    {
        initialized = true;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Turn the sound:");
        lcd.setCursor(0, 1);
        lcd.print("ON / OFF");
        lcd.setCursor(0, 1);
    }

    if (buttonPressed)
    {
        buttonPressed = false;
        if (millis() - lastCallToFunction > debounceDelayFunctionMs)
        {
            initialized  = false;
            settingsMenu = EXIT_MENU_STATE;

            playSong = state;
            EEPROM.put(idxSoundControl, playSong);

            state = true;
            return;
        }
    }

    byte direction = checkMovement();

    if (direction == NO_MOVEMENT || direction == UP || direction == DOWN)
        return;

    if (direction == LEFT)
    {
        lcd.setCursor(0, 1);
        state = true;
    }
    else
    {
        lcd.setCursor(5, 1);
        state = false;
    }
}

void playTune()
{
    /*
  Pacman Intro Theme
  Connect a piezo buzzer or speaker to pin 11 or select a new pin.
  More songs available at https://github.com/robsoncouto/arduino-songs

                                              Robson Couto, 2019
*/
#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978
#define REST     0

    // change this to make the song slower or faster
    const int tempo = 105;

    static int           thisNote   = 0;
    static unsigned long lastRecord = 0;

    static int  divider      = 0;
    static int  noteDuration = 0;
    static bool playedNote   = 1;

    if (playGame)
    {
        if (playNote)
        {
            tone(buzzerPin, NOTE_D3, 300);
            Serial.println("HELLLOOO");
            playNote = false;
        }
        return;
    }

    // notes of the moledy followed by the duration.
    // a 4 means a quarter note, 8 an eighteenth , 16 sixteenth, so on
    // !!negative numbers are used to represent dotted notes,
    // so -4 means a dotted quarter note, that is, a quarter plus an eighteenth!!
    const int melody[] = {
        // Pacman
        // Score available at https://musescore.com/user/85429/scores/107109
        NOTE_B4,  16,  NOTE_B5,  16,  NOTE_FS5, 16,  NOTE_DS5, 16, // 1
        NOTE_B5,  32,  NOTE_FS5, -16, NOTE_DS5, 8,   NOTE_C5,  16, NOTE_C6, 16, NOTE_G6, 16,
        NOTE_E6,  16,  NOTE_C6,  32,  NOTE_G6,  -16, NOTE_E6,  8,

        NOTE_B4,  16,  NOTE_B5,  16,  NOTE_FS5, 16,  NOTE_DS5, 16, NOTE_B5, 32, // 2
        NOTE_FS5, -16, NOTE_DS5, 8,   NOTE_DS5, 32,  NOTE_E5,  32, NOTE_F5, 32, NOTE_F5, 32,
        NOTE_FS5, 32,  NOTE_G5,  32,  NOTE_G5,  32,  NOTE_GS5, 32, NOTE_A5, 16, NOTE_B5, 8};

    // sizeof gives the number of bytes, each int value is composed of two bytes (16 bits)
    // there are two values per note (pitch and duration), so for each note there are four bytes
    int notes = sizeof(melody) / sizeof(melody[0]) / 2;

    // this calculates the duration of a whole note in ms
    int wholenote = (60000 * 4) / tempo;

    if (millis() - lastRecord > noteDuration)
    {
        lastRecord = millis();
        playedNote = !playedNote;
        if (playedNote)
        {
            noteDuration = noteDuration * 0.1;
            noTone(buzzerPin);
            return;
        }
        thisNote = (thisNote + 2) % (notes * 2);
        {

            // calculates the duration of each note
            divider = melody[thisNote + 1];
            if (divider > 0)
            {
                // regular note, just proceed
                noteDuration = (wholenote) / divider;
            }
            else if (divider < 0)
            {
                // dotted notes are represented with negative durations!!
                noteDuration = (wholenote) / abs(divider);
                noteDuration *= 1.5; // increases the duration in half for dotted notes
            }

            // we only play the note for 90% of the duration, leaving 10% as a pause
            tone(buzzerPin, melody[thisNote], noteDuration * 0.9);
        }
    }
}

//--------------------------------------------------------------------//--------------------------------------------------------------------//

//  When the button is pressed, this ISR will be triggered
void handleInterrupt()
{
    static unsigned long lastInterrupt = 0;

    unsigned long now = micros();

    if (now - lastInterrupt > debounceDelayMs * 1000)
    {
        bulletRequested = true;
        playNote        = true;
        if (numberOfWalls == 0)
            skipImage = true;
        buttonPressed = true;
        lastInterrupt = now;
    }
}