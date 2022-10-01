#include <Arduboy2.h>
// #include "EEPROMUtils.h"
#include "images.h"

#define GROUND_LEVEL                55
#define CH_GROUND_LEVEL             GROUND_LEVEL + 7
#define MAP_SIZE                    20
#define POINTS_ON_MAP               8
#define DEBOUNCE_DELAY              500
#define STEPS_TO_MOVE               100 // chow many button pushes is needed to change position on the map
#define CHB_NAME                    "Boko"


enum class GameStatus : uint8_t {
  Introduction,
  PlayGame,
  WorldMap,
  Event,
  KnockOut,
  MeetMogu,
  GameOver,
};

enum class GroundType : uint8_t {
  FlatA,
};
enum class GroundAnimFrames : uint8_t {
  Frame1,
  Frame2,
  Frame3,
  Frame4,
  Frame5,
};

enum class Stance : uint8_t {
  StandingR,
  StandingL,
  StandingU,
  StandingD,
  WalkingR1,
  WalkingR2,
  WalkingL1,
  WalkingL2,
  WalkingU1,
  WalkingU2,
  WalkingD1,
  WalkingD2,
  Resting,
};

struct Chocobo {
  uint8_t x;
  uint8_t y;
  uint8_t map_x;
  uint8_t map_y;
  uint8_t hp;
  Stance stance;
  const uint8_t *image;
  // const uint8_t *mask;
};

Arduboy2 arduboy;
uint8_t groundX = 0;
int8_t currentFrame = 0;

uint8_t chocobo_max_hp = 7;

GroundType ground[1] = {
  GroundType::FlatA,
};
GroundAnimFrames ground_vert_anim[5] = {
  GroundAnimFrames::Frame1,
  GroundAnimFrames::Frame2,
  GroundAnimFrames::Frame3,
  GroundAnimFrames::Frame4,
  GroundAnimFrames::Frame5,
};

uint16_t score = 0;
uint16_t highScore = 0;

GameStatus gameStatus = GameStatus::Introduction;
Chocobo chocobo = {0, CH_GROUND_LEVEL, 10, 10, chocobo_max_hp, Stance::StandingR, chocobo_still_right };

const uint8_t *chocobo_images[] = { chocobo_still_right, chocobo_still_left, chocobo_still_up, chocobo_still_down, chocobo_walk1_right, chocobo_walk2_right, chocobo_walk1_left, chocobo_walk2_left, chocobo_walk1_up, chocobo_walk2_up, chocobo_walk1_down, chocobo_walk2_down, chocobo_zzz };
const uint8_t *ground_images[] = { ground_flat_a };
const uint8_t *ground_anim[] = { grnd_vert_animation_1, grnd_vert_animation_2, grnd_vert_animation_3, grnd_vert_animation_4, grnd_vert_animation_5 };
bool animateGround = false;
uint8_t grndMovDir = 0;
uint8_t chwmap[MAP_SIZE][MAP_SIZE];
uint8_t steps[4] = {0,0,0,0}; // left, right, up,down
// uint8_t monsters[4] = {1,2,3,4}; // Creeps, Red Bat, Blobra, Wendigo
uint8_t monster = 0; // selected monster
uint8_t cursorx = 30; // x position in hit-bar cursor
bool move_cursor_back = false;
uint8_t monsters_hp[] = {4,4,7,8,12}; /// Creeps, Red Bat, Blobra, Wendigo, Demon King

long walkDebounceTime = 0;
long mapDebounceTime = 0;
bool change_status = false;
bool hit_made = false;
uint8_t count_wins = 0;



void setup() {
  arduboy.boot();
  arduboy.setFrameRate(30);
  arduboy.initRandomSeed();
}

void loop() {
  // Pause here until it's time for the next frame ..
  if (!(arduboy.nextFrame())) return;
  arduboy.pollButtons();
  switch (gameStatus) {
    case GameStatus::Introduction:
      introduction();
      break;
    case GameStatus::PlayGame:
      walkDebounceTime = 0;
      walkDebounceTime = millis();
      playGame();
      break;
    case GameStatus::WorldMap:
      mapDebounceTime = 0;
      mapDebounceTime = millis();
      draw_map();
      break;
    case GameStatus::Event:
      make_event();
      break;
    case GameStatus::KnockOut:
      knock_out();
      break;
    case GameStatus::MeetMogu:
      meet_mogu();
      break;
    case GameStatus::GameOver:
      gameOver();
      break;
  }
}

