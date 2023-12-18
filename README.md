# 16x16 LED Matrix Game :video_game:

## Description

## Description

Welcome to Tank Battle, an exciting game where you take on the role of a tank commander on a mission to destroy all the hardened walls on a dynamic battlefield. The game starts with a cheerful greeting message displayed on the 8x8 LED matrix. If you're eager to jump into action, a quick press of the joystick button skips the greeting message, getting you straight into the game.

### Gameplay

- **Map Generation:**
  - A random map is generated with static LEDs representing hard walls.
  - A tank (slow blinking LED) is also randomly placed on the battlefield.

- **Objective:**
  - Your mission is to strategically maneuver the tank to destroy all the walls.

- **Tank Controls:**
  - Use the joystick for smooth and responsive control over the tank's movements.
  - The tank is distinguishable by its slow-blinking LED.

- **Bullet Destruction:**
  - Press the joystick button to fire a fast-blinking LED bullet.
  - If the bullet reaches a wall, the wall will be destroyed.

- **Winning the Game:**
  - Once all the walls are successfully destroyed, a heart symbol will be displayed on the LED matrix.
  - Congratulations, you've won the Tank Battle!
  - After winning the game, you can press the joystick to activate a new round

### Game Features

- **Greeting Message:**
  - At the start of the game, a greeting message appears on the LED matrix.
  - If you're eager to begin, a press of the joystick button skips the message.

- **Randomization:**
  - The map, wall placements, and tank spawn locations are all randomized for a unique experience each time you play.

- **Dynamic LED Display:**
  - LED indicators clearly differentiate the tank, bullets, and walls.
  - Tank: Slow-blinking LED.
  - Bullet: Fast-blinking LED.
  - Walls: Static LEDs.

## Menu

- [Select UserName](#select-username)
  - [Menu](#menu)
    - [1. Start Game](#1-start-game)
    - [2. Settings](#2-settings)
      - [1. LCD Brightness](#1-lcd-brightness)
      - [2. Matrix Brightness](#2-matrix-brightness)
      - [3. Change Username](#3-change-username)
      - [4. Reset Highscores](#4-reset-highscores)
      - [5. Sound (Turn it ON or OFF)](#5-sound-turn-it-on-or-off)
      - [6. Back](#6-back)
    - [3. Highscores](#3-highscores)
    - [4. About](#4-about)
    - [5. How to Play](#5-how-to-play)

## Select UserName

- UserName selection is the first step.

## Menu

- Main menu options.

### 1. Start Game

- Start the game.

### 2. Settings

- Options to customize game settings.

  #### 1. LCD Brightness

  - Adjust the brightness of the LCD.

  #### 2. Matrix Brightness

  - Adjust the brightness of the matrix.

  #### 3. Change Username

  - Change the current username.

  #### 4. Reset Highscores

  - Reset the highscores to default.

  #### 5. Sound (Turn it ON or OFF)

  - Toggle sound ON or OFF.

  #### 6. Back

  - Go back to the main menu.

### 3. Highscores

- View the top 3 highscores.

### 4. About

- Information about the game.

  - Name of the game.
  - HighScore with associated user.
  - Author.
  - GitHub user.

### 5. How to Play

- Instructions on how to play the game.



## Components

- Arduino Uno
- Joystick
- 8x8 LED Matrix
- MAX7219
- LCD
- Buzzer
- Capacitors
- Potentiometer
- Resistors, capacitors and wires

## Setup

![](https://github.com/radubuzas/MatrixProject/blob/master/Assets/top.jpg)

![](https://github.com/radubuzas/MatrixProject/blob/master/Assets/front.jpg)

![](https://github.com/radubuzas/MatrixProject/blob/master/Assets/back.jpg)


## [DEMO](https://youtu.be/WOwh-9-vRxs)
[![](https://img.youtube.com/vi/WOwh-9-vRxs/0.jpg)](https://youtu.be/WOwh-9-vRxs)

## [Code](https://github.com/radubuzas/MatrixProject/blob/master/matrix/matrix.ino)