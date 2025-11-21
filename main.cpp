#include <SFML/Graphics.hpp>
#include <iostream>
#include <cstdlib>
#include <ctime>
using namespace std;
using namespace sf;

// Grid Setup: Defines the dimensions and layout of the game grid
const int ROWS = 23;
const int COLS = 15;
const int CELL_SIZE = 40;
const int MARGIN = 40; // Margin around the grid for UI spacing

// Game States: Constants representing different screens/states of the game
const int STATE_MENU = 0;
const int STATE_PLAYING = 1;
const int STATE_INSTRUCTIONS = 2;
const int STATE_GAME_OVER = 3;
const int STATE_LEVEL_UP = 4;
const int STATE_VICTORY = 5;
const int STATE_PAUSED = 6;

int main()
{

    // Random Number Generator Setup: Seeds the random number generator with the current time
    srand(static_cast<unsigned int>(time(0)));

    // Window Setup: Calculates window size based on grid dimensions and creates the SFML window
    const int windowWidth = COLS * CELL_SIZE + MARGIN * 2 + 500; // Extra width for side panel (score, lives, etc.)
    const int windowHeight = ROWS * CELL_SIZE + MARGIN * 2;
    RenderWindow window(VideoMode(windowWidth, windowHeight), "Space Shooter");
    window.setFramerateLimit(60); // Limit frame rate to 60 FPS for smooth gameplay

    // Game Variables Setup: Initialize core game state variables
    int currentState = STATE_MENU;
    int selectedMenuItem = 0; // Index of the currently selected menu item
    int lives = 3;            // Player's remaining lives
    int score = 0;            // Current score
    int level = 1;            // Current game level
    const int MAX_LEVEL = 5;  // Maximum level before victory
    bool isInvincible = false;                 // Flag for temporary invincibility after being hit
    Clock invincibilityTimer;                  // Timer to track invincibility duration
    const float INVINCIBILITY_DURATION = 1.0f; // Duration of invincibility in seconds

    // Level Up Effect Setup: Variables for the blinking text effect during level transitions
    Clock levelUpTimer;
    bool levelUpBlinkState = true; // Toggles visibility of level up text
    Clock levelUpBlinkClock;

    // Boss Mechanics Setup: Tracks boss movements to determine when to fire
    int bossMoveCounter = 0;

    // Grid System: 2D array representing the game board
    // 0 = Empty, 1 = Spaceship, 2 = Meteor, 3 = Bullet, 4 = Enemy, 5 = Boss, 6 = Boss Bullet
    int grid[ROWS][COLS] = {0};

    // Hit Effect System: Arrays to manage visual effects when entities are destroyed
    const int MAX_HIT_EFFECTS = 50;
    int hitEffectRow[MAX_HIT_EFFECTS] = {0};
    int hitEffectCol[MAX_HIT_EFFECTS] = {0};
    float hitEffectTimer[MAX_HIT_EFFECTS] = {0.0f};
    bool hitEffectActive[MAX_HIT_EFFECTS] = {false};
    const float HIT_EFFECT_DURATION = 0.3f; // Duration of the hit explosion effect

    // Spaceship Initialization: Place player at the bottom center
    int spaceshipCol = COLS / 2;      // Start column
    grid[ROWS - 1][spaceshipCol] = 1; // Mark grid position as spaceship
    Texture spaceshipTexture;
    if (!spaceshipTexture.loadFromFile("assets/images/player.png"))
    {
        cerr << "Failed to load spaceship texture" << endl;
        return -1;
    }
    Sprite spaceship;
    spaceship.setTexture(spaceshipTexture);
    // Scale sprite to fit within a single grid cell
    spaceship.setScale(
        static_cast<float>(CELL_SIZE) / spaceshipTexture.getSize().x,
        static_cast<float>(CELL_SIZE) / spaceshipTexture.getSize().y);

    // Life Icon Setup: Used for displaying remaining lives in the UI
    Texture lifeTexture;
    if (!lifeTexture.loadFromFile("assets/images/life.png"))
    {
        cerr << "Failed to load life texture" << endl;
        return -1;
    }
    Sprite lifeIcon;
    lifeIcon.setTexture(lifeTexture);
    // Scale life icon to be small (approx 24x24 pixels) to match UI text size
    lifeIcon.setScale(
        24.0f / lifeTexture.getSize().x,
        24.0f / lifeTexture.getSize().y);

    // Game Background Setup: Background image for the playing area
    Texture bgTexture;
    if (!bgTexture.loadFromFile("assets/images/backgroundColor.png"))
    {
        cerr << "Failed to load background texture" << endl;
        return -1;
    }
    Sprite background;
    background.setTexture(bgTexture);
    // Scale background to cover the entire grid area
    background.setScale(
        static_cast<float>(COLS * CELL_SIZE) / bgTexture.getSize().x,
        static_cast<float>(ROWS * CELL_SIZE) / bgTexture.getSize().y);
    background.setPosition(MARGIN, MARGIN);

    // Game Border Setup: A black outline around the playing grid
    RectangleShape gameBox(Vector2f(COLS * CELL_SIZE, ROWS * CELL_SIZE));
    gameBox.setFillColor(Color::Transparent);
    gameBox.setOutlineThickness(5);
    gameBox.setOutlineColor(Color::Black);
    gameBox.setPosition(MARGIN, MARGIN);

    // Meteor Entity Setup
    Texture meteorTexture;
    if (!meteorTexture.loadFromFile("assets/images/meteorSmall.png"))
    {
        cerr << "Failed to load meteor texture" << endl;
        return -1;
    }
    Sprite meteor;
    meteor.setTexture(meteorTexture);
    meteor.setScale(
        static_cast<float>(CELL_SIZE) / meteorTexture.getSize().x,
        static_cast<float>(CELL_SIZE) / meteorTexture.getSize().y);

    /// Enemy Entities Setup

    // Standard Enemy Setup
    Texture enemyTexture;
    if (!enemyTexture.loadFromFile("assets/images/enemyUFO.png"))
    {
        cerr << "Failed to load enemy texture" << endl;
        return -1;
    }
    Sprite enemy;
    enemy.setTexture(enemyTexture);
    enemy.setScale(
        static_cast<float>(CELL_SIZE) / enemyTexture.getSize().x,
        static_cast<float>(CELL_SIZE) / enemyTexture.getSize().y);

    // Boss Enemy Setup
    Texture bossEnemyTexture;
    if (!bossEnemyTexture.loadFromFile("assets/images/enemyShip.png"))
    {
        cerr << "Failed to load boss enemy texture" << endl;
        return -1;
    }
    Sprite bossEnemy;
    bossEnemy.setTexture(bossEnemyTexture);
    bossEnemy.setScale(
        static_cast<float>(CELL_SIZE) / bossEnemyTexture.getSize().x,
        static_cast<float>(CELL_SIZE) / bossEnemyTexture.getSize().y);

    // Player Bullet Setup
    Texture bulletTexture;
    if (!bulletTexture.loadFromFile("assets/images/laserRed.png"))
    {
        cerr << "Failed to load bullet texture" << endl;
        return -1;
    }
    Sprite bullet;
    bullet.setTexture(bulletTexture);
    // Scale bullet to be narrower (30% width) and slightly shorter (80% height) than a cell
    bullet.setScale(
        static_cast<float>(CELL_SIZE * 0.3f) / bulletTexture.getSize().x,
        static_cast<float>(CELL_SIZE * 0.8f) / bulletTexture.getSize().y);

    // Bullet Impact Effect Setup
    Texture bulletHitTexture;
    if (!bulletHitTexture.loadFromFile("assets/images/laserRedShot.png"))
    {
        cerr << "Failed to load bullet hit texture" << endl;
        return -1;
    }
    Sprite bulletHit;
    bulletHit.setTexture(bulletHitTexture);
    bulletHit.setScale(
        static_cast<float>(CELL_SIZE) / bulletHitTexture.getSize().x,
        static_cast<float>(CELL_SIZE) / bulletHitTexture.getSize().y);

    // Boss Bullet Setup
    Texture bossBulletTexture;
    if (!bossBulletTexture.loadFromFile("assets/images/laserGreen.png"))
    {
        cerr << "Failed to load boss bullet texture" << endl;
        return -1;
    }
    Sprite bossBullet;
    bossBullet.setTexture(bossBulletTexture);
    bossBullet.setScale(
        static_cast<float>(CELL_SIZE * 0.3f) / bossBulletTexture.getSize().x,
        static_cast<float>(CELL_SIZE * 0.8f) / bossBulletTexture.getSize().y);

    // Boss Bullet Impact Effect Setup
    Texture bossBulletHitTexture;
    if (!bossBulletHitTexture.loadFromFile("assets/images/laserGreenShot.png"))
    {
        cerr << "Failed to load boss bullet hit texture" << endl;
        return -1;
    }
    Sprite bossBulletHit;
    bossBulletHit.setTexture(bossBulletHitTexture);
    bossBulletHit.setScale(
        static_cast<float>(CELL_SIZE) / bossBulletHitTexture.getSize().x,
        static_cast<float>(CELL_SIZE) / bossBulletHitTexture.getSize().y);

    // Main Menu Background Setup
    Texture menuBgTexture;
    if (!menuBgTexture.loadFromFile("assets/images/starBackground.png"))
    {
        cerr << "Failed to load menu background texture" << endl;
        return -1;
    }
    Sprite menuBackground;
    menuBackground.setTexture(menuBgTexture);
    // Scale menu background to cover the entire window
    menuBackground.setScale(
        static_cast<float>(windowWidth) / menuBgTexture.getSize().x,
        static_cast<float>(windowHeight) / menuBgTexture.getSize().y);
    menuBackground.setPosition(0, 0);

    // Font Loading
    Font font;
    if (!font.loadFromFile("assets/fonts/font.ttf"))
    {
        cerr << "Failed to load font" << endl;
        return -1;
    }

    // Main Menu Title Setup
    Text menuTitle("SPACE SHOOTER", font, 40);
    menuTitle.setFillColor(Color::Yellow);
    // Center menu title horizontally
    menuTitle.setPosition(windowWidth / 2 - menuTitle.getLocalBounds().width / 2.0f, 100);

    // Main Menu Items Setup
    Text menuItems[4];
    const char menuTexts[4][20] = {"Start Game", "Load Saved Game", "Instructions", "Exit"};
    for (int i = 0; i < 4; i++)
    {
        menuItems[i].setFont(font);
        menuItems[i].setString(menuTexts[i]);
        menuItems[i].setCharacterSize(28);
        menuItems[i].setFillColor(Color::White);
        // Center each menu item horizontally under the title
        menuItems[i].setPosition(windowWidth / 2 - menuItems[i].getLocalBounds().width / 2.0f, 260 + i * 56);
    }

    // Menu Navigation Instructions Setup
    Text menuInstructions("Use UP/DOWN or W/S to navigate  |  ENTER to select", font, 18);
    menuInstructions.setFillColor(Color(150, 150, 150)); // Gray color
    menuInstructions.setPosition(windowWidth / 2 - menuInstructions.getLocalBounds().width / 2.0f, windowHeight - 80);

    /// In-Game UI Elements Setup

    // Game Title (displayed during gameplay)
    Text title("Space  Shooter  Game", font, 28);
    title.setFillColor(Color::Yellow);
    // Place title to the right of the game grid
    title.setPosition(MARGIN + COLS * CELL_SIZE + 20, MARGIN);

    // Lives Display Text
    Text livesText("Lives:", font, 20);
    livesText.setFillColor(Color::White);
    livesText.setPosition(MARGIN + COLS * CELL_SIZE + 20, MARGIN + 150);

    // Score Display Text
    Text scoreText("Score: 0", font, 20);
    scoreText.setFillColor(Color::White);
    scoreText.setPosition(MARGIN + COLS * CELL_SIZE + 20, MARGIN + 200);

    // Level Display Text
    Text levelText("Level: 1", font, 20);
    levelText.setFillColor(Color::White);
    levelText.setPosition(MARGIN + COLS * CELL_SIZE + 20, MARGIN + 250);

    // Game Over Screen Setup
    Text gameOverTitle("GAME OVER", font, 40);
    gameOverTitle.setFillColor(Color::Red);
    gameOverTitle.setPosition(windowWidth / 2 - gameOverTitle.getLocalBounds().width / 2.0f, 100);

    // Game Over Menu Items
    Text gameOverItems[2];
    const char gameOverTexts[2][20] = {"Restart", "Main Menu"};
    for (int i = 0; i < 2; i++)
    {
        gameOverItems[i].setFont(font);
        gameOverItems[i].setString(gameOverTexts[i]);
        gameOverItems[i].setCharacterSize(28);
        gameOverItems[i].setFillColor(Color::White);
        gameOverItems[i].setPosition(windowWidth / 2 - gameOverItems[i].getLocalBounds().width / 2.0f, 300 + i * 56);
    }

    // Game Over Navigation Instructions
    Text gameOverInstructions("Use UP/DOWN or W/S to navigate  |  ENTER to select", font, 18);
    gameOverInstructions.setFillColor(Color(150, 150, 150));
    gameOverInstructions.setPosition(windowWidth / 2 - gameOverInstructions.getLocalBounds().width / 2.0f, windowHeight - 80);

    // Level Up Screen Setup (Centered on the playing grid)
    Text levelUpText("LEVEL UP!", font, 40);
    levelUpText.setFillColor(Color::Green);
    // Calculate center of the grid area
    float gridCenterX = MARGIN + (COLS * CELL_SIZE) / 2.0f;
    float gridCenterY = MARGIN + (ROWS * CELL_SIZE) / 2.0f;
    levelUpText.setPosition(gridCenterX - levelUpText.getLocalBounds().width / 2.0f, gridCenterY - levelUpText.getLocalBounds().height / 2.0f - 10);

    // Pause Screen Setup
    Text pauseTitle("PAUSED", font, 40);
    pauseTitle.setFillColor(Color::Cyan);
    pauseTitle.setPosition(gridCenterX - pauseTitle.getLocalBounds().width / 2.0f, gridCenterY - 200);

    Text pauseItems[3];
    const char pauseTexts[3][20] = {"Resume", "Restart", "Main Menu"};
    for (int i = 0; i < 3; i++)
    {
        pauseItems[i].setFont(font);
        pauseItems[i].setString(pauseTexts[i]);
        pauseItems[i].setCharacterSize(28);
        pauseItems[i].setFillColor(Color::White);
        pauseItems[i].setPosition(gridCenterX - pauseItems[i].getLocalBounds().width / 2.0f, gridCenterY - 50 + i * 56);
    }

    // Victory Screen Setup
    Text victoryTitle("VICTORY!", font, 40);
    victoryTitle.setFillColor(Color::Yellow);
    victoryTitle.setPosition(windowWidth / 2 - victoryTitle.getLocalBounds().width / 2.0f, 100);

    Text victoryItems[2];
    const char victoryTexts[2][20] = {"Restart", "Main Menu"};
    for (int i = 0; i < 2; i++)
    {
        victoryItems[i].setFont(font);
        victoryItems[i].setString(victoryTexts[i]);
        victoryItems[i].setCharacterSize(28);
        victoryItems[i].setFillColor(Color::White);
        victoryItems[i].setPosition(windowWidth / 2 - victoryItems[i].getLocalBounds().width / 2.0f, 300 + i * 56);
    }

    Text victoryInstructions("Use UP/DOWN or W/S to navigate  |  ENTER to select", font, 18);
    victoryInstructions.setFillColor(Color(150, 150, 150));
    victoryInstructions.setPosition(windowWidth / 2 - victoryInstructions.getLocalBounds().width / 2.0f, windowHeight - 80);

    // Instructions Screen Setup
    Text instructionsTitle("HOW TO PLAY", font, 40);
    instructionsTitle.setFillColor(Color::Yellow);
    instructionsTitle.setPosition(windowWidth / 2 - instructionsTitle.getLocalBounds().width / 2.0f, 40);

    // Control Instructions Section
    Text controlsTitle("CONTROLS", font, 24);
    controlsTitle.setFillColor(Color::Cyan);
    controlsTitle.setPosition(50, 100);

    Text moveText("Move Left/Right: A/D or Arrow Keys", font, 18);
    moveText.setFillColor(Color::White);
    moveText.setPosition(50, 140);

    Text shootText("Shoot: SPACEBAR", font, 18);
    shootText.setFillColor(Color::White);
    shootText.setPosition(50, 170);

    Text pauseText("Pause: P", font, 18);
    pauseText.setFillColor(Color::White);
    pauseText.setPosition(50, 200);

    // Entities Explanation Section
    Text entitiesTitle("ENTITIES", font, 24);
    entitiesTitle.setFillColor(Color::Cyan);
    entitiesTitle.setPosition(50, 250);

    Text playerDesc("Your Ship", font, 18);
    playerDesc.setFillColor(Color::White);
    playerDesc.setPosition(120, 290);

    Text meteorDesc("Meteor - Avoid!", font, 18);
    meteorDesc.setFillColor(Color::White);
    meteorDesc.setPosition(120, 330);

    Text enemyDesc("Enemy - 1 Point", font, 18);
    enemyDesc.setFillColor(Color::White);
    enemyDesc.setPosition(120, 370);

    Text bossDesc("Boss - 3 Points (Level 3+)", font, 18);
    bossDesc.setFillColor(Color::White);
    bossDesc.setPosition(120, 410);

    Text bulletDesc("Your Bullet", font, 18);
    bulletDesc.setFillColor(Color::White);
    bulletDesc.setPosition(120, 450);

    Text bossBulletDesc("Boss Bullet - Avoid!", font, 18);
    bossBulletDesc.setFillColor(Color::White);
    bossBulletDesc.setPosition(120, 490);

    Text lifeDesc("Life Icon", font, 18);
    lifeDesc.setFillColor(Color::White);
    lifeDesc.setPosition(120, 530);

    // Objective Section
    Text objectiveTitle("OBJECTIVE", font, 24);
    objectiveTitle.setFillColor(Color::Cyan);
    objectiveTitle.setPosition(50, 580);

    Text objective1("- Destroy enemies and bosses to score points", font, 18);
    objective1.setFillColor(Color::White);
    objective1.setPosition(50, 620);

    Text objective2("- Each level requires (Level x 10) points", font, 18);
    objective2.setFillColor(Color::White);
    objective2.setPosition(50, 650);

    Text objective3("- Complete Level 5 to win!", font, 18);
    objective3.setFillColor(Color::White);
    objective3.setPosition(50, 680);

    Text objective4("- You have 3 lives. Don't let enemies escape!", font, 18);
    objective4.setFillColor(Color::White);
    objective4.setPosition(50, 710);

    Text instructionsBack("Press ESC or BACKSPACE to return to menu", font, 18);
    instructionsBack.setFillColor(Color(150, 150, 150));
    instructionsBack.setPosition(windowWidth / 2 - instructionsBack.getLocalBounds().width / 2.0f, windowHeight - 80);

    // Game Timing Clocks Setup

    // Movement Cooldown: Prevents overly sensitive controls
    Clock moveClock;
    Time moveCooldown = milliseconds(100); // 100ms delay between movements

    // Meteor Spawning Logic
    Clock meteorSpawnClock;
    Clock meteorMoveClock;
    float nextSpawnTime = 1.0f + (rand() % 3); // Random interval between 1-3 seconds

    // Enemy Spawning Logic
    Clock enemySpawnClock;
    Clock enemyMoveClock;
    float nextEnemySpawnTime = 2.0f + (rand() % 4); // Random interval between 2-5 seconds

    // Boss Spawning Logic (Bosses appear after level 3)
    Clock bossSpawnClock;
    Clock bossMoveClock;
    Clock bossBulletMoveClock;
    float nextBossSpawnTime = 8.0f + (rand() % 5); // Random interval between 8-12 seconds

    // Bullet Firing Logic
    Clock bulletMoveClock;
    Clock bulletFireClock;
    Time bulletFireCooldown = milliseconds(300); // Fire rate limit: 0.3 seconds

    // Hit Effect Timer
    Clock hitEffectClock;

    // Menu Navigation Cooldown
    Clock menuClock;
    Time menuCooldown = milliseconds(200);

    // Main Game Loop: Runs until the window is closed
    while (window.isOpen())
    {

        // Event Polling: Handle window close events
        Event event;
        while (window.pollEvent(event))
        {
            if (event.type == Event::Closed)
                window.close();
        }

        // State Machine: Handle logic based on current game state
        if (currentState == STATE_MENU)
        {
            // Menu Logic: Handle navigation and selection
            if (menuClock.getElapsedTime() >= menuCooldown)
            {
                bool menuAction = false;

                // Navigate menu up/down
                if (Keyboard::isKeyPressed(Keyboard::Up) || Keyboard::isKeyPressed(Keyboard::W))
                {
                    selectedMenuItem = (selectedMenuItem - 1 + 4) % 4;
                    menuAction = true;
                }
                else if (Keyboard::isKeyPressed(Keyboard::Down) || Keyboard::isKeyPressed(Keyboard::S))
                {
                    selectedMenuItem = (selectedMenuItem + 1) % 4;
                    menuAction = true;
                }
                // Select menu item
                else if (Keyboard::isKeyPressed(Keyboard::Enter))
                {
                    if (selectedMenuItem == 0)
                    { // Start Game
                        currentState = STATE_PLAYING;
                        // Reset game state variables for a new game
                        lives = 3;
                        score = 0;
                        level = 1;
                        bossMoveCounter = 0;
                        // Clear the grid
                        for (int r = 0; r < ROWS; r++)
                        {
                            for (int c = 0; c < COLS; c++)
                            {
                                grid[r][c] = 0;
                            }
                        }
                        // Reset spaceship position
                        spaceshipCol = COLS / 2;
                        grid[ROWS - 1][spaceshipCol] = 1;
                        // Restart all game clocks
                        meteorSpawnClock.restart();
                        meteorMoveClock.restart();
                        enemySpawnClock.restart();
                        enemyMoveClock.restart();
                        bossSpawnClock.restart();
                        bossMoveClock.restart();
                        bossBulletMoveClock.restart();
                        bulletMoveClock.restart();
                    }
                    else if (selectedMenuItem == 2)
                    { // Instructions
                        currentState = STATE_INSTRUCTIONS;
                    }
                    else if (selectedMenuItem == 3)
                    { // Exit
                        window.close();
                    }
                    menuAction = true;
                }

                if (menuAction)
                {
                    menuClock.restart();
                }
            }

            // Update menu item colors to highlight selection
            for (int i = 0; i < 4; i++)
            {
                if (i == selectedMenuItem)
                {
                    menuItems[i].setFillColor(Color::Yellow);
                }
                else
                {
                    menuItems[i].setFillColor(Color::White);
                }
            }
        }
        // Game Over State Logic
        else if (currentState == STATE_GAME_OVER)
        {
            // Game over menu navigation with cooldown
            if (menuClock.getElapsedTime() >= menuCooldown)
            {
                bool menuAction = false;

                // Navigate game over menu up/down
                if (Keyboard::isKeyPressed(Keyboard::Up) || Keyboard::isKeyPressed(Keyboard::W))
                {
                    selectedMenuItem = (selectedMenuItem - 1 + 2) % 2;
                    menuAction = true;
                }
                else if (Keyboard::isKeyPressed(Keyboard::Down) || Keyboard::isKeyPressed(Keyboard::S))
                {
                    selectedMenuItem = (selectedMenuItem + 1) % 2;
                    menuAction = true;
                }
                // Select game over menu item
                else if (Keyboard::isKeyPressed(Keyboard::Enter))
                {
                    if (selectedMenuItem == 0)
                    { // Restart Game
                        currentState = STATE_PLAYING;
                        // Reset game state
                        lives = 3;
                        score = 0;
                        level = 1;
                        bossMoveCounter = 0;
                        for (int r = 0; r < ROWS; r++)
                        {
                            for (int c = 0; c < COLS; c++)
                            {
                                grid[r][c] = 0;
                            }
                        }
                        spaceshipCol = COLS / 2;
                        grid[ROWS - 1][spaceshipCol] = 1;
                        meteorSpawnClock.restart();
                        meteorMoveClock.restart();
                        enemySpawnClock.restart();
                        enemyMoveClock.restart();
                        bossSpawnClock.restart();
                        bossMoveClock.restart();
                        bossBulletMoveClock.restart();
                        bulletMoveClock.restart();
                    }
                    else if (selectedMenuItem == 1)
                    { // Return to Main Menu
                        currentState = STATE_MENU;
                        selectedMenuItem = 0;
                    }
                    menuAction = true;
                }

                if (menuAction)
                {
                    menuClock.restart();
                }
            }

            // Update game over item colors based on selection
            for (int i = 0; i < 2; i++)
            {
                if (i == selectedMenuItem)
                {
                    gameOverItems[i].setFillColor(Color::Yellow);
                }
                else
                {
                    gameOverItems[i].setFillColor(Color::White);
                }
            }
        }
        // Instructions Screen Logic
        else if (currentState == STATE_INSTRUCTIONS)
        {
            // Check for input to return to menu
            if (menuClock.getElapsedTime() >= menuCooldown)
            {
                if (Keyboard::isKeyPressed(Keyboard::Escape) || Keyboard::isKeyPressed(Keyboard::BackSpace))
                {
                    currentState = STATE_MENU;
                    selectedMenuItem = 0;
                    menuClock.restart();
                }
            }
        }
        // Gameplay State Logic: Main game mechanics happen here
        else if (currentState == STATE_PLAYING)
        {
            // Check for pause input
            if (menuClock.getElapsedTime() >= menuCooldown)
            {
                if (Keyboard::isKeyPressed(Keyboard::P))
                {
                    currentState = STATE_PAUSED;
                    selectedMenuItem = 0;
                    menuClock.restart();
                    cout << "Game Paused" << endl;
                }
            }

            // Player Movement Logic: Handle Left/Right input
            if (moveClock.getElapsedTime() >= moveCooldown)
            {
                bool moved = false;
                // Move Left
                if ((Keyboard::isKeyPressed(Keyboard::Left) || Keyboard::isKeyPressed(Keyboard::A)) && spaceshipCol > 0)
                {
                    grid[ROWS - 1][spaceshipCol] = 0; // Clear old position
                    spaceshipCol--;
                    grid[ROWS - 1][spaceshipCol] = 1; // Set new position
                    cout << "Moved Left to column " << spaceshipCol << endl;
                    moved = true;
                }
                // Move Right
                else if ((Keyboard::isKeyPressed(Keyboard::Right) || Keyboard::isKeyPressed(Keyboard::D)) && spaceshipCol < COLS - 1)
                {
                    grid[ROWS - 1][spaceshipCol] = 0; // Clear old position
                    spaceshipCol++;
                    grid[ROWS - 1][spaceshipCol] = 1; // Set new position
                    cout << "Moved Right to column " << spaceshipCol << endl;
                    moved = true;
                }

                if (moved)
                {
                    moveClock.restart();
                }
            }

            // Player Shooting Logic
            if (Keyboard::isKeyPressed(Keyboard::Space) && bulletFireClock.getElapsedTime() >= bulletFireCooldown)
            {
                // Fire bullet from position immediately above spaceship
                int bulletRow = ROWS - 2;
                if (bulletRow >= 0 && grid[bulletRow][spaceshipCol] == 0)
                {
                    grid[bulletRow][spaceshipCol] = 3; // 3 represents bullet
                    cout << "Bullet fired from column " << spaceshipCol << endl;
                }
                bulletFireCooldown = milliseconds(300);
                bulletFireClock.restart();
            }

            // Meteor Spawning Logic
            if (meteorSpawnClock.getElapsedTime().asSeconds() >= nextSpawnTime)
            {
                int randomCol = rand() % COLS;
                // Only spawn if the top row at that column is empty
                if (grid[0][randomCol] == 0)
                {
                    grid[0][randomCol] = 2; // 2 represents meteor
                    cout << "Meteor spawned at column " << randomCol << endl;
                }
                meteorSpawnClock.restart();
                nextSpawnTime = 1.0f + (rand() % 3); // Next spawn in 1-3 seconds
            }

            // Enemy Spawning Logic (Spawn rate increases with level)
            if (enemySpawnClock.getElapsedTime().asSeconds() >= nextEnemySpawnTime)
            {
                int randomCol = rand() % COLS;
                // Only spawn if the top row at that column is empty
                if (grid[0][randomCol] == 0)
                {
                    grid[0][randomCol] = 4; // 4 represents enemy
                    cout << "Enemy spawned at column " << randomCol << endl;
                }
                enemySpawnClock.restart();
                // Calculate next spawn time based on level difficulty
                float baseTime = 2.5f - (level * 0.4f);
                float variance = 3.0f - (level * 0.4f);
                if (baseTime < 0.5f)
                    baseTime = 0.5f;
                if (variance < 1.0f)
                    variance = 1.0f;
                nextEnemySpawnTime = baseTime + (rand() % (int)variance);
            }

            // Boss Spawning Logic (Only spawns at Level 3 and above)
            if (level >= 3 && bossSpawnClock.getElapsedTime().asSeconds() >= nextBossSpawnTime)
            {
                int randomCol = rand() % COLS;
                // Only spawn if the top row at that column is empty
                if (grid[0][randomCol] == 0)
                {
                    grid[0][randomCol] = 5; // 5 represents boss
                    cout << "Boss spawned at column " << randomCol << endl;
                }
                bossSpawnClock.restart();
                // Calculate next boss spawn time based on level
                float bossBaseTime = 10.0f - ((level - 3) * 1.5f);
                float bossVariance = 4.0f;
                if (bossBaseTime < 5.0f)
                    bossBaseTime = 5.0f;
                nextBossSpawnTime = bossBaseTime + (rand() % (int)bossVariance);
            }

            // Meteor Movement Logic (Moves down every 0.833 seconds)
            if (meteorMoveClock.getElapsedTime().asSeconds() >= 0.833f)
            {
                // Iterate from bottom to top to avoid overwriting entities as they move down
                for (int r = ROWS - 1; r >= 0; r--)
                {
                    for (int c = 0; c < COLS; c++)
                    {
                        if (grid[r][c] == 2)
                        { // Found a meteor
                            // Check if meteor reached the bottom of the grid
                            if (r == ROWS - 1)
                            {
                                grid[r][c] = 0; // Remove meteor
                                cout << "Meteor removed at bottom" << endl;
                            }
                            else
                            {
                                grid[r][c] = 0; // Clear current position
                                // Move down if next cell is empty or contains another meteor
                                if (grid[r + 1][c] == 0 || grid[r + 1][c] == 2)
                                {
                                    grid[r + 1][c] = 2;
                                }
                                // Collision with Spaceship
                                else if (grid[r + 1][c] == 1)
                                {
                                    if (!isInvincible)
                                    {
                                        lives--;
                                        isInvincible = true;
                                        invincibilityTimer.restart();
                                        cout << "Meteor hit spaceship! Lives remaining: " << lives << endl;
                                        // Check for Game Over
                                        if (lives <= 0)
                                        {
                                            currentState = STATE_GAME_OVER;
                                            selectedMenuItem = 0;
                                            cout << "Game Over!" << endl;
                                        }
                                    }
                                    grid[r + 1][c] = 0; // Remove meteor
                                }
                                // Collision with Player Bullet
                                else if (grid[r + 1][c] == 3)
                                {
                                    cout << "Meteor destroyed by bullet!" << endl;
                                    grid[r + 1][c] = 0; // Destroy both meteor and bullet
                                    // Trigger hit effect
                                    for (int i = 0; i < MAX_HIT_EFFECTS; i++)
                                    {
                                        if (!hitEffectActive[i])
                                        {
                                            hitEffectRow[i] = r + 1;
                                            hitEffectCol[i] = c;
                                            hitEffectTimer[i] = 0.0f;
                                            hitEffectActive[i] = true;
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                meteorMoveClock.restart();
            }

            // Enemy Movement Logic (Speed increases with level)
            float enemyMoveSpeed = 0.833f - ((level - 1) * 0.1f);
            if (enemyMoveClock.getElapsedTime().asSeconds() >= enemyMoveSpeed)
            {
                // Iterate from bottom to top
                for (int r = ROWS - 1; r >= 0; r--)
                {
                    for (int c = 0; c < COLS; c++)
                    {
                        if (grid[r][c] == 4)
                        { // Found an enemy
                            // Check if enemy reached the bottom (Player loses a life)
                            if (r == ROWS - 1)
                            {
                                grid[r][c] = 0; // Remove enemy
                                lives--;        // Penalty for letting enemy escape
                                isInvincible = true;
                                invincibilityTimer.restart();
                                cout << "Enemy escaped! Lives remaining: " << lives << endl;
                                // Check for Game Over
                                if (lives <= 0)
                                {
                                    currentState = STATE_GAME_OVER;
                                    selectedMenuItem = 0;
                                    cout << "Game Over!" << endl;
                                }
                            }
                            else
                            {
                                grid[r][c] = 0; // Clear current position
                                // Move down if next cell is empty or contains another enemy
                                if (grid[r + 1][c] == 0 || grid[r + 1][c] == 4)
                                {
                                    grid[r + 1][c] = 4;
                                }
                                // Collision with Spaceship
                                else if (grid[r + 1][c] == 1)
                                {
                                    if (!isInvincible)
                                    {
                                        lives--;
                                        isInvincible = true;
                                        invincibilityTimer.restart();
                                        cout << "Enemy hit spaceship! Lives remaining: " << lives << endl;
                                        // Check for Game Over
                                        if (lives <= 0)
                                        {
                                            currentState = STATE_GAME_OVER;
                                            selectedMenuItem = 0;
                                            cout << "Game Over!" << endl;
                                        }
                                    }
                                    grid[r + 1][c] = 0; // Remove enemy
                                }
                                // Collision with Player Bullet
                                else if (grid[r + 1][c] == 3)
                                {
                                    score += 1; // Award points
                                    cout << "Enemy destroyed by bullet! +1 point. Score: " << score << endl;
                                    grid[r + 1][c] = 0; // Destroy both
                                    // Trigger hit effect
                                    for (int i = 0; i < MAX_HIT_EFFECTS; i++)
                                    {
                                        if (!hitEffectActive[i])
                                        {
                                            hitEffectRow[i] = r + 1;
                                            hitEffectCol[i] = c;
                                            hitEffectTimer[i] = 0.0f;
                                            hitEffectActive[i] = true;
                                            break;
                                        }
                                    }
                                    // Check for Level Up Condition
                                    int scoreNeeded = level * 10;
                                    if (level < MAX_LEVEL && score >= scoreNeeded)
                                    {
                                        level++;
                                        score = 0;           // Reset score for next level
                                        bossMoveCounter = 0; // Reset boss firing counter
                                        // Clear grid of all entities
                                        for (int r = 0; r < ROWS; r++)
                                        {
                                            for (int c = 0; c < COLS; c++)
                                            {
                                                if (grid[r][c] >= 2 && grid[r][c] <= 6)
                                                {
                                                    grid[r][c] = 0;
                                                }
                                            }
                                        }
                                        // Reset spaceship position
                                        grid[ROWS - 1][spaceshipCol] = 0;
                                        spaceshipCol = COLS / 2;
                                        grid[ROWS - 1][spaceshipCol] = 1;
                                        // Transition to Level Up State
                                        currentState = STATE_LEVEL_UP;
                                        levelUpTimer.restart();
                                        levelUpBlinkClock.restart();
                                        cout << "Level Up! Now at Level " << level << endl;
                                    }
                                    else if (level >= MAX_LEVEL && score >= scoreNeeded)
                                    {
                                        // Victory Condition Met
                                        currentState = STATE_VICTORY;
                                        selectedMenuItem = 0;
                                        cout << "Victory! Game Complete!" << endl;
                                    }
                                }
                            }
                        }
                    }
                }
                enemyMoveClock.restart();
            }

            // Boss Movement Logic (Speed increases with level)
            float bossMoveSpeed = 0.8f - ((level - 3) * 0.1f);
            if (bossMoveSpeed < 0.5f)
                bossMoveSpeed = 0.5f;
            if (bossMoveClock.getElapsedTime().asSeconds() >= bossMoveSpeed)
            {
                // Iterate from bottom to top
                for (int r = ROWS - 1; r >= 0; r--)
                {
                    for (int c = 0; c < COLS; c++)
                    {
                        if (grid[r][c] == 5)
                        { // Found a boss
                            // Check if boss reached the bottom (Player loses a life)
                            if (r == ROWS - 1)
                            {
                                grid[r][c] = 0; // Remove boss
                                lives--;        // Penalty
                                isInvincible = true;
                                invincibilityTimer.restart();
                                cout << "Boss escaped! Lives remaining: " << lives << endl;
                                // Check for Game Over
                                if (lives <= 0)
                                {
                                    currentState = STATE_GAME_OVER;
                                    selectedMenuItem = 0;
                                    cout << "Game Over!" << endl;
                                }
                            }
                            else
                            {
                                int nextRow = r + 1;
                                int nextCell = grid[nextRow][c];

                                grid[r][c] = 0; // Clear current position

                                // Move down if next cell is empty or contains other enemies/projectiles
                                if (nextCell == 0 || nextCell == 5 || nextCell == 6 || nextCell == 2 || nextCell == 4)
                                {
                                    grid[nextRow][c] = 5;
                                }
                                // Collision with Spaceship
                                else if (grid[r + 1][c] == 1)
                                {
                                    if (!isInvincible)
                                    {
                                        lives--;
                                        isInvincible = true;
                                        invincibilityTimer.restart();
                                        cout << "Boss hit spaceship! Lives remaining: " << lives << endl;
                                        // Check for Game Over
                                        if (lives <= 0)
                                        {
                                            currentState = STATE_GAME_OVER;
                                            selectedMenuItem = 0;
                                            cout << "Game Over!" << endl;
                                        }
                                    }
                                    grid[r + 1][c] = 0; // Remove boss
                                }
                                // Collision with Player Bullet
                                else if (grid[r + 1][c] == 3)
                                {
                                    score += 3; // Award 3 points for boss
                                    cout << "Boss destroyed by bullet! +3 points. Score: " << score << endl;
                                    grid[r + 1][c] = 0; // Destroy both
                                    // Trigger hit effect
                                    for (int i = 0; i < MAX_HIT_EFFECTS; i++)
                                    {
                                        if (!hitEffectActive[i])
                                        {
                                            hitEffectRow[i] = r + 1;
                                            hitEffectCol[i] = c;
                                            hitEffectTimer[i] = 0.0f;
                                            hitEffectActive[i] = true;
                                            break;
                                        }
                                    }
                                    // Check for Level Up Condition
                                    int scoreNeeded = level * 10;
                                    if (level < MAX_LEVEL && score >= scoreNeeded)
                                    {
                                        level++;
                                        score = 0;
                                        bossMoveCounter = 0;
                                        // Clear grid
                                        for (int r = 0; r < ROWS; r++)
                                        {
                                            for (int c = 0; c < COLS; c++)
                                            {
                                                if (grid[r][c] >= 2 && grid[r][c] <= 6)
                                                {
                                                    grid[r][c] = 0;
                                                }
                                            }
                                        }
                                        // Reset spaceship
                                        grid[ROWS - 1][spaceshipCol] = 0;
                                        spaceshipCol = COLS / 2;
                                        grid[ROWS - 1][spaceshipCol] = 1;
                                        // Transition to Level Up
                                        currentState = STATE_LEVEL_UP;
                                        levelUpTimer.restart();
                                        levelUpBlinkClock.restart();
                                        cout << "Level Up! Now at Level " << level << endl;
                                    }
                                    else if (level >= MAX_LEVEL && score >= scoreNeeded)
                                    {
                                        // Victory Condition
                                        currentState = STATE_VICTORY;
                                        selectedMenuItem = 0;
                                        cout << "Victory! Game Complete!" << endl;
                                    }
                                }
                            }
                        }
                    }
                }

                // Boss Firing Logic
                bossMoveCounter++;

                // Determine firing frequency based on level
                int firingInterval;
                if (level == 3)
                {
                    firingInterval = 3; // Fire every 3rd movement
                }
                else if (level == 4)
                {
                    firingInterval = 2; // Fire every 2nd movement
                }
                else
                {                       // Level 5
                    firingInterval = 1.5; // Fire almost every movement
                }

                // Execute Boss Firing
                if (bossMoveCounter >= firingInterval)
                {
                    for (int r = 0; r < ROWS; r++)
                    {
                        for (int c = 0; c < COLS; c++)
                        {
                            if (grid[r][c] == 5)
                            { // Found a boss
                                // Fire bullet downwards
                                if (r < ROWS - 1)
                                {
                                    int bulletRow = r + 1;
                                    if (bulletRow < ROWS && grid[bulletRow][c] == 0)
                                    {
                                        grid[bulletRow][c] = 6; // 6 represents boss bullet
                                        cout << "Boss fired bullet at column " << c << " (Level " << level << ")" << endl;
                                    }
                                }
                            }
                        }
                    }
                    bossMoveCounter = 0; // Reset counter
                }

                bossMoveClock.restart();
            }

            // Boss Bullet Movement Logic (Moves faster than bosses)
            float bossBulletSpeed = bossMoveSpeed / 2.0f;
            if (bossBulletMoveClock.getElapsedTime().asSeconds() >= bossBulletSpeed)
            {
                // Iterate from bottom to top
                for (int r = ROWS - 1; r >= 0; r--)
                {
                    for (int c = 0; c < COLS; c++)
                    {
                        if (grid[r][c] == 6)
                        { // Found a boss bullet
                            // Check if bullet reached the bottom
                            if (r == ROWS - 1)
                            {
                                grid[r][c] = 0; // Remove bullet
                            }
                            else
                            {
                                grid[r][c] = 0; // Clear current position
                                // Collision with Spaceship
                                if (grid[r + 1][c] == 1)
                                {
                                    if (!isInvincible)
                                    {
                                        lives--;
                                        isInvincible = true;
                                        invincibilityTimer.restart();
                                        cout << "Boss bullet hit spaceship! Lives remaining: " << lives << endl;
                                        // Check for Game Over
                                        if (lives <= 0)
                                        {
                                            currentState = STATE_GAME_OVER;
                                            selectedMenuItem = 0;
                                            cout << "Game Over!" << endl;
                                        }
                                    }
                                    // Trigger hit effect
                                    for (int i = 0; i < MAX_HIT_EFFECTS; i++)
                                    {
                                        if (!hitEffectActive[i])
                                        {
                                            hitEffectRow[i] = r + 1;
                                            hitEffectCol[i] = c;
                                            hitEffectTimer[i] = 0.0f;
                                            hitEffectActive[i] = true;
                                            break;
                                        }
                                    }
                                }
                                // Pass through other enemies/meteors
                                else if (grid[r + 1][c] == 2 || grid[r + 1][c] == 4)
                                {
                                    grid[r + 1][c] = 6; // Bullet continues, entity stays
                                }
                                // Move to empty space or overwrite another boss bullet
                                else if (grid[r + 1][c] == 0 || grid[r + 1][c] == 6)
                                {
                                    grid[r + 1][c] = 6;
                                }
                            }
                        }
                    }
                }
                bossBulletMoveClock.restart();
            }

            // Player Bullet Movement Logic (Moves up)
            if (bulletMoveClock.getElapsedTime().asSeconds() >= 0.05f)
            {
                // Iterate from top to bottom to avoid overwriting
                for (int r = 0; r < ROWS; r++)
                {
                    for (int c = 0; c < COLS; c++)
                    {
                        if (grid[r][c] == 3)
                        { // Found a player bullet
                            // Check if bullet reached the top
                            if (r == 0)
                            {
                                grid[r][c] = 0; // Remove bullet
                                cout << "Bullet removed at top" << endl;
                            }
                            else
                            {
                                grid[r][c] = 0; // Clear current position
                                // Move up if next cell is empty or contains another bullet
                                if (grid[r - 1][c] == 0 || grid[r - 1][c] == 3)
                                {
                                    grid[r - 1][c] = 3;
                                }
                                // Collision with Boss Bullet
                                else if (grid[r - 1][c] == 6)
                                {
                                    cout << "Player bullet destroyed boss bullet!" << endl;
                                    grid[r - 1][c] = 0; // Destroy both
                                    // Trigger hit effect
                                    for (int i = 0; i < MAX_HIT_EFFECTS; i++)
                                    {
                                        if (!hitEffectActive[i])
                                        {
                                            hitEffectRow[i] = r - 1;
                                            hitEffectCol[i] = c;
                                            hitEffectTimer[i] = 0.0f;
                                            hitEffectActive[i] = true;
                                            break;
                                        }
                                    }
                                }
                                // Collision with Meteor
                                else if (grid[r - 1][c] == 2)
                                {
                                    cout << "Bullet hit meteor!" << endl;
                                    grid[r - 1][c] = 0; // Destroy both
                                    // Trigger hit effect
                                    for (int i = 0; i < MAX_HIT_EFFECTS; i++)
                                    {
                                        if (!hitEffectActive[i])
                                        {
                                            hitEffectRow[i] = r - 1;
                                            hitEffectCol[i] = c;
                                            hitEffectTimer[i] = 0.0f;
                                            hitEffectActive[i] = true;
                                            break;
                                        }
                                    }
                                }
                                // Collision with Enemy
                                else if (grid[r - 1][c] == 4)
                                {
                                    score += 1; // Award points
                                    cout << "Bullet hit enemy! +1 point. Score: " << score << endl;
                                    grid[r - 1][c] = 0; // Destroy both
                                    // Trigger hit effect
                                    for (int i = 0; i < MAX_HIT_EFFECTS; i++)
                                    {
                                        if (!hitEffectActive[i])
                                        {
                                            hitEffectRow[i] = r - 1;
                                            hitEffectCol[i] = c;
                                            hitEffectTimer[i] = 0.0f;
                                            hitEffectActive[i] = true;
                                            break;
                                        }
                                    }
                                    // Check for Level Up
                                    int scoreNeeded = level * 10;
                                    if (level < MAX_LEVEL && score >= scoreNeeded)
                                    {
                                        level++;
                                        score = 0;
                                        bossMoveCounter = 0;
                                        // Clear grid
                                        for (int r = 0; r < ROWS; r++)
                                        {
                                            for (int c = 0; c < COLS; c++)
                                            {
                                                if (grid[r][c] >= 2 && grid[r][c] <= 6)
                                                {
                                                    grid[r][c] = 0;
                                                }
                                            }
                                        }
                                        // Reset spaceship
                                        grid[ROWS - 1][spaceshipCol] = 0;
                                        spaceshipCol = COLS / 2;
                                        grid[ROWS - 1][spaceshipCol] = 1;
                                        // Transition to Level Up
                                        currentState = STATE_LEVEL_UP;
                                        levelUpTimer.restart();
                                        levelUpBlinkClock.restart();
                                        cout << "Level Up! Now at Level " << level << endl;
                                    }
                                    else if (level >= MAX_LEVEL && score >= scoreNeeded)
                                    {
                                        // Victory Condition
                                        currentState = STATE_VICTORY;
                                        selectedMenuItem = 0;
                                        cout << "Victory! Game Complete!" << endl;
                                    }
                                }
                                // Collision with Boss
                                else if (grid[r - 1][c] == 5)
                                {
                                    score += 3; // Award points
                                    cout << "Bullet hit boss! +3 points. Score: " << score << endl;
                                    grid[r - 1][c] = 0; // Destroy both
                                    // Trigger hit effect
                                    for (int i = 0; i < MAX_HIT_EFFECTS; i++)
                                    {
                                        if (!hitEffectActive[i])
                                        {
                                            hitEffectRow[i] = r - 1;
                                            hitEffectCol[i] = c;
                                            hitEffectTimer[i] = 0.0f;
                                            hitEffectActive[i] = true;
                                            break;
                                        }
                                    }
                                    // Check for Level Up
                                    int scoreNeeded = level * 10;
                                    if (level < MAX_LEVEL && score >= scoreNeeded)
                                    {
                                        level++;
                                        score = 0;
                                        bossMoveCounter = 0;
                                        // Clear grid
                                        for (int r = 0; r < ROWS; r++)
                                        {
                                            for (int c = 0; c < COLS; c++)
                                            {
                                                if (grid[r][c] >= 2 && grid[r][c] <= 6)
                                                {
                                                    grid[r][c] = 0;
                                                }
                                            }
                                        }
                                        // Reset spaceship
                                        grid[ROWS - 1][spaceshipCol] = 0;
                                        spaceshipCol = COLS / 2;
                                        grid[ROWS - 1][spaceshipCol] = 1;
                                        // Transition to Level Up
                                        currentState = STATE_LEVEL_UP;
                                        levelUpTimer.restart();
                                        levelUpBlinkClock.restart();
                                        cout << "Level Up! Now at Level " << level << endl;
                                    }
                                    else if (level >= MAX_LEVEL && score >= scoreNeeded)
                                    {
                                        // Victory Condition
                                        currentState = STATE_VICTORY;
                                        selectedMenuItem = 0;
                                        cout << "Victory! Game Complete!" << endl;
                                    }
                                }
                            }
                        }
                    }
                }
                bulletMoveClock.restart();
            }

            // Update Hit Effects (Remove after duration)
            float deltaTime = hitEffectClock.getElapsedTime().asSeconds();
            for (int i = 0; i < MAX_HIT_EFFECTS; i++)
            {
                if (hitEffectActive[i])
                {
                    hitEffectTimer[i] += deltaTime;
                    if (hitEffectTimer[i] >= HIT_EFFECT_DURATION)
                    {
                        hitEffectActive[i] = false; // Deactivate
                    }
                }
            }
            hitEffectClock.restart();

            // Update Invincibility Status
            if (isInvincible && invincibilityTimer.getElapsedTime().asSeconds() >= INVINCIBILITY_DURATION)
            {
                isInvincible = false;
                cout << "Invincibility ended" << endl;
            }

        } // End of PLAYING state

        // Level Up State Logic
        else if (currentState == STATE_LEVEL_UP)
        {
            // Blink effect for "LEVEL UP" text
            if (levelUpBlinkClock.getElapsedTime().asSeconds() >= 0.3f)
            {
                levelUpBlinkState = !levelUpBlinkState;
                levelUpBlinkClock.restart();
            }

            // Return to gameplay after 2 seconds
            if (levelUpTimer.getElapsedTime().asSeconds() >= 2.0f)
            {
                currentState = STATE_PLAYING;
                meteorSpawnClock.restart();
                meteorMoveClock.restart();
                enemySpawnClock.restart();
                enemyMoveClock.restart();
                bossSpawnClock.restart();
                bossMoveClock.restart();
                bossBulletMoveClock.restart();
            }
        }

        // Victory State Logic
        else if (currentState == STATE_VICTORY)
        {
            // Victory menu navigation
            if (menuClock.getElapsedTime() >= menuCooldown)
            {
                bool menuAction = false;

                if (Keyboard::isKeyPressed(Keyboard::Up) || Keyboard::isKeyPressed(Keyboard::W))
                {
                    selectedMenuItem = (selectedMenuItem - 1 + 2) % 2;
                    menuAction = true;
                }
                else if (Keyboard::isKeyPressed(Keyboard::Down) || Keyboard::isKeyPressed(Keyboard::S))
                {
                    selectedMenuItem = (selectedMenuItem + 1) % 2;
                    menuAction = true;
                }
                else if (Keyboard::isKeyPressed(Keyboard::Enter))
                {
                    if (selectedMenuItem == 0)
                    { // Restart Game
                        currentState = STATE_PLAYING;
                        lives = 3;
                        score = 0;
                        level = 1;
                        bossMoveCounter = 0;
                        isInvincible = false;
                        for (int r = 0; r < ROWS; r++)
                        {
                            for (int c = 0; c < COLS; c++)
                            {
                                grid[r][c] = 0;
                            }
                        }
                        spaceshipCol = COLS / 2;
                        grid[ROWS - 1][spaceshipCol] = 1;
                        meteorSpawnClock.restart();
                        meteorMoveClock.restart();
                        enemySpawnClock.restart();
                        enemyMoveClock.restart();
                        bossSpawnClock.restart();
                        bossMoveClock.restart();
                        bossBulletMoveClock.restart();
                        bulletMoveClock.restart();
                    }
                    else if (selectedMenuItem == 1)
                    { // Main Menu
                        currentState = STATE_MENU;
                        selectedMenuItem = 0;
                    }
                    menuAction = true;
                }

                if (menuAction)
                {
                    menuClock.restart();
                }
            }
        }

        // Pause State Logic
        else if (currentState == STATE_PAUSED)
        {
            // Pause menu navigation
            if (menuClock.getElapsedTime() >= menuCooldown)
            {
                bool menuAction = false;

                if (Keyboard::isKeyPressed(Keyboard::Up) || Keyboard::isKeyPressed(Keyboard::W))
                {
                    selectedMenuItem = (selectedMenuItem - 1 + 3) % 3;
                    menuAction = true;
                }
                else if (Keyboard::isKeyPressed(Keyboard::Down) || Keyboard::isKeyPressed(Keyboard::S))
                {
                    selectedMenuItem = (selectedMenuItem + 1) % 3;
                    menuAction = true;
                }
                else if (Keyboard::isKeyPressed(Keyboard::Enter))
                {
                    if (selectedMenuItem == 0)
                    { // Resume Game
                        currentState = STATE_PLAYING;
                        cout << "Game Resumed" << endl;
                    }
                    else if (selectedMenuItem == 1)
                    { // Restart Level (Keep level/lives, reset score)
                        currentState = STATE_PLAYING;
                        score = 0;
                        bossMoveCounter = 0;
                        isInvincible = false;
                        // Clear grid but keep spaceship
                        for (int r = 0; r < ROWS; r++)
                        {
                            for (int c = 0; c < COLS; c++)
                            {
                                grid[r][c] = 0;
                            }
                        }
                        spaceshipCol = COLS / 2;
                        grid[ROWS - 1][spaceshipCol] = 1;
                        meteorSpawnClock.restart();
                        meteorMoveClock.restart();
                        enemySpawnClock.restart();
                        enemyMoveClock.restart();
                        bossSpawnClock.restart();
                        bossMoveClock.restart();
                        bossBulletMoveClock.restart();
                        bulletMoveClock.restart();
                        cout << "Game Restarted at Level " << level << " with " << lives << " lives" << endl;
                    }
                    else if (selectedMenuItem == 2)
                    { // Main Menu
                        currentState = STATE_MENU;
                        selectedMenuItem = 0;
                    }
                    menuAction = true;
                }
                else if (Keyboard::isKeyPressed(Keyboard::P))
                { // P to resume
                    currentState = STATE_PLAYING;
                    cout << "Game Resumed" << endl;
                    menuAction = true;
                }

                if (menuAction)
                {
                    menuClock.restart();
                }
            }
        }

        // Rendering Section: Draw everything to the window
        window.clear(Color(40, 40, 40)); // Clear with dark gray background

        if (currentState == STATE_MENU)
        {
            // Draw Main Menu
            window.draw(menuBackground);
            window.draw(menuTitle);
            for (int i = 0; i < 4; i++)
            {
                if (i == selectedMenuItem)
                {
                    menuItems[i].setFillColor(Color::Yellow);
                }
                else
                {
                    menuItems[i].setFillColor(Color::White);
                }
                window.draw(menuItems[i]);
            }
            window.draw(menuInstructions);
        }
        else if (currentState == STATE_INSTRUCTIONS)
        {
            // Draw Instructions Screen
            window.draw(menuBackground);
            window.draw(instructionsTitle);

            // Draw controls
            window.draw(controlsTitle);
            window.draw(moveText);
            window.draw(shootText);
            window.draw(pauseText);

            // Draw entities
            window.draw(entitiesTitle);

            // Player sprite
            spaceship.setPosition(60, 285);
            window.draw(spaceship);
            window.draw(playerDesc);

            // Meteor sprite
            meteor.setPosition(60, 325);
            window.draw(meteor);
            window.draw(meteorDesc);

            // Enemy sprite
            enemy.setPosition(60, 365);
            window.draw(enemy);
            window.draw(enemyDesc);

            // Boss sprite
            bossEnemy.setPosition(60, 405);
            window.draw(bossEnemy);
            window.draw(bossDesc);

            // Player bullet sprite
            float bulletWidth = CELL_SIZE * 0.3f;
            float xOffset = (CELL_SIZE - bulletWidth) / 2.0f;
            bullet.setPosition(60 + xOffset, 445);
            window.draw(bullet);
            window.draw(bulletDesc);

            // Boss bullet sprite
            bossBullet.setPosition(60 + xOffset, 485);
            window.draw(bossBullet);
            window.draw(bossBulletDesc);

            // Life icon
            lifeIcon.setPosition(60 + 8, 525);
            window.draw(lifeIcon);
            window.draw(lifeDesc);

            // Draw objectives
            window.draw(objectiveTitle);
            window.draw(objective1);
            window.draw(objective2);
            window.draw(objective3);
            window.draw(objective4);

            // Draw back instruction
            window.draw(instructionsBack);
        }
        else if (currentState == STATE_PLAYING)
        {
            // Draw Gameplay Screen
            window.draw(background);
            window.draw(gameBox);

            // Draw Grid Entities (Meteors, Bullets, Spaceship, Enemies)
            for (int r = 0; r < ROWS; r++)
            {
                for (int c = 0; c < COLS; c++)
                {
                    if (grid[r][c] == 1)
                    { // Spaceship
                        spaceship.setPosition(MARGIN + c * CELL_SIZE, MARGIN + r * CELL_SIZE);
                        // Blink effect during invincibility
                        if (!isInvincible || ((int)(invincibilityTimer.getElapsedTime().asMilliseconds() / 100) % 2 == 0))
                        {
                            window.draw(spaceship);
                        }
                    }
                    else if (grid[r][c] == 2)
                    { // Meteor
                        meteor.setPosition(MARGIN + c * CELL_SIZE, MARGIN + r * CELL_SIZE);
                        window.draw(meteor);
                    }
                    else if (grid[r][c] == 3)
                    { // Player Bullet
                        // Center bullet horizontally
                        float bulletWidth = CELL_SIZE * 0.3f;
                        float xOffset = (CELL_SIZE - bulletWidth) / 2.0f;
                        bullet.setPosition(MARGIN + c * CELL_SIZE + xOffset, MARGIN + r * CELL_SIZE);
                        window.draw(bullet);
                    }
                    else if (grid[r][c] == 4)
                    { // Enemy
                        enemy.setPosition(MARGIN + c * CELL_SIZE, MARGIN + r * CELL_SIZE);
                        window.draw(enemy);
                    }
                    else if (grid[r][c] == 5)
                    { // Boss
                        bossEnemy.setPosition(MARGIN + c * CELL_SIZE, MARGIN + r * CELL_SIZE);
                        window.draw(bossEnemy);
                    }
                    else if (grid[r][c] == 6)
                    { // Boss Bullet
                        // Center bullet horizontally
                        float bulletWidth = CELL_SIZE * 0.3f;
                        float xOffset = (CELL_SIZE - bulletWidth) / 2.0f;
                        bossBullet.setPosition(MARGIN + c * CELL_SIZE + xOffset, MARGIN + r * CELL_SIZE);
                        window.draw(bossBullet);
                    }
                }
            }

            // Draw Active Hit Effects
            for (int i = 0; i < MAX_HIT_EFFECTS; i++)
            {
                if (hitEffectActive[i])
                {
                    bulletHit.setPosition(MARGIN + hitEffectCol[i] * CELL_SIZE, MARGIN + hitEffectRow[i] * CELL_SIZE);
                    window.draw(bulletHit);
                }
            }

            // Draw UI: Lives
            livesText.setString("Lives:");

            // Draw life icons
            float lifeIconStartX = livesText.getPosition().x + livesText.getLocalBounds().width + 10;
            float lifeIconY = livesText.getPosition().y + (livesText.getLocalBounds().height / 2.0f) - 12;

            for (int i = 0; i < lives; i++)
            {
                lifeIcon.setPosition(lifeIconStartX + (i * 28), lifeIconY);
                window.draw(lifeIcon);
            }

            // Draw UI: Score
            char scoreBuffer[20];
            sprintf(scoreBuffer, "Score: %d", score);
            scoreText.setString(scoreBuffer);

            // Draw UI: Level
            char levelBuffer[20];
            sprintf(levelBuffer, "Level: %d", level);
            levelText.setString(levelBuffer);

            // Render UI elements
            window.draw(title);
            window.draw(livesText);
            window.draw(scoreText);
            window.draw(levelText);
        }
        else if (currentState == STATE_LEVEL_UP)
        {
            // Draw Level Up Screen
            window.draw(background);
            window.draw(gameBox);

            // Draw spaceship
            spaceship.setPosition(MARGIN + spaceshipCol * CELL_SIZE, MARGIN + (ROWS - 1) * CELL_SIZE);
            window.draw(spaceship);

            // Draw blinking text
            if (levelUpBlinkState)
            {
                window.draw(levelUpText);
            }

            // Show updated level info
            char levelBuffer[20];
            sprintf(levelBuffer, "Level: %d", level);
            levelText.setString(levelBuffer);
            window.draw(title);
            window.draw(levelText);
        }
        else if (currentState == STATE_PAUSED)
        {
            // Draw Pause Screen (Overlay on top of game)
            window.draw(background);
            window.draw(gameBox);

            // Draw game state in background
            for (int r = 0; r < ROWS; r++)
            {
                for (int c = 0; c < COLS; c++)
                {
                    if (grid[r][c] == 1)
                    { // Spaceship
                        spaceship.setPosition(MARGIN + c * CELL_SIZE, MARGIN + r * CELL_SIZE);
                        window.draw(spaceship);
                    }
                    else if (grid[r][c] == 2)
                    { // Meteor
                        meteor.setPosition(MARGIN + c * CELL_SIZE, MARGIN + r * CELL_SIZE);
                        window.draw(meteor);
                    }
                    else if (grid[r][c] == 3)
                    { // Bullet
                        float bulletWidth = CELL_SIZE * 0.3f;
                        float xOffset = (CELL_SIZE - bulletWidth) / 2.0f;
                        bullet.setPosition(MARGIN + c * CELL_SIZE + xOffset, MARGIN + r * CELL_SIZE);
                        window.draw(bullet);
                    }
                    else if (grid[r][c] == 4)
                    { // Enemy
                        enemy.setPosition(MARGIN + c * CELL_SIZE, MARGIN + r * CELL_SIZE);
                        window.draw(enemy);
                    }
                    else if (grid[r][c] == 5)
                    { // Boss
                        bossEnemy.setPosition(MARGIN + c * CELL_SIZE, MARGIN + r * CELL_SIZE);
                        window.draw(bossEnemy);
                    }
                    else if (grid[r][c] == 6)
                    { // Boss Bullet
                        float bulletWidth = CELL_SIZE * 0.3f;
                        float xOffset = (CELL_SIZE - bulletWidth) / 2.0f;
                        bossBullet.setPosition(MARGIN + c * CELL_SIZE + xOffset, MARGIN + r * CELL_SIZE);
                        window.draw(bossBullet);
                    }
                }
            }

            // Draw semi-transparent overlay
            RectangleShape overlay(Vector2f(COLS * CELL_SIZE, ROWS * CELL_SIZE));
            overlay.setPosition(MARGIN, MARGIN);
            overlay.setFillColor(Color(0, 0, 0, 150)); // Semi-transparent black
            window.draw(overlay);

            // Draw pause menu
            window.draw(pauseTitle);
            for (int i = 0; i < 3; i++)
            {
                if (i == selectedMenuItem)
                {
                    pauseItems[i].setFillColor(Color::Yellow);
                }
                else
                {
                    pauseItems[i].setFillColor(Color::White);
                }
                window.draw(pauseItems[i]);
            }
        }
        else if (currentState == STATE_VICTORY)
        {
            // Draw Victory Screen
            window.draw(menuBackground);
            window.draw(victoryTitle);

            // Draw menu items
            for (int i = 0; i < 2; i++)
            {
                if (i == selectedMenuItem)
                {
                    victoryItems[i].setFillColor(Color::Yellow);
                }
                else
                {
                    victoryItems[i].setFillColor(Color::White);
                }
                window.draw(victoryItems[i]);
            }

            window.draw(victoryInstructions);
        }
        else if (currentState == STATE_GAME_OVER)
        {
            // Draw Game Over Screen
            window.draw(menuBackground);
            window.draw(gameOverTitle);

            // Draw menu items
            for (int i = 0; i < 2; i++)
            {
                if (i == selectedMenuItem)
                {
                    gameOverItems[i].setFillColor(Color::Yellow);
                }
                else
                {
                    gameOverItems[i].setFillColor(Color::White);
                }
                window.draw(gameOverItems[i]);
            }

            window.draw(gameOverInstructions);
        }

        window.display();
        sleep(milliseconds(50)); // Small delay to control game speed
    }

    return 0;
}