void draw_sun() {
  arduboy.drawBitmap(121, 0, sun, 8, 8, 1);
}

void clear_map()
{
  int8_t i, j;
  for(i = 0; i < MAP_SIZE; i++) {
    for(j = 0; j < MAP_SIZE; j++)
      {
        chwmap[i][j] = 0;
      }
  }
}

void generate_map() {
  clear_map();
  uint8_t i = 0;
  while (i < POINTS_ON_MAP) {
    uint8_t row = random(0, MAP_SIZE-1);
    uint8_t col = random(0, MAP_SIZE-1);
    if ((chwmap[row][col] == 0) && (row != 10) && (col != 10)) {
      chwmap[row][col] = 1;
      i++;
      }
  }
}

void draw_map() {
  arduboy.clear();
  arduboy.drawRect(0,0,MAP_SIZE,MAP_SIZE);
  arduboy.setCursor(5, 50);
  arduboy.print(F("MAP"));
  arduboy.setCursor(25, 0);
  arduboy.print(F("WINS: "));
  arduboy.print(count_wins);
  arduboy.setCursor(25, 10);
  arduboy.print(F("NAME: "));
  arduboy.print(CHB_NAME);
  // draw locations
  int8_t i, j;
  for(i=0;i<MAP_SIZE;i++) {
    for(j=0;j<MAP_SIZE;j++) {
      if (chwmap[i][j] == 1) arduboy.drawPixel(i,j,1);
      // if (chwmap[i][j] == 1) arduboy.drawRect(i,j,2,2,1);
    }
  }
  arduboy.drawCircle((chocobo.map_x),(chocobo.map_y),1,1);
  arduboy.display();
  arduboy.delayShort (240);
  if ((mapDebounceTime) > DEBOUNCE_DELAY) {
    if (arduboy.pressed(B_BUTTON))  { change_status = true; gameStatus = GameStatus::PlayGame; }
  }
}

void make_step(uint8_t direction) {
  // stes in steps[4] : left, right, up,down
  if (direction == 0) {
    steps[0]++;
    if (chocobo.map_x > 0) { 
      if (steps[0]>=STEPS_TO_MOVE) { chocobo.map_x--; steps[0] = 0; check_event(); arduboy.delayShort (240); }
    }
  }
  if (direction == 1) {
    steps[1]++;
    if (chocobo.map_x < MAP_SIZE-1) {
      if (steps[1]>=STEPS_TO_MOVE) { chocobo.map_x++; steps[1] = 0; check_event(); arduboy.delayShort (240); }
    }
  }
  if (direction == 2) {
    steps[2]++;
    if (chocobo.map_y > 0) {
      if (steps[2]>=STEPS_TO_MOVE) { chocobo.map_y--; steps[2] = 0; check_event(); arduboy.delayShort (240); }
    }
  }
  if (direction == 3) {
    steps[3]++;
    if (chocobo.map_y < MAP_SIZE-1) {
      if (steps[3]>=STEPS_TO_MOVE) { chocobo.map_y++; steps[3] = 0; check_event(); arduboy.delayShort (240); }
    }
  }
}

void check_hit() {
  // checking if hit cursor is in the middle of hit bar:
  // if it is on 30 (hit bar starts) + 32 (half of hit bar lenght)
  // hit_made = false;
  uint8_t k10_throw = 0;
  // if (cursorx == 62) { 
  if ((cursorx > 56) && (cursorx < 68)) { // Chocobo hited the monster
    arduboy.setCursor(25, 15);
    arduboy.println("HIT!");
    monsters_hp[monster-1]--;
    if (monsters_hp[monster-1] > 0) { hit_made = false; }
    //hit_made = true;
    if (monsters_hp[monster-1] <= 0) {
      hit_made = true;
      arduboy.clear();
      arduboy.setCursor(60, 30);
      arduboy.println("WIN!");
      chwmap[chocobo.map_x][chocobo.map_y]=0;
      count_wins++; 
      arduboy.display();
      arduboy.delayShort (1000);
      gameStatus = GameStatus::PlayGame;
      return;
    }
    arduboy.initRandomSeed();
    k10_throw = random(1, 10);
    if (k10_throw == monsters_hp[monster-1]) {monsters_hp[monster-1] = 0; arduboy.clear(); arduboy.setCursor(60, 30); count_wins++; hit_made = true; arduboy.println("WIN!"); chwmap[chocobo.map_x][chocobo.map_y]=0;  arduboy.display(); arduboy.delayShort (500); arduboy.delayShort (500); gameStatus = GameStatus::PlayGame; return;}
    // else { monsters_hp[monster-1]--; hit_made = false; }
    arduboy.display(); 
  }
  else {                        // chocobo has missed
    arduboy.setCursor(25, 15);
    arduboy.println("MISS!");
    hit_made = false;
    arduboy.display();
    arduboy.initRandomSeed();
    k10_throw = random(1, 10);
    arduboy.setCursor(25, 5);
    arduboy.println(k10_throw);
    if (k10_throw > chocobo.hp) { 
      arduboy.setCursor(70, 15); 
      arduboy.println("HIT!");
      arduboy.display(); 
      chocobo.hp--;
      hit_made = false;
      arduboy.delayShort (240); 
    }
    else if (k10_throw == chocobo.hp) {arduboy.clear(); arduboy.setCursor(60, 30); arduboy.println("LOST!!!"); hit_made = true; arduboy.display(); arduboy.delayShort (500); change_status = true; gameStatus = GameStatus::KnockOut; }
    if (chocobo.hp <= 0) {arduboy.clear(); arduboy.setCursor(60, 30); arduboy.println("LOST!"); hit_made = true; arduboy.display(); arduboy.delayShort (500); change_status = true; gameStatus = GameStatus::KnockOut; }
    arduboy.display();
  }
}

void print_hp() {
  arduboy.fillRect(30,52,20,12,0); // erase monster hp
  arduboy.fillRect(84,52,10,12,0); // erase chocobo hp
  arduboy.setCursor(32, 55);
  arduboy.println(monsters_hp[monster-1]);
  if (monster == 5) arduboy.drawRect(30,52,20,12);
  arduboy.drawRect(30,52,10,12); // monster hp frame
  arduboy.setCursor(87, 55);
  arduboy.println(chocobo.hp);
  arduboy.drawRect(84,52,10,12); // chocobo.hp frame
}

void make_event() {
  arduboy.clear();
  // arduboy.delayShort (240);
  // action bar
  arduboy.drawBitmap(70, 22, chocobo_still_left, 24, 24, 1); 
  if (monster == 1) { arduboy.drawBitmap(30, 20, creeps, 22, 22, 1); }
  else if (monster == 2) { arduboy.drawBitmap(30, 22, red_bat, 20, 20, 1); }
  else if (monster == 3) { arduboy.drawBitmap(30, 22, blobra, 24, 24, 1); }
  else if (monster == 4) { arduboy.drawBitmap(30, 22, wendigo, 26, 24, 1); }
  else if (monster == 5) { arduboy.drawBitmap(30, 22, demonking, 22, 22, 1); }
  if (arduboy.everyXFrames(16)) {
    // if (move_cursor_back == false){ cursorx = cursorx +2; if (cursorx >= 90) {move_cursor_back = true;}}
    // else (cursorx >= 90){ cursorx = cursorx -2; }
    while (hit_made == false) {
      while (cursorx < 90 && monsters_hp[monster-1] > 0 && chocobo.hp > 0) {
        arduboy.delayShort (100);
        arduboy.fillRect(cursorx,45,5,5,1);
        arduboy.fillRect(cursorx-2,45,5,5,0); // erase previous rectangle
        arduboy.drawRect(30,45,64,5);
        print_hp();
        arduboy.display();
        cursorx = cursorx +2;
        if (arduboy.pressed(B_BUTTON) && monsters_hp[monster-1] > 0 && chocobo.hp > 0) { hit_made = true; check_hit(); arduboy.delayShort (100);  }
      }
      while (cursorx > 30 && monsters_hp[monster-1] > 0 && chocobo.hp > 0) {
        arduboy.delayShort (100);
        arduboy.fillRect(cursorx,45,5,5,1);
        arduboy.fillRect(cursorx+2,45,5,5,0); // erase previous rectangle
        arduboy.drawRect(30,45,64,5);
        print_hp();
        arduboy.display();
        cursorx = cursorx -2;
        if (arduboy.pressed(B_BUTTON) && monsters_hp[monster-1] > 0 && chocobo.hp > 0) { hit_made = true; check_hit(); arduboy.delayShort (100); }
      }
    }
  }
}

void check_event() {
  monsters_hp[0] = 4;
  monsters_hp[1] = 4;
  monsters_hp[2] = 7;
  monsters_hp[3] = 8;
  monsters_hp[4] = 12;
  // event type: 0 - random, 1 - location
  hit_made = false;
  arduboy.delayShort (240);
  if ((chwmap[chocobo.map_x][chocobo.map_y] == 1) || (count_wins == 8)) {
    arduboy.setCursor(60, 30);
    arduboy.println("EVENT!");
    arduboy.display();
    uint8_t ev_type = random(0, 20);
    switch (ev_type) {
          case 0 ... 10:
            monster = 1; // creeps
            break;
          case 11 ... 15:
            monster = 2; // Red Bat
            break;
          case 16 ... 18:
            monster = 3; // Blobra
            break;
          case 19 ... 20:
          monster = 4; // Wendigo
          break;
    }
    if (count_wins == 8) { monster = 5; }
    arduboy.delayShort (240);
    arduboy.delayShort (240);
    arduboy.delayShort (240);
    arduboy.delayShort (240);
    gameStatus = GameStatus::Event; 
  }
  if (count_wins >= 9) { gameStatus = GameStatus::MeetMogu; }
}

void meet_mogu() {
  arduboy.clear();
  arduboy.drawBitmap(50, 35, mogu, 15, 18, 1);
  arduboy.setCursor(10, 10);
  arduboy.print("Thank You ");
  arduboy.print(CHB_NAME);
  arduboy.display();
  if (arduboy.pressed(A_BUTTON)) {
    gameStatus = GameStatus::Introduction; 
    chocobo.stance = Stance::StandingR;
  }
}

void initialiseGame() {
  score = 0;
  animateGround = false;
  chocobo.x = 50;
  chocobo.y = CH_GROUND_LEVEL-2;
  chocobo.stance = Stance::StandingR;
}

void introduction() {
  arduboy.clear();
  initialiseGame();
  generate_map();
  arduboy.drawBitmap(0, 0, titlescreen, 128, 64, 1);
  arduboy.setCursor(17, 48);
  arduboy.print(F("Press A"));
  arduboy.drawCircle(55, 52, 6, 1);
  arduboy.display();
  if (arduboy.pressed(A_BUTTON)) {
    gameStatus = GameStatus::PlayGame; 
    chocobo.stance = Stance::StandingR;
  }
}
void gameOver() {
  arduboy.clear();
  drawGround(false,0);
  drawChocobo();
  // drawScoreboard(true);
  arduboy.setCursor(40, 12);
  arduboy.print(F("Game Over"));
  // Update Chocobo's image.  If he is dead and still standing, this will change him to lying on the ground ..
  updateChocobo();
  arduboy.display();
  if (arduboy.justPressed(A_BUTTON)) { 
    initialiseGame();
    gameStatus = GameStatus::PlayGame; 
    chocobo.stance = Stance::StandingR;
  }
}

// drawing
void drawChocobo() {

  uint8_t imageIndex = static_cast<uint8_t>(chocobo.stance);

  chocobo.image = chocobo_images[imageIndex];
  // steve.mask = steve_masks[imageIndex];
  // Sprites::drawExternalMask(steve.x, steve.y - getImageHeight(steve.image), steve.image, steve.mask, 0, 0);
  arduboy.drawBitmap(chocobo.x, chocobo.y - 24, chocobo.image, 24, 24, 1);
  
}
void drawGround(bool moveGround, uint8_t direction) {
  // direction:
  //            0 = right
  //            1 = left
  //            2 = up
  //            3 = down
  if (moveGround && direction == 0) {
    if (groundX == 32) groundX = 0;
    groundX++;
  }
  else if (moveGround && direction == 1) {
    if (groundX == 0) groundX = 32;
    groundX--;
  }

  if (moveGround && direction == 2) {
    if (currentFrame == 0) currentFrame = 4;
    currentFrame--;
  }
  if (moveGround && direction == 3) {
    if (currentFrame == 4) currentFrame = 0;
    currentFrame++;
  }
  
  if ((direction == 2) || (direction == 3)) {
  // Render the road.   
  for (uint8_t i = 0; i < 5; i++) {
        uint8_t imageIndex = static_cast<uint8_t>(ground_vert_anim[currentFrame]);
        arduboy.drawBitmap((i * 32) - groundX, GROUND_LEVEL, ground_anim[imageIndex], 32, 8, 1);
      }
}
    
  if ((direction == 0) || (direction == 1)) {
  // Render the road.   
  for (uint8_t i = 0; i < 5; i++) {
    uint8_t imageIndex = static_cast<uint8_t>(ground[0]);
    // Sprites::drawSelfMasked((i * 32) - groundX, GROUND_LEVEL, ground_images[imageIndex], 0); 
    arduboy.drawBitmap((i * 32) - groundX, GROUND_LEVEL, ground_images[imageIndex], 32, 8, 1);  
  }
}
}

void drawScoreboard(bool displayCurrentScore) {
  arduboy.fillRect(0, 0, WIDTH, 10, BLACK);
  if (displayCurrentScore) { 
    arduboy.setCursor(1, 0);
    arduboy.print(F("Score: "));
    arduboy.setCursor(39, 0);
    if (score < 1000) arduboy.print("0");
    if (score < 100) arduboy.print("0");
    if (score < 10)  arduboy.print("0");
    arduboy.print(score);
  }
  arduboy.setCursor(72, 0);
  arduboy.print(F("High: "));
  arduboy.setCursor(104, 0);
  if (highScore < 1000) arduboy.print("0");
  if (highScore < 100) arduboy.print("0");
  if (highScore < 10)  arduboy.print("0");
  arduboy.print(highScore);
  arduboy.drawLine(0, 9, WIDTH, 9, WHITE);
}


// update chocobo sprite

void updateChocobo() {
    // Swap the image to simulate running ..
    if (arduboy.everyXFrames(5)) {

      switch (chocobo.stance) {
        case Stance::StandingR:
          chocobo.stance = Stance::StandingR;
          break;
        case Stance::StandingL:
          chocobo.stance = Stance::StandingL;
          break;
        case Stance::WalkingR1:
          chocobo.stance = Stance::WalkingR2;
          break;
        case Stance::WalkingR2:
          chocobo.stance = Stance::WalkingR1;
          break;
        case Stance::WalkingL1:
          chocobo.stance = Stance::WalkingL2;
          break;
        case Stance::WalkingL2:
          chocobo.stance = Stance::WalkingL1;
          break;
        case Stance::WalkingU1:
          chocobo.stance = Stance::WalkingU2;
          break;
        case Stance::WalkingU2:
          chocobo.stance = Stance::WalkingU1;
          break;
        case Stance::WalkingD1:
          chocobo.stance = Stance::WalkingD2;
          break;
        case Stance::WalkingD2:
          chocobo.stance = Stance::WalkingD1;
          break;
        case Stance::Resting:
          chocobo.stance = Stance::Resting;
          break;
        default:
          break;
      }
    }
}

void knock_out() {
  uint8_t sleep_time = 0;
  arduboy.setCursor(64, 32);
  while (sleep_time<100) {
    arduboy.clear();
    drawGround(0,0);
    arduboy.drawBitmap(chocobo.x, chocobo.y - 24, chocobo_zzz, 24, 24, 1);
    arduboy.print("Zzz");
    arduboy.display();
    arduboy.delayShort (250);
    arduboy.clear();
    drawGround(0,0);
    arduboy.drawBitmap(chocobo.x, chocobo.y - 24, chocobo_zzz, 24, 24, 1);
    arduboy.print("zZz");
    arduboy.display();
    arduboy.delayShort (250);
    arduboy.clear();
    drawGround(0,0);
    arduboy.drawBitmap(chocobo.x, chocobo.y - 24, chocobo_zzz, 24, 24, 1);
    arduboy.print("zzZ");
    arduboy.display();
    arduboy.delayShort (250);
    sleep_time++;
  }
  chocobo.hp = chocobo_max_hp;
  gameStatus = GameStatus::PlayGame;
}


void playGame() {
  arduboy.clear();
  draw_sun();
  if (change_status == true) { arduboy.delayShort (240); change_status = false; }
  // no button press at all and chocobo was walking left
  if (arduboy.notPressed(UP_BUTTON) && arduboy.notPressed(UP_BUTTON) && arduboy.notPressed(RIGHT_BUTTON) && arduboy.notPressed(LEFT_BUTTON) && arduboy.notPressed(A_BUTTON) && arduboy.notPressed(B_BUTTON) && ((chocobo.stance == Stance::WalkingL1) || (chocobo.stance == Stance::WalkingL2))) { chocobo.stance = Stance::StandingL; animateGround = false; }
  // no button press at all and chocobo was walking right
  if (arduboy.notPressed(UP_BUTTON) && arduboy.notPressed(UP_BUTTON) && arduboy.notPressed(RIGHT_BUTTON) && arduboy.notPressed(LEFT_BUTTON) && arduboy.notPressed(A_BUTTON) && arduboy.notPressed(B_BUTTON) && ((chocobo.stance == Stance::WalkingR1) || (chocobo.stance == Stance::WalkingR2))) { chocobo.stance = Stance::StandingR; animateGround = false; }
  // no button press at all and chocobo was walking up
  if (arduboy.notPressed(UP_BUTTON) && arduboy.notPressed(UP_BUTTON) && arduboy.notPressed(RIGHT_BUTTON) && arduboy.notPressed(LEFT_BUTTON) && arduboy.notPressed(A_BUTTON) && arduboy.notPressed(B_BUTTON) && ((chocobo.stance == Stance::WalkingU1) || (chocobo.stance == Stance::WalkingU2))) { chocobo.stance = Stance::StandingU; animateGround = false; }
  // no button press at all and chocobo was walking down
  if (arduboy.notPressed(UP_BUTTON) && arduboy.notPressed(UP_BUTTON) && arduboy.notPressed(RIGHT_BUTTON) && arduboy.notPressed(LEFT_BUTTON) && arduboy.notPressed(A_BUTTON) && arduboy.notPressed(B_BUTTON) && ((chocobo.stance == Stance::WalkingD1) || (chocobo.stance == Stance::WalkingD2))) { chocobo.stance = Stance::StandingD; animateGround = false; }
  
  // ground moving direction:
  //            0 = right
  //            1 = left
  //            2 = up
  //            3 = down
  
  // steps in steps[4] : left, right, up,down
  if ((walkDebounceTime) > DEBOUNCE_DELAY) {
    if (arduboy.pressed(RIGHT_BUTTON)) { chocobo.stance = Stance::WalkingR1; make_step(1); animateGround = true; grndMovDir = 0; }
    if (arduboy.pressed(LEFT_BUTTON))  { chocobo.stance = Stance::WalkingL1; make_step(0); animateGround = true; grndMovDir = 1; }
    if (arduboy.pressed(UP_BUTTON))    { chocobo.stance = Stance::WalkingU1; make_step(2); animateGround = true; grndMovDir = 2; }
    if (arduboy.pressed(DOWN_BUTTON))  { chocobo.stance = Stance::WalkingD1; make_step(3); animateGround = true; grndMovDir = 3; }
  }

  if (arduboy.pressed(B_BUTTON))  { chocobo.stance = Stance::StandingR; animateGround = false; grndMovDir = 3; gameStatus = GameStatus::WorldMap; }
  
  drawGround(animateGround,grndMovDir);
  updateChocobo();
  drawChocobo();
  // drawScoreboard(true);
  arduboy.display();
}
