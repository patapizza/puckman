/*
 * Puck-Man game
 * Copyright (C) 2009 Julien Odent <julien at odent dot net>
 * Images Copyright (C) 2009 Martin Meys <martin dot meys at gmail dot com>
 *
 * This game is an unofficial clone of the original
 * Pac-Man game and is not endorsed by the
 * registered trademark owners Namco, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_gfxPrimitives.h>

#define PACPATH "/usr/share/puckman/"
#define SCREEN_WIDTH 461
#define SCREEN_HEIGHT 580
#define RIGHT 0
#define LEFT 1
#define UP 2
#define DOWN 3
#define MAZE_R 0
#define MAZE_G 0
#define MAZE_B 178
#define BLINKY 0
#define PINKY 1
#define INKY 2
#define CLYDE 3

typedef struct Ghost {
  int x, y, dir, image_index, id, state, ways[4], initloop, lowspeed;
  SDL_Surface *image[4][2], *scared[2], *scared2[8], *eyes[4];
  struct Ghost *next;
} Ghost;
typedef struct Pacman {
  int x, y, dir, nextDir, image_index, stuck;
  SDL_Surface *image[4][4], *dead[12];
} Pacman;
typedef struct ImageData {
  SDL_Surface *candy[6], *digits[10], *life, *levels[4], *dot;
  SDL_Surface *level, *getready, *gameover, *paused, *lives, *score;
  SDL_Surface *bonus100, *bonus200, *bonus300, *bonus400, *bonus500, *bonus700, *bonus800, *bonus1600;
  SDL_Surface *logo, *anim[39], *legal, *notice, *playgame[2], *highscores[2], *rules[2], *quitgame[2], *back, *rules_main, *enter;
} ImageData;
typedef struct GameData {
  int running, speed, delay, walls[53][46], score, candy_index, candy_blow_delay, lives, level, state, bonus, newlife, paused, fruit, anim_index, selected, newscorer_index;
  SDL_Surface *screen;
  clock_t ticks, ticks_fruit;
  ImageData *img;
  char highscores[2][11][20];
  Pacman *pacman;
  Ghost *ghosts;
  char* scores_file;
} GameData;

void Blinky_chase(Ghost *ghost);
int cleanUp(int err);
void Clyde_chase(Ghost *ghost);
void drawBonus(int x, int y);
void drawBottom();
void drawCandies();
void drawCandy(int x, int y);
void drawDigit(int digit, int x, int y);
void drawFruit();
void drawHighscores();
void drawMain();
void drawMaze();
void drawNewscorer();
void drawNumber(int score, int x, int y);
void drawPower(int x, int y);
void drawRules();
void eraseScreen();
void Game_init();
void Game_new();
void Game_process();
SDL_Surface *getImage(char *str);
SDL_Surface *getLetter(int letter);
void Ghost_chase(Ghost *ghost);
void Ghost_checkWay(Ghost *ghost);
void Ghost_draw();
void Ghost_goHome(Ghost *ghost);
void Ghost_move();
int Ghost_moveDown(Ghost *ghost);
int Ghost_moveLeft(Ghost *ghost);
int Ghost_moveRight(Ghost *ghost);
int Ghost_moveUp(Ghost *ghost);
void Ghost_init();
void Ghost_load(Ghost *ghost);
void Ghost_scare(Ghost *ghost, int s);
void Ghost_unscare();
void Image_init();
void Inky_chase(Ghost *ghost);
void Pacman_checkDir();
void Pacman_draw();
void Pacman_free();
void Pacman_init();
void Pacman_load();
void Pacman_move();
void Pinky_chase(Ghost *ghost);
void raiseWalls();
void sort();
void swap(int i);
int toInt(char score[20]);
void writeScores();

GameData *game;

int main (int argc, char **argv) {
  game = malloc(sizeof(struct GameData));
  ImageData *image = malloc(sizeof(struct ImageData));
  Pacman *pacman = malloc(sizeof(struct Pacman));
  Ghost *blinky = malloc(sizeof(struct Ghost));
  Ghost *pinky = malloc(sizeof(struct Ghost));
  Ghost *inky = malloc(sizeof(struct Ghost));
  Ghost *clyde = malloc(sizeof(struct Ghost));
  game->img = image;
  game->pacman = pacman;
  game->ghosts = blinky;
  blinky->next = pinky;
  pinky->next = inky;
  inky->next = clyde;
  clyde->next = blinky;
  Game_init();
  Image_init();
  Pacman_init();
  Ghost_init();
  SDL_Event event;
  Uint8 *keystate;
  if (SDL_Init(SDL_INIT_VIDEO != 0)) {
    fprintf(stderr, "Could not initialise SDL: %s\n", SDL_GetError());
    return 1;
  }
  if ((game->screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 8, SDL_SWSURFACE)) == NULL) {
    fprintf(stderr, "Could not set SDL video mode: %s\n", SDL_GetError());
    return cleanUp(1);
  }
  SDL_Rect dest = { 170, 290, 0, 0 };
  SDL_WM_SetCaption("Puck-Man", "Puck-Man");
  SDL_ShowCursor(SDL_DISABLE);
  while (1) {
    eraseScreen();
    if (game->state == 5) {
      --(game->anim_index);
      if (game->anim_index == -1)
        game->anim_index = 38;
      drawMain();
    }
    else if (game->state == 6) {
      ++(game->candy_index);
      if (game->candy_index == 6)
        game->candy_index = 0;
      drawRules();
    }
    else if (game->state == 7)
      drawHighscores(game);
    else if (game->state == 8)
      drawNewscorer(game);
    else {
      Game_process();
      if (game->state != 1) {
        game->fruit = 0;
        game->ticks_fruit = clock();
      }
      if (game->state == 0) {
        ++(game->level);
        raiseWalls();
        Pacman_init();
        Ghost_init();
        SDL_BlitSurface(game->img->level, NULL, game->screen, &dest);
        drawNumber(game->level, 260, 290);
      }
      else if (game->state == 4) {
        --(game->lives);
        Pacman_init();
        Ghost_init();
        if (game->lives < 1)
          SDL_BlitSurface(game->img->gameover, NULL, game->screen, &dest);
        else
          SDL_BlitSurface(game->img->getready, NULL, game->screen, &dest);
	if (game->lives == -2) {
	  game->delay = 100;
	  game->state = 8;
	}
      }
      if (game->state == 1 && ((double) clock() - game->ticks_fruit) / CLOCKS_PER_SEC >= 0.5) {
        game->fruit ^= 1;
        game->ticks_fruit = clock();
      }
      if (game->fruit == 1)
        drawFruit();
      drawBottom();
      drawMaze();
      drawCandies();
      Pacman_draw();
      if (game->state != 2)
        Ghost_draw();
      if (game->paused == 1)
        SDL_BlitSurface(game->img->paused, NULL, game->screen, &dest);
      if (game->state == 3) {
        game->score += game->bonus;
        drawBonus(game->pacman->x, game->pacman->y - 10);
        game->bonus = 0;
      }
    }
    SDL_UpdateRect(game->screen, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    while (SDL_PollEvent(&event))
      if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
        game->running = 0;
      else if (game->state == 1 && event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_p)
        game->paused ^= 1;
      else if (game->state == 5 && event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_UP)
	  game->selected = (game->selected == 0) ? 0 : game->selected - 1;
	else if (event.key.keysym.sym == SDLK_DOWN)
	  game->selected = (game->selected == 3) ? 3 : game->selected + 1;
	else if (event.key.keysym.sym == SDLK_RETURN)
	  switch (game->selected) {
	    case 0:
	      Game_new();
	      break;
	    case 1:
	      game->state = 7;
	      break;
	    case 2:
	      game->state = 6;
	      game->delay = 42;
	      break;
	    case 3:
	      game->running = 0;
	  }
      }
      else if ((game->state == 6 || game->state == 7) && event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RETURN) {
        game->state = 5;
	game->delay = 100;
      }
      else if (game->state == 8 && event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_RETURN) {
	  if (game->newscorer_index > 1) {
	    snprintf(game->highscores[1][10], 20, "%d                    ", game->score);
	    sort();
	    game->state = 5;
	  }
	}
	else if (event.key.keysym.sym >= SDLK_a && event.key.keysym.sym <= SDLK_z) {
	  if (game->newscorer_index < 19)
            game->highscores[0][10][game->newscorer_index] = event.key.keysym.sym - 32;
	  if (game->newscorer_index < 19)
	    ++(game->newscorer_index);
	}
	else if (event.key.keysym.sym == SDLK_BACKSPACE && game->newscorer_index > 0) {
	  game->highscores[0][10][game->newscorer_index - 1] = ' ';
	  --(game->newscorer_index);
	}
      }
    if (game->running == 0)
      break;
    if (game->state < 5) {
      keystate = SDL_GetKeyState(NULL);
      if (keystate[SDLK_RIGHT])
        game->pacman->nextDir = RIGHT;
      else if (keystate[SDLK_LEFT])
        game->pacman->nextDir = LEFT;
      else if (keystate[SDLK_UP])
        game->pacman->nextDir = UP;
      else if (keystate[SDLK_DOWN])
        game->pacman->nextDir = DOWN;
    }
    if (game->paused == 1)
      ;
    else if (game->state == 0) {
      SDL_Delay(1000);
      game->state = 1;
    }
    else if (game->state == 1) {
      int i = 0;
      while (i < game->speed) {
        Pacman_move();
        Ghost_move();
        ++i;
      }
    }
    else if (game->state == 2) {
      if (((double) clock() - game->ticks) / CLOCKS_PER_SEC >= 0.03)
	game->state = 4;
    }
    else if (game->state == 3) {
      SDL_Delay(300);
      game->state = 1;
    }
    else if (game->state == 4) {
      SDL_Delay(1000);
      if (game->lives > 0)
        game->state = 1;
    }
    else if (game->state == 9)
      game->state = 0;
    SDL_Delay(game->delay);
  }
  free(game);
  free(image);
  free(pacman);
  free(blinky);
  free(pinky);
  free(inky);
  free(clyde);
  return cleanUp(0);
}
void Blinky_chase(Ghost *ghost) {
  int x = game->pacman->x;
  int y = game->pacman->y;
  if (ghost->state == 1 || ghost->state == 2) {
    x = 461 - x;
    y = 580 - y;
  }
  if (x == ghost->x && y > ghost->y && ghost->ways[DOWN] == 1 && Ghost_moveDown(ghost) == 1)
    ghost->dir = DOWN;
  else if (y == ghost->y && x < ghost->x && ghost->ways[LEFT] == 1 && Ghost_moveLeft(ghost) == 1)
    ghost->dir = LEFT;
  else if (x == ghost->x && y < ghost->y && ghost->ways[UP] == 1 && Ghost_moveUp(ghost) == 1)
    ghost->dir = UP;
  else if (y == ghost->y && x > ghost->x && ghost->ways[RIGHT] == 1 && Ghost_moveRight(ghost) == 1)
    ghost->dir = RIGHT;
  else if (x > ghost->x && ghost->ways[RIGHT] == 1 && Ghost_moveRight(ghost) == 1)
    ghost->dir = RIGHT;
  else if (y > ghost->y && ghost->ways[DOWN] == 1 && Ghost_moveDown(ghost) == 1)
    ghost->dir = DOWN;
  else if (x < ghost->x && ghost->ways[LEFT] == 1 && Ghost_moveLeft(ghost) == 1)
    ghost->dir = LEFT;
  else if (y < ghost->y && ghost->ways[UP] == 1 && Ghost_moveUp(ghost) == 1)
    ghost->dir = UP;
  else if (ghost->dir == LEFT && Ghost_moveLeft(ghost) == 0) {
    if (y >= ghost->y && ghost->ways[DOWN] == 1 && Ghost_moveDown(ghost) == 1)
      ghost->dir = DOWN;
    else if (ghost->ways[UP] == 1 && Ghost_moveUp(ghost) == 1)
      ghost->dir = UP;
    else {
      ghost->dir = DOWN;
      Ghost_moveDown(ghost);
    }
  }
  else if (ghost->dir == DOWN && Ghost_moveDown(ghost) == 0) {
    if (x >= ghost->x && ghost->ways[RIGHT] == 1 && Ghost_moveRight(ghost) == 1)
      ghost->dir = RIGHT;
    else if (ghost->ways[LEFT] == 1 && Ghost_moveLeft(ghost) == 1)
      ghost->dir = LEFT;
    else {
      ghost->dir = RIGHT;
      Ghost_moveRight(ghost);
    }
  }
  else if (ghost->dir == RIGHT && Ghost_moveRight(ghost) == 0) {
    if (y >= ghost->y && ghost->ways[DOWN] && Ghost_moveDown(ghost) == 1)
      ghost->dir = DOWN;
    else if (ghost->ways[UP] == 1 && Ghost_moveUp(ghost) == 1)
      ghost->dir = UP;
    else {
      ghost->dir = DOWN;
      Ghost_moveDown(ghost);
    }
  }
  else if (ghost->dir == UP && Ghost_moveUp(ghost) == 0) {
    if (x >= ghost->x && ghost->ways[RIGHT] == 1 && Ghost_moveRight(ghost) == 1)
      ghost->dir = RIGHT;
    else if (ghost->ways[LEFT] == 1 && Ghost_moveLeft(ghost) == 1)
      ghost->dir = LEFT;
    else {
      ghost->dir = RIGHT;
      Ghost_moveRight(ghost);
    }
  }
}
int cleanUp(int err) {
  SDL_Quit();
  return err;
}
void Clyde_chase(Ghost *ghost) {
  int x = game->pacman->x;
  int y = game->pacman->y;
  if (ghost->state == 1 || ghost->state == 2) {
    x = 461 - x;
    y = 580 - y;
  }
  if (ghost->dir == LEFT && Ghost_moveLeft(ghost) == 0) {
    if (y >= ghost->y && ghost->ways[DOWN] == 1 && Ghost_moveDown(ghost) == 1)
      ghost->dir = DOWN;
    else if (ghost->ways[UP] == 1 && Ghost_moveUp(ghost) == 1)
      ghost->dir = UP;
    else {
      ghost->dir = DOWN;
      Ghost_moveDown(ghost);
    }
  }
  else if (ghost->dir == DOWN && Ghost_moveDown(ghost) == 0) {
    if (x >= ghost->x && ghost->ways[RIGHT] == 1 && Ghost_moveRight(ghost) == 1)
      ghost->dir = RIGHT;
    else if (ghost->ways[LEFT] == 1 && Ghost_moveLeft(ghost) == 1)
      ghost->dir = LEFT;
    else {
      ghost->dir = RIGHT;
      Ghost_moveRight(ghost);
    }
  }
  else if (ghost->dir == RIGHT && Ghost_moveRight(ghost) == 0) {
    if (y >= ghost->y && ghost->ways[DOWN] == 1 && Ghost_moveDown(ghost) == 1)
      ghost->dir = DOWN;
    else if (ghost->ways[UP] == 1 && Ghost_moveUp(ghost) == 1)
      ghost->dir = UP;
    else {
      ghost->dir = DOWN;
      Ghost_moveDown(ghost);
    }
  }
  else if (ghost->dir == UP && Ghost_moveUp(ghost) == 0) {
    if (x >= ghost->x && ghost->ways[RIGHT] == 1 && Ghost_moveRight(ghost) == 1)
      ghost->dir = RIGHT;
    else if (ghost->ways[LEFT] == 1 && Ghost_moveLeft(ghost) == 1)
      ghost->dir = LEFT;
    else {
      ghost->dir = RIGHT;
      Ghost_moveRight(ghost);
    }
  }
}
void drawBonus(int x, int y) {
  SDL_Surface *img;
  switch (game->bonus) {
    case 100:
      img = game->img->bonus100;
      break;
    case 200:
      img = game->img->bonus200;
      break;
    case 300:
      img = game->img->bonus300;
      break;
    case 400:
      img = game->img->bonus400;
      break;
    case 500:
      img = game->img->bonus500;
      break;
    case 700:
      img = game->img->bonus700;
      break;
    case 800:
      img = game->img->bonus800;
      break;
    case 1600:
      img = game->img->bonus1600;
  }
  SDL_Rect dest = { x, y, 0, 0 };
  SDL_BlitSurface(img, NULL, game->screen, &dest);
}
void drawBottom() {
  SDL_Rect dest = { 5, 542, 0, 0 };
  SDL_BlitSurface(game->img->score, NULL, game->screen, &dest);
  dest.x = 200;
  dest.y = 542;
  SDL_BlitSurface(game->img->lives, NULL, game->screen, &dest);
  if (game->lives > 1) {
    dest.x = 300;
    dest.y = 543;
    SDL_BlitSurface(game->img->life, NULL, game->screen, &dest);
    if (game->lives > 2) {
      dest.x = 330;
      SDL_BlitSurface(game->img->life, NULL, game->screen, &dest);
    }
    if (game->lives > 3) {
      dest.x = 360;
      SDL_BlitSurface(game->img->life, NULL, game->screen, &dest);
    }
  }
  dest.x = 420;
  dest.y = 543;
  int level = (game->level > 4) ? 3 : game->level - 1;
  SDL_BlitSurface(game->img->levels[level], NULL, game->screen, &dest);
  drawNumber(game->score, 180, 542);
}
void drawCandies() {
  int i = 25;
  if (game->walls[2][2] == 1)
    drawCandy(25, i);
  if (game->walls[2][4] == 1)
    drawCandy(40, i);
  if (game->walls[2][5] == 1)
    drawCandy(55, i);
  if (game->walls[2][7] == 1)
    drawCandy(70, i);
  if (game->walls[2][8] == 1)
    drawCandy(89, i);
  if (game->walls[2][10] == 1)
    drawCandy(105, i);
  if (game->walls[2][12] == 1)
    drawCandy(120, i);
  if (game->walls[2][13] == 1)
    drawCandy(139, i);
  if (game->walls[2][15] == 1)
    drawCandy(155, i);
  if (game->walls[2][17] == 1)
    drawCandy(170, i);
  if (game->walls[2][18] == 1)
    drawCandy(189, i);
  if (game->walls[2][20] == 1)
    drawCandy(205, i);
  if (game->walls[2][25] == 1)
    drawCandy(255, i);
  if (game->walls[2][27] == 1)
    drawCandy(270, i);
  if (game->walls[2][28] == 1)
    drawCandy(289, i);
  if (game->walls[2][30] == 1)
    drawCandy(305, i);
  if (game->walls[2][31] == 1)
    drawCandy(319, i);
  if (game->walls[2][33] == 1)
    drawCandy(339, i);
  if (game->walls[2][35] == 1)
    drawCandy(355, i);
  if (game->walls[2][37] == 1)
    drawCandy(371, i);
  if (game->walls[2][38] == 1)
    drawCandy(389, i);
  if (game->walls[2][40] == 1)
    drawCandy(405, i);
  if (game->walls[2][41] == 1)
    drawCandy(419, i);
  if (game->walls[2][43] == 1)
    drawCandy(434, i);
  int j = 4;
  i += 15;
  while (i < 82) {
    if (j == 4) {
      if (game->walls[j][2] == 1)
        drawPower(20, 47);
      if (game->walls[j][43] == 1)
        drawPower(428, 47);
    }
    if (game->walls[j][2] == 1)
      drawCandy(25, i);
    if (game->walls[j][10] == 1)
      drawCandy(105, i);
    if (game->walls[j][20] == 1)
      drawCandy(205, i);
    if (game->walls[j][25] == 1)
      drawCandy(255, i);
    if (game->walls[j][35] == 1)
      drawCandy(355, i);
    if (game->walls[j][43] == 1)
      drawCandy(434, i);
    ++j;
    i += 14;
  }
  i += 3;
  if (game->walls[8][2] == 1)
    drawCandy(25, i);
  if (game->walls[8][4] == 1)
    drawCandy(40, i);
  if (game->walls[8][5] == 1)
    drawCandy(55, i);
  if (game->walls[8][7] == 1)
    drawCandy(70, i);
  if (game->walls[8][8] == 1)
    drawCandy(89, i);
  if (game->walls[8][10] == 1)
    drawCandy(105, i);
  if (game->walls[8][12] == 1)
    drawCandy(120, i);
  if (game->walls[8][13] == 1)
    drawCandy(139, i);
  if (game->walls[8][15] == 1)
    drawCandy(155, i);
  if (game->walls[8][17] == 1)
    drawCandy(170, i);
  if (game->walls[8][18] == 1)
    drawCandy(189, i);
  if (game->walls[8][20] == 1)
    drawCandy(205, i);
  if (game->walls[8][25] == 1)
    drawCandy(255, i);
  if (game->walls[8][27] == 1)
    drawCandy(270, i);
  if (game->walls[8][28] == 1)
    drawCandy(289, i);
  if (game->walls[8][30] == 1)
    drawCandy(305, i);
  if (game->walls[8][31] == 1)
    drawCandy(319, i);
  if (game->walls[8][33] == 1)
    drawCandy(339, i);
  if (game->walls[8][35] == 1)
    drawCandy(355, i);
  if (game->walls[8][37] == 1)
    drawCandy(371, i);
  if (game->walls[8][38] == 1)
    drawCandy(389, i);
  if (game->walls[8][40] == 1)
    drawCandy(405, i);
  if (game->walls[8][41] == 1)
    drawCandy(419, i);
  if (game->walls[8][43] == 1)
    drawCandy(434, i);
  if (game->walls[8][22] == 1)
    drawCandy(220, i);
  if (game->walls[8][23] == 1)
    drawCandy(239, i);
  j = 10;
  i += 15;
  while (i < 120) {
    if (game->walls[j][2] == 1)
      drawCandy(25, i);
    if (game->walls[j][10] == 1)
      drawCandy(105, i);
    if (game->walls[j][15] == 1)
      drawCandy(155, i);
    if (game->walls[j][30] == 1)
      drawCandy(305, i);
    if (game->walls[j][35] == 1)
      drawCandy(355, i);
    if (game->walls[j][43] == 1)
      drawCandy(434, i);
    ++j;
    i += 19;
  }
  i -= 3;
  if (game->walls[13][2] == 1)
    drawCandy(25, i);
  if (game->walls[13][4] == 1)
    drawCandy(40, i);
  if (game->walls[13][5] == 1)
    drawCandy(55, i);
  if (game->walls[13][7] == 1)
    drawCandy(70, i);
  if (game->walls[13][8] == 1)
    drawCandy(89, i);
  if (game->walls[13][10] == 1)
    drawCandy(105, i);
  if (game->walls[13][15] == 1)
    drawCandy(155, i);
  if (game->walls[13][17] == 1)
    drawCandy(170, i);
  if (game->walls[13][18] == 1)
    drawCandy(189, i);
  if (game->walls[13][20] == 1)
    drawCandy(205, i);
  if (game->walls[13][25] == 1)
    drawCandy(255, i);
  if (game->walls[13][27] == 1)
    drawCandy(270, i);
  if (game->walls[13][28] == 1)
    drawCandy(289, i);
  if (game->walls[13][30] == 1)
    drawCandy(305, i);
  if (game->walls[13][35] == 1)
    drawCandy(355, i);
  if (game->walls[13][37] == 1)
    drawCandy(371, i);
  if (game->walls[13][38] == 1)
    drawCandy(389, i);
  if (game->walls[13][40] == 1)
    drawCandy(405, i);
  if (game->walls[13][41] == 1)
    drawCandy(419, i);
  if (game->walls[13][43] == 1)
    drawCandy(434, i);
  i += 15;
  if (game->walls[15][10] == 1)
    drawCandy(105, i);
  if (game->walls[15][35] == 1)
    drawCandy(355, i);
  i += 19;
  if (game->walls[16][10] == 1)
    drawCandy(105, i);
  if (game->walls[16][35] == 1)
    drawCandy(355, i);
  i += 15;
  if (game->walls[18][10] == 1)
    drawCandy(105, i);
  if (game->walls[18][35] == 1)
    drawCandy(355, i);
  i += 16;
  if (game->walls[20][10] == 1)
    drawCandy(105, i);
  if (game->walls[20][35] == 1)
    drawCandy(355, i);
  i += 15;
  if (game->walls[21][10] == 1)
    drawCandy(105, i);
  if (game->walls[21][35] == 1)
    drawCandy(355, i);
  i += 14;
  if (game->walls[22][10] == 1)
    drawCandy(105, i);
  if (game->walls[22][35] == 1)
    drawCandy(355, i);
  i += 16;
  if (game->walls[24][10] == 1)
    drawCandy(105, i);
  if (game->walls[24][35] == 1)
    drawCandy(355, i);
  i += 15;
  if (game->walls[26][10] == 1)
    drawCandy(105, i);
  if (game->walls[26][35] == 1)
    drawCandy(355, i);
  i += 15;
  if (game->walls[27][10] == 1)
    drawCandy(105, i);
  if (game->walls[27][35] == 1)
    drawCandy(355, i);
  i += 14;
  if (game->walls[28][10] == 1)
    drawCandy(105, i);
  if (game->walls[28][35] == 1)
    drawCandy(355, i);
  i += 16;
  if (game->walls[30][10] == 1)
    drawCandy(105, i);
  if (game->walls[30][35] == 1)
    drawCandy(355, i);
  i += 15;
  if (game->walls[32][10] == 1)
    drawCandy(105, i);
  if (game->walls[32][35] == 1)
    drawCandy(355, i);
  i += 19;
  if (game->walls[33][10] == 1)
    drawCandy(105, i);
  if (game->walls[33][35] == 1)
    drawCandy(355, i);
  i += 15;
  if (game->walls[35][2] == 1)
    drawCandy(25, i);
  if (game->walls[35][4] == 1)
    drawCandy(40, i);
  if (game->walls[35][5] == 1)
    drawCandy(55, i);
  if (game->walls[35][7] == 1)
    drawCandy(70, i);
  if (game->walls[35][8] == 1)
    drawCandy(89, i);
  if (game->walls[35][10] == 1)
    drawCandy(105, i);
  if (game->walls[35][12] == 1)
    drawCandy(120, i);
  if (game->walls[35][13] == 1)
    drawCandy(139, i);
  if (game->walls[35][15] == 1)
    drawCandy(155, i);
  if (game->walls[35][17] == 1)
    drawCandy(170, i);
  if (game->walls[35][18] == 1)
    drawCandy(189, i);
  if (game->walls[35][20] == 1)
    drawCandy(205, i);
  if (game->walls[35][25] == 1)
    drawCandy(255, i);
  if (game->walls[35][27] == 1)
    drawCandy(270, i);
  if (game->walls[35][28] == 1)
    drawCandy(289, i);
  if (game->walls[35][30] == 1)
    drawCandy(305, i);
  if (game->walls[35][31] == 1)
    drawCandy(319, i);
  if (game->walls[35][33] == 1)
    drawCandy(339, i);
  if (game->walls[35][35] == 1)
    drawCandy(355, i);
  if (game->walls[35][37] == 1)
    drawCandy(371, i);
  if (game->walls[35][38] == 1)
    drawCandy(389, i);
  if (game->walls[35][40] == 1)
    drawCandy(405, i);
  if (game->walls[35][41] == 1)
    drawCandy(419, i);
  if (game->walls[35][43] == 1)
    drawCandy(434, i);
  j = 37;
  i += 16;
  while (i < 390) {
    if (game->walls[j][2] == 1)
      drawCandy(25, i);
    if (game->walls[j][10] == 1)
      drawCandy(105, i);
    if (game->walls[j][20] == 1)
      drawCandy(205, i);
    if (game->walls[j][25] == 1)
      drawCandy(255, i);
    if (game->walls[j][35] == 1)
      drawCandy(355, i);
    if (game->walls[j][43] == 1)
      drawCandy(434, i);
    ++j;
    i += 19;
  }
  i -= 3;
  if (game->walls[40][2] == 1)
    drawPower(20, 400);
  if (game->walls[40][4] == 1)
    drawCandy(40, i);
  if (game->walls[40][5] == 1)
    drawCandy(55, i);
  if (game->walls[40][10] == 1)
    drawCandy(105, i);
  if (game->walls[40][12] == 1)
    drawCandy(120, i);
  if (game->walls[40][13] == 1)
    drawCandy(139, i);
  if (game->walls[40][15] == 1)
    drawCandy(155, i);
  if (game->walls[40][17] == 1)
    drawCandy(170, i);
  if (game->walls[40][18] == 1)
    drawCandy(189, i);
  if (game->walls[40][20] == 1)
    drawCandy(205, i);
  if (game->walls[40][25] == 1)
    drawCandy(255, i);
  if (game->walls[40][27] == 1)
    drawCandy(270, i);
  if (game->walls[40][28] == 1)
    drawCandy(289, i);
  if (game->walls[40][30] == 1)
    drawCandy(305, i);
  if (game->walls[40][31] == 1)
    drawCandy(319, i);
  if (game->walls[40][33] == 1)
    drawCandy(339, i);
  if (game->walls[40][35] == 1)
    drawCandy(355, i);
  if (game->walls[40][40] == 1)
    drawCandy(405, i);
  if (game->walls[40][41] == 1)
    drawCandy(419, i);
  if (game->walls[40][43] == 1)
    drawPower(428, 400);
  j = 42;
  i += 16;
  while (i < 440) {
    if (game->walls[j][5] == 1)
      drawCandy(55, i);
    if (game->walls[j][10] == 1)
      drawCandy(105, i);
    if (game->walls[j][15] == 1)
      drawCandy(155, i);
    if (game->walls[j][30] == 1)
      drawCandy(305, i);
    if (game->walls[j][35] == 1)
      drawCandy(355, i);
    if (game->walls[j][40] == 1)
      drawCandy(405, i);
    ++j;
    i += 18;
  }
  i -= 3;
  if (game->walls[45][2] == 1)
    drawCandy(25, i);
  if (game->walls[45][4] == 1)
    drawCandy(40, i);
  if (game->walls[45][5] == 1)
    drawCandy(55, i);
  if (game->walls[45][7] == 1)
    drawCandy(70, i);
  if (game->walls[45][8] == 1)
    drawCandy(89, i);
  if (game->walls[45][10] == 1)
    drawCandy(105, i);
  if (game->walls[45][15] == 1)
    drawCandy(155, i);
  if (game->walls[45][17] == 1)
    drawCandy(170, i);
  if (game->walls[45][18] == 1)
    drawCandy(189, i);
  if (game->walls[45][20] == 1)
    drawCandy(205, i);
  if (game->walls[45][25] == 1)
    drawCandy(255, i);
  if (game->walls[45][27] == 1)
    drawCandy(270, i);
  if (game->walls[45][28] == 1)
    drawCandy(289, i);
  if (game->walls[45][30] == 1)
    drawCandy(305, i);
  if (game->walls[45][35] == 1)
    drawCandy(355, i);
  if (game->walls[45][37] == 1)
    drawCandy(371, i);
  if (game->walls[45][38] == 1)
    drawCandy(389, i);
  if (game->walls[45][40] == 1)
    drawCandy(405, i);
  if (game->walls[45][41] == 1)
    drawCandy(419, i);
  if (game->walls[45][43] == 1)
    drawCandy(434, i);
  j = 47;
  i += 16;
  while (i < 490) {
    if (game->walls[j][2] == 1)
      drawCandy(25, i);
    if (game->walls[j][20] == 1)
      drawCandy(205, i);
    if (game->walls[j][25] == 1)
      drawCandy(255, i);
    if (game->walls[j][43] == 1)
      drawCandy(434, i);
    ++j;
    i += 19;
  }
  i -= 3;
  if (game->walls[50][2] == 1)
    drawCandy(25, i);
  if (game->walls[50][4] == 1)
    drawCandy(40, i);
  if (game->walls[50][5] == 1)
    drawCandy(55, i);
  if (game->walls[50][7] == 1)
    drawCandy(70, i);
  if (game->walls[50][8] == 1)
    drawCandy(89, i);
  if (game->walls[50][10] == 1)
    drawCandy(105, i);
  if (game->walls[50][12] == 1)
    drawCandy(120, i);
  if (game->walls[50][13] == 1)
    drawCandy(139, i);
  if (game->walls[50][15] == 1)
    drawCandy(155, i);
  if (game->walls[50][17] == 1)
    drawCandy(170, i);
  if (game->walls[50][18] == 1)
    drawCandy(189, i);
  if (game->walls[50][20] == 1)
    drawCandy(205, i);
  if (game->walls[50][25] == 1)
    drawCandy(255, i);
  if (game->walls[50][27] == 1)
    drawCandy(270, i);
  if (game->walls[50][28] == 1)
    drawCandy(289, i);
  if (game->walls[50][30] == 1)
    drawCandy(305, i);
  if (game->walls[50][31] == 1)
    drawCandy(319, i);
  if (game->walls[50][33] == 1)
    drawCandy(339, i);
  if (game->walls[50][35] == 1)
    drawCandy(355, i);
  if (game->walls[50][37] == 1)
    drawCandy(371, i);
  if (game->walls[50][38] == 1)
    drawCandy(389, i);
  if (game->walls[50][40] == 1)
    drawCandy(405, i);
  if (game->walls[50][41] == 1)
    drawCandy(419, i);
  if (game->walls[50][43] == 1)
    drawCandy(434, i);
  if (game->walls[50][22] == 1)
    drawCandy(220, i);
  if (game->walls[50][23] == 1)
    drawCandy(239, i);
}
void drawCandy(int x, int y) {
  lineRGBA(game->screen, x, y, x + 1, y, 255, 255, 255, 255);
  lineRGBA(game->screen, x, y + 1, x + 1, y + 1, 255, 255, 255, 255);
}
void drawDigit(int digit, int x, int y) {
  SDL_Rect dest = { x, y, 0, 0 };
  SDL_BlitSurface(game->img->digits[digit], NULL, game->screen, &dest);
}
void drawFruit() {
  SDL_Rect dest = { 215, 290, 0, 0 };
  SDL_Surface *i = (game->level > 3) ? game->img->levels[3] : game->img->levels[game->level - 1];
  SDL_BlitSurface(i, NULL, game->screen, &dest);
}
void drawHighscores() {
  SDL_Rect dest = { 38, 25, 0, 0 };
  SDL_BlitSurface(game->img->logo, NULL, game->screen, &dest);
  dest.x = 135;
  dest.y = 100;
  SDL_BlitSurface(game->img->highscores[0], NULL, game->screen, &dest);
  dest.x = 130;
  dest.y = 520;
  SDL_BlitSurface(game->img->back, NULL, game->screen, &dest);
  dest.y = 150;
  int i = 0, j;
  while (i < 10) {
    dest.x = 80;
    dest.y += 20;
    if (i == 9)
      drawNumber(10, dest.x, dest.y);
    else
      drawDigit(i + 1, dest.x, dest.y);
    dest.x += 10;
    SDL_BlitSurface(game->img->dot, NULL, game->screen, &dest);
    j = 0;
    while (j < 20) {
      dest.x += 15;
      if (game->highscores[0][i][j] > 64 && game->highscores[0][i][j] < 91)
        SDL_BlitSurface(getLetter(game->highscores[0][i][j]), NULL, game->screen, &dest);
      ++j;
    }
    drawNumber(toInt(game->highscores[1][i]), dest.x, dest.y);
    ++i;
  }
}
void drawMain() {
  SDL_Rect dest = { 38, 25, 0, 0 };
  SDL_BlitSurface(game->img->logo, NULL, game->screen, &dest);
  dest.y = 150;
  SDL_BlitSurface(game->img->anim[game->anim_index], NULL, game->screen, &dest);
  dest.x = 140;
  dest.y = 200;
  SDL_BlitSurface(game->img->legal, NULL, game->screen, &dest);
  dest.x = 80;
  dest.y = 250;
  SDL_BlitSurface(game->img->notice, NULL, game->screen, &dest);
  SDL_Surface *img = (game->selected == 0) ? game->img->playgame[1] : game->img->playgame[0];
  dest.x = 130;
  dest.y = 320;
  SDL_BlitSurface(img, NULL, game->screen, &dest);
  img = (game->selected == 1) ? game->img->highscores[1] : game->img->highscores[0];
  dest.y = 370;
  SDL_BlitSurface(img, NULL, game->screen, &dest);
  img = (game->selected == 2) ? game->img->rules[1] : game->img->rules[0];
  dest.y = 420;
  SDL_BlitSurface(img, NULL, game->screen, &dest);
  img = (game->selected == 3) ? game->img->quitgame[1] : game->img->quitgame[0];
  dest.y = 520;
  SDL_BlitSurface(img, NULL, game->screen, &dest);
}
void drawMaze() {
  // top left
  lineRGBA(game->screen, 10, 0, 230, 0, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 10, 10, 10, 180, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 0, 10, 0, 150, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 10, 150, 10, 90, 180, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 10, 160, 75, 160, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 75, 165, 5, 270, 0, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 80, 165, 80, 215, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 75, 215, 5, 0, 90, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 0, 220, 75, 220, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 0, 230, 80, 230, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 80, 220, 10, 0, 90, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 90, 160, 90, 220, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 80, 160, 10, 270, 0, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 15, 150, 80, 150, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 15, 145, 5, 90, 180, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 10, 15, 10, 144, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 15, 15, 5, 180, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 15, 10, 215, 10, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 215, 15, 5, 270, 0, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 220, 15, 220, 60, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 230, 60, 10, 90, 180, MAZE_R, MAZE_G, MAZE_B, 255);
  //top left middle block
  lineRGBA(game->screen, 50, 40, 80, 40, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 50, 50, 10, 180, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 40, 50, 40, 60, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 50, 60, 10, 90, 180, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 50, 70, 80, 70, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 80, 60, 10, 0, 90, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 90, 50, 90, 60, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 80, 50, 10, 270, 0, MAZE_R, MAZE_G, MAZE_B, 255);
  //top left tiny block
  lineRGBA(game->screen, 50, 100, 80, 100, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 50, 110, 10, 90, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 50, 120, 80, 120, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 80, 110, 10, 270, 90, MAZE_R, MAZE_G, MAZE_B, 255);
  //top left big block
  lineRGBA(game->screen, 130, 40, 180, 40, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 130, 50, 10, 180, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 120, 50, 120, 60, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 130, 60, 10, 90, 180, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 130, 70, 180, 70, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 180, 60, 10, 0, 90, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 190, 50, 190, 60, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 180, 50, 10, 270, 0, MAZE_R, MAZE_G, MAZE_B, 255);
  //top left |-
  lineRGBA(game->screen, 120, 110, 120, 220, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 130, 110, 10, 180, 0, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 140, 110, 140, 145, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 145, 145, 5, 90, 180, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 145, 150, 180, 150, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 180, 160, 10, 270, 90, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 145, 170, 180, 170, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 145, 175, 5, 180, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 140, 175, 140, 220, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 130, 220, 10, 0, 180, MAZE_R, MAZE_G, MAZE_B, 255);
  //top left T
  arcRGBA(game->screen, 180, 110, 10, 90, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 180, 100, 230, 100, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 180, 120, 215, 120, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 215, 125, 5, 270, 0, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 220, 125, 220, 160, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 230, 160, 10, 90, 180, MAZE_R, MAZE_G, MAZE_B, 255);
  //bottom left
  lineRGBA(game->screen, 0, 260, 80, 260, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 0, 270, 75, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 75, 275, 5, 270, 0, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 80, 270, 10, 270, 0, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 80, 275, 80, 325, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 90, 270, 90, 330, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 75, 325, 5, 0, 90, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 80, 330, 10, 0, 90, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 10, 330, 75, 330, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 15, 340, 80, 340, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 10, 340, 10, 180, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 15, 345, 5, 180, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 0, 340, 0, 520, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 10, 345, 10, 415, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 15, 415, 5, 90, 180, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 15, 420, 30, 420, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 30, 430, 10, 270, 90, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 15, 440, 30, 440, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 15, 445, 5, 180, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 10, 445, 10, 515, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 15, 515, 5, 90, 180, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 10, 520, 10, 90, 180, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 10, 530, 230, 530, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 15, 520, 230, 520, MAZE_R, MAZE_G, MAZE_B, 255);
  //bottom left pill block
  arcRGBA(game->screen, 130, 270, 10, 180, 0, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 120, 270, 120, 330, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 130, 330, 10, 0, 180, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 140, 270, 140, 330, MAZE_R, MAZE_G, MAZE_B, 255);
  //bottom left center block
  lineRGBA(game->screen, 170, 200, 210, 200, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 170, 200, 170, 290, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 170, 290, 235, 290, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 210, 200, 210, 210, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 180, 210, 210, 210, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 180, 210, 180, 280, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 180, 280, 230, 280, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 211, 205, 230, 205, 255, 255, 255, 255);
  //bottom left 7 block
  lineRGBA(game->screen, 50, 370, 80, 370, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 50, 380, 10, 90, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 50, 390, 65, 390, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 65, 395, 5, 270, 0, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 70, 395, 70, 430, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 80, 430, 10, 0, 180, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 90, 380, 90, 430, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 80, 380, 10, 270, 0, MAZE_R, MAZE_G, MAZE_B, 255);
  //bottom left === block
  lineRGBA(game->screen, 130, 370, 180, 370, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 130, 380, 10, 90, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 130, 390, 180, 390, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 180, 380, 10, 270, 90, MAZE_R, MAZE_G, MAZE_B, 255);
  //bottom left inversed T block
  lineRGBA(game->screen, 50, 470, 115, 470, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 115, 465, 5, 0, 90, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 120, 430, 120, 465, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 130, 430, 10, 180, 0, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 140, 430, 140, 465, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 145, 465, 5, 90, 180, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 145, 470, 180, 470, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 180, 480, 10, 270, 90, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 50, 490, 180, 490, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 50, 480, 10, 90, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  //bottom left upper T block
  lineRGBA(game->screen, 180, 320, 230, 320, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 180, 330, 10, 90, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 180, 340, 215, 340, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 215, 345, 5, 270, 0, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 220, 345, 220, 380, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 230, 380, 10, 90, 180, MAZE_R, MAZE_G, MAZE_B, 255);
  //bottom left lower T block
  lineRGBA(game->screen, 180, 420, 230, 420, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 180, 430, 10, 90, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 180, 440, 215, 440, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 215, 445, 5, 270, 0, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, 220, 445, 220, 480, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, 230, 480, 10, 90, 180, MAZE_R, MAZE_G, MAZE_B, 255);
  
  // right : vertical symetry
  int X = SCREEN_WIDTH - 2;
  // top right
  lineRGBA(game->screen, X-10, 0, X-230, 0, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-10, 10, 10, 270, 0, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-0, 10, X-0, 150, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-10, 150, 10, 0, 90, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-10, 160, X-75, 160, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-75, 165, 5, 180, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-80, 165, X-80, 215, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-75, 215, 5, 90, 180, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-0, 220, X-75, 220, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-0, 230, X-80, 230, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-80, 220, 10, 90, 180, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-90, 160, X-90, 220, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-80, 160, 10, 180, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-15, 150, X-80, 150, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-15, 145, 5, 0, 90, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-10, 15, X-10, 144, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-15, 15, 5, 270, 0, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-15, 10, X-215, 10, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-215, 15, 5, 180, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-220, 15, X-220, 60, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-230, 60, 10, 0, 90, MAZE_R, MAZE_G, MAZE_B, 255);
  //top right middle block
  lineRGBA(game->screen, X-50, 40, X-80, 40, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-50, 50, 10, 270, 0, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-40, 50, X-40, 60, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-50, 60, 10, 0, 90, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-50, 70, X-80, 70, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-80, 60, 10, 90, 180, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-90, 50, X-90, 60, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-80, 50, 10, 180, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  //top right tiny block
  lineRGBA(game->screen, X-50, 100, X-80, 100, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-50, 110, 10, 270, 90, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-50, 120, X-80, 120, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-80, 110, 10, 90, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  //top right big block
  lineRGBA(game->screen, X-130, 40, X-180, 40, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-130, 50, 10, 270, 0, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-120, 50, X-120, 60, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-130, 60, 10, 0, 90, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-130, 70, X-180, 70, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-180, 60, 10, 90, 180, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-190, 50, X-190, 60, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-180, 50, 10, 180, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  //top right |-
  lineRGBA(game->screen, X-120, 110, X-120, 220, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-130, 110, 10, 180, 0, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-140, 110, X-140, 145, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-145, 145, 5, 0, 90, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-145, 150, X-180, 150, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-180, 160, 10, 90, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-145, 170, X-180, 170, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-145, 175, 5, 270, 0, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-140, 175, X-140, 220, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-130, 220, 10, 0, 180, MAZE_R, MAZE_G, MAZE_B, 255);
  //top right T
  arcRGBA(game->screen, X-180, 110, 10, 270, 90, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-180, 100, X-230, 100, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-180, 120, X-215, 120, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-215, 125, 5, 180, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-220, 125, X-220, 160, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-230, 160, 10, 0, 90, MAZE_R, MAZE_G, MAZE_B, 255);
  //bottom right
  lineRGBA(game->screen, X-0, 260, X-80, 260, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-0, 270, X-75, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-75, 275, 5, 180, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-80, 270, 10, 180, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-80, 275, X-80, 325, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-90, 270, X-90, 330, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-75, 325, 5, 90, 180, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-80, 330, 10, 90, 180, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-10, 330, X-75, 330, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-15, 340, X-80, 340, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-10, 340, 10, 270, 0, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-15, 345, 5, 270, 0, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-0, 340, X-0, 520, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-10, 345, X-10, 415, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-15, 415, 5, 0, 90, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-15, 420, X-30, 420, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-30, 430, 10, 90, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-15, 440, X-30, 440, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-15, 445, 5, 270, 0, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-10, 445, X-10, 515, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-15, 515, 5, 0, 90, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-10, 520, 10, 0, 90, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-10, 530, X-230, 530, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-15, 520, X-230, 520, MAZE_R, MAZE_G, MAZE_B, 255);
  //bottom right pill block
  arcRGBA(game->screen, X-130, 270, 10, 180, 0, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-120, 270, X-120, 330, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-130, 330, 10, 0, 180, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-140, 270, X-140, 330, MAZE_R, MAZE_G, MAZE_B, 255);
  //bottom right center block
  lineRGBA(game->screen, X-170, 200, X-210, 200, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-170, 200, X-170, 290, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-170, 290, X-235, 290, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-210, 200, X-210, 210, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-180, 210, X-210, 210, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-180, 210, X-180, 280, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-180, 280, X-230, 280, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-211, 205, X-230, 205, 255, 255, 255, 255);
  //bottom right 7 block
  lineRGBA(game->screen, X-50, 370, X-80, 370, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-50, 380, 10, 270, 90, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-50, 390, X-65, 390, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-65, 395, 5, 180, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-70, 395, X-70, 430, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-80, 430, 10, 0, 180, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-90, 380, X-90, 430, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-80, 380, 10, 180, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  //bottom right === block
  lineRGBA(game->screen, X-130, 370, X-180, 370, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-130, 380, 10, 270, 90, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-130, 390, X-180, 390, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-180, 380, 10, 90, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  //bottom right inversed T block
  lineRGBA(game->screen, X-50, 470, X-115, 470, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-115, 465, 5, 90, 180, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-120, 430, X-120, 465, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-130, 430, 10, 180, 0, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-140, 430, X-140, 465, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-145, 465, 5, 0, 90, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-145, 470, X-180, 470, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-180, 480, 10, 90, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-50, 490, X-180, 490, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-50, 480, 10, 270, 90, MAZE_R, MAZE_G, MAZE_B, 255);
  //bottom right upper T block
  lineRGBA(game->screen, X-180, 320, X-230, 320, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-180, 330, 10, 270, 90, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-180, 340, X-215, 340, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-215, 345, 5, 180, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-220, 345, X-220, 380, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-230, 380, 10, 0, 90, MAZE_R, MAZE_G, MAZE_B, 255);
  //bottom right lower T block
  lineRGBA(game->screen, X-180, 420, X-230, 420, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-180, 430, 10, 270, 90, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-180, 440, X-215, 440, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-215, 445, 5, 180, 270, MAZE_R, MAZE_G, MAZE_B, 255);
  lineRGBA(game->screen, X-220, 445, X-220, 480, MAZE_R, MAZE_G, MAZE_B, 255);
  arcRGBA(game->screen, X-230, 480, 10, 0, 90, MAZE_R, MAZE_G, MAZE_B, 255);
}
void drawNewscorer() {
  SDL_Rect dest = { 38, 25, 0, 0 };
  SDL_BlitSurface(game->img->logo, NULL, game->screen, &dest);
  dest.x = 130;
  dest.y = 170;
  SDL_BlitSurface(game->img->enter, NULL, game->screen, &dest);
  int i = 0;
  dest.x = 100;
  dest.y = 200;
  char *str = malloc(13 * sizeof(char));
  while (i < sizeof game->highscores[0][10]) {
    if (game->highscores[0][10][i] > 64 && game->highscores[0][10][i] < 91) {
      snprintf(str, 13, "letter%d.png", game->highscores[0][10][i]);
      dest.x += 15;
      SDL_BlitSurface(getImage(str), NULL, game->screen, &dest);
    }
    ++i;
  }
  free(str);
}
void drawNumber(int score, int x, int y) {
  while (score != 0) {
    drawDigit(score % 10, x, y);
    x -= 10;
    score /= 10;
  }
}
void drawPower(int x, int y) {
  SDL_Rect dest = { x, y, 0, 0 };
  SDL_Surface *image = game->img->candy[game->candy_index];
  SDL_BlitSurface(image, NULL, game->screen, &dest);
  game->candy_blow_delay += 1;
  if (game->candy_blow_delay == 5) {
    ++(game->candy_index);
    game->candy_blow_delay = 0;
  }
  if (game->candy_index == 6)
    game->candy_index = 0;
}
void drawRules() {
  SDL_Rect dest = { 38, 25, 0, 0 };
  SDL_BlitSurface(game->img->logo, NULL, game->screen, &dest);
  dest.x = 130;
  dest.y = 100;
  SDL_BlitSurface(game->img->rules[0], NULL, game->screen, &dest);
  dest.x = 42;
  dest.y = 160;
  SDL_BlitSurface(game->img->rules_main, NULL, game->screen, &dest);
  dest.x = 130;
  dest.y = 520;
  SDL_BlitSurface(game->img->back, NULL, game->screen, &dest);
}
void eraseScreen() {
  SDL_Rect rect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
  Uint32 black = SDL_MapRGB(game->screen->format, 0x00, 0x00, 0x00);
  SDL_FillRect(game->screen, &rect, black);
}
void Game_init() {
  Game_new();
  game->delay = 100;
  game->state = 5;
  game->anim_index = 38;
  game->selected = 0;
  char *str = malloc(11 * sizeof(char));
  int i = 0;
  while (i < 6) {
    snprintf(str, 11, "candy%d.gif", i);
    game->img->candy[i++] = getImage(str);
  }
  free(str);
  game->candy_index = 0;
  game->candy_blow_delay = 0;
  char *scores_file = malloc((strlen(getenv("HOME")) + 25) * sizeof(char));
  snprintf(scores_file, strlen(getenv("HOME")) + 25, "%s/.puckman/highscores.txt", getenv("HOME"));
  game->scores_file = scores_file;
  FILE *f;
  if ((f = fopen(game->scores_file, "r")) == NULL) {
    if ((f = fopen(game->scores_file, "w")) == NULL) {
      printf("Cannot open file %s\n", game->scores_file);
      return;
    }
    i = 0;
    while (i++ < 10)
      fwrite("UNKNOWN            :0                  \n", 1, 40, f);
    fclose(f);
  }
  if ((f = fopen(game->scores_file, "r")) == NULL) {
    printf("Cannot open file %s\n", game->scores_file);
    return;
  }
  char c;
  int j = 0, k = 0;
  i = 0;
  while ((c = fgetc(f)) != EOF)
    switch (c) {
      case ':':
        k = 1;
	j = 0;
	break;
      case '\n':
        ++i;
	j = 0;
	k = 0;
	break;
      default:
        game->highscores[k][i][j] = c;
	++j;
    }
  fclose(f);
}
void Game_new() {
  game->running = 1;
  game->delay = 42;
  game->speed = 7;
  game->score = 0;
  game->lives = 3;
  game->level = 0;
  game->bonus = 0;
  game->newlife = 0;
  game->paused = 0;
  game->fruit = 0;
  game->state = 9;
  game->ticks_fruit = clock();
  game->newscorer_index = 0;
  snprintf(game->highscores[0][10], 20, "                   ");
}
void Game_process() {
  int i = 0, j, eaten = 0;
  while (i < 53) {
    j = 0;
    while (j < 46)
      if (game->walls[i][j++] == 1)
        break;
    if (j != 46)
      break;
    ++i;
  }
  if (i == 53)
    game->state = 0;
  i = 0;
  while (i < 4) {
    if ((game->ghosts->x >= game->pacman->x && game->ghosts->x <= game->pacman->x + 20 && game->pacman->y == game->ghosts->y) || (game->pacman->x >= game->ghosts->x && game->pacman->x <= game->ghosts->x + 20 && game->pacman->y == game->ghosts->y) || (game->ghosts->y >= game->pacman->y && game->ghosts->y <= game->pacman->y + 20 && game->pacman->x == game->ghosts->x) || (game->pacman->y >= game->ghosts->y && game->pacman->y <= game->ghosts->y + 20 && game->pacman->x == game->ghosts->x)) {
      if (game->ghosts->state == 1 || game->ghosts->state == 2) {
        game->state = 3;
	Ghost_scare(game->ghosts, 3);
      }
      else if (game->ghosts->state == 0 && game->state == 1) {
	game->ticks = clock();
        game->state = 2;
	game->fruit = 0;
	game->ticks_fruit = clock();
      }
    }
    if (game->ghosts->state == 3)
     ++eaten;
    ++i;
    game->ghosts = game->ghosts->next;
  }
  if (game->state == 3)
    game->bonus = 100 * (int) pow(2, eaten);
  if (game->score >= 10000 && game->newlife == 0) {
    game->newlife = 1;
    ++(game->lives);
  }
  if (game->pacman->y == 290 && (game->pacman->x >= 215 && game->pacman->x <= 244) && game->fruit == 1) {
    game->bonus = (game->level > 5) ? 700 : game->level * 100;
    game->state = 3;
  }
}
SDL_Surface *getImage(char *str) {
  int size = strlen(str) + strlen(PACPATH) + 1;
  char *path = malloc(size * sizeof(char));
  snprintf(path, size, "%s%s", PACPATH, str);
  SDL_Surface *image = IMG_Load(path);
  if (!image) {
    printf("IMG_Load: %s\n", IMG_GetError());
    cleanUp(1);
  }
  free(path);
  return image;
}
SDL_Surface *getLetter(int letter) {
  char *str = malloc(13 * sizeof(char));
  snprintf(str, 13, "letter%d.png", letter);
  SDL_Surface *image = getImage(str);
  free(str);
  return image;
}
void Ghost_chase(Ghost *ghost) {
  switch (ghost->id) {
    case BLINKY:
      Blinky_chase(ghost);
      break;
    case PINKY:
      Pinky_chase(ghost);
      break;
    case INKY:
      Inky_chase(ghost);
      break;
    case CLYDE:
      Clyde_chase(ghost);
  }
}
void Ghost_checkWay(Ghost *ghost) {
  ghost->ways[RIGHT] = 0;
  ghost->ways[LEFT] = 0;
  ghost->ways[UP] = 0;
  ghost->ways[DOWN] = 0;
  int XX = (int) ghost->x / 10;
  int YY = (int) ghost->y / 10;
  if (ghost->y % 10 == 0) {
    int Y = YY;
    int X = XX + 3;
    if (game->walls[Y++][X] != -1 && game->walls[Y++][X] != -1 && game->walls[Y][X] != -1)
      ghost->ways[RIGHT] = 1;
    Y = YY;
    X = XX - 1;
    if (game->walls[Y++][X] != -1 && game->walls[Y++][X] != -1 && game->walls[Y][X] != -1)
      ghost->ways[LEFT] = 1;
  }
  if (ghost->x % 10 == 0) {
    int Y = YY - 1;
    int X = XX;
    if ((Y == 20 && (X == 21 || X == 22)) || (game->walls[Y][X++] != -1 && game->walls[Y][X++] != -1 && game->walls[Y][X] != -1))
      ghost->ways[UP] = 1;
    X = XX;
    Y = Y + 3;
    if ((ghost->state == 3 && Y == 20 && X == 22) || (game->walls[Y][X++] != -1 && game->walls[Y][X++] != -1 && game->walls[Y][X] != -1))
      ghost->ways[DOWN] = 1;
  }
 switch (ghost->dir) {
   case RIGHT:
     ghost->ways[LEFT] = 0;
     break;
   case LEFT:
     ghost->ways[RIGHT] = 0;
     break;
   case UP:
     ghost->ways[DOWN] = 0;
     break;
   case DOWN:
     ghost->ways[UP] = 0;
 }
}
void Ghost_draw() {
  SDL_Rect dest;
  dest.w = 0;
  dest.h = 0;
  SDL_Surface *image;
  int i = 0;
  while (i < 4) {
    dest.x = (game->ghosts->dir == UP || game->ghosts->dir == DOWN) ? (game->ghosts->x + 1) : game->ghosts->x;
    dest.y = (game->ghosts->dir == RIGHT || game->ghosts->dir == LEFT) ? (game->ghosts->y + 1) : game->ghosts->y;
    switch (game->ghosts->state) {
      case 0:
        image = game->ghosts->image[game->ghosts->dir][game->ghosts->image_index];
	break;
      case 1:
        image = game->ghosts->scared[game->ghosts->image_index];
	break;
      case 2:
        image = game->ghosts->scared2[game->ghosts->image_index];
        break;
      case 3:
        image = game->ghosts->eyes[game->ghosts->dir];
	break;
      case 4:
        image = game->ghosts->image[game->ghosts->dir][game->ghosts->image_index];
    }
    SDL_BlitSurface(image, NULL, game->screen, &dest);
    if (game->ghosts->state == 2) {
      ++(game->ghosts->image_index);
      if (game->ghosts->image_index == 8)
        game->ghosts->image_index = 0;
    }
    else
      game->ghosts->image_index ^= 1;
    ++i;
    game->ghosts = game->ghosts->next;
  }
}
void Ghost_goHome(Ghost *ghost) {
  int x = 220, y = 170;
  if (y < ghost->y && ghost->ways[UP] == 1 && Ghost_moveUp(ghost) == 1)
    ghost->dir = UP;
  else if (x > ghost->x && ghost->ways[RIGHT] == 1 && Ghost_moveRight(ghost) == 1)
    ghost->dir = RIGHT;
  else if (y > ghost->y && ghost->ways[DOWN] == 1 && Ghost_moveDown(ghost) == 1)
    ghost->dir = DOWN;
  else if (x < ghost->x && ghost->ways[LEFT] == 1 && Ghost_moveLeft(ghost) == 1)
    ghost->dir = LEFT;
  else if (ghost->dir == LEFT && Ghost_moveLeft(ghost) == 0) {
    if (y >= ghost->y && ghost->ways[DOWN] == 1 && Ghost_moveDown(ghost) == 1)
      ghost->dir = DOWN;
    else if (ghost->ways[UP] == 1 && Ghost_moveUp(ghost) == 1)
      ghost->dir = UP;
    else {
      ghost->dir = DOWN;
      Ghost_moveDown(ghost);
    }
  }
  else if (ghost->dir == DOWN && Ghost_moveDown(ghost) == 0) {
    if (x >= ghost->x && ghost->ways[RIGHT] == 1 && Ghost_moveRight(ghost) == 1)
      ghost->dir = RIGHT;
    else if (ghost->ways[LEFT] == 1 && Ghost_moveLeft(ghost) == 1)
      ghost->dir = LEFT;
    else {
      ghost->dir = RIGHT;
      Ghost_moveRight(ghost);
    }
  }
  else if (ghost->dir == RIGHT && Ghost_moveRight(ghost) == 0) {
    if (y >= ghost->y && ghost->ways[DOWN] == 1 && Ghost_moveDown(ghost) == 1)
      ghost->dir = DOWN;
    else if (ghost->ways[UP] == 1 && Ghost_moveUp(ghost) == 1)
      ghost->dir = UP;
    else {
      ghost->dir = DOWN;
      Ghost_moveDown(ghost);
    }
  }
  else if (ghost->dir == UP && Ghost_moveUp(ghost) == 0) {
    if (x >= ghost->x && ghost->ways[RIGHT] == 1 && Ghost_moveRight(ghost) == 1)
      ghost->dir = RIGHT;
    else if (ghost->ways[LEFT] == 1 && Ghost_moveLeft(ghost) == 1)
      ghost->dir = LEFT;
    else {
      ghost->dir = RIGHT;
      Ghost_moveRight(ghost);
    }
  }
  if (ghost->x == 220 && ghost->y == 170)
    ghost->dir = DOWN;
}
void Ghost_init() {
  int i = 0;
  while (i < 4) {
    game->ghosts->id = i;
    switch (i) {
      case BLINKY:
        game->ghosts->x = 217;
	game->ghosts->y = 170;
	game->ghosts->dir = LEFT;
	game->ghosts->state = 0;
        break;
      case PINKY:
        game->ghosts->x = 220;
	game->ghosts->y = 250;
	game->ghosts->dir = UP;
	game->ghosts->state = 4;
        break;
      case INKY:
        game->ghosts->x = 190;
	game->ghosts->y = 250;
	game->ghosts->dir = UP;
	game->ghosts->state = 4;
	game->ghosts->initloop = 0;
        break;
      case CLYDE:
        game->ghosts->x = 250;
	game->ghosts->y = 250;
	game->ghosts->dir = LEFT;
	game->ghosts->state = 4;
	game->ghosts->initloop = 0;
    }
    Ghost_load(game->ghosts);
    game->ghosts->image_index = 0;
    game->ghosts->lowspeed = 0;
    ++i;
    game->ghosts = game->ghosts->next;
  }
}
void Ghost_load(Ghost *ghost) {
  char *str = malloc(13 * sizeof(char));
  int i = 0, j;
  while (i < 4) {
    j = 0;
    while (j < 2) {
      snprintf(str, 13, "ghost%d%d%d.gif", ghost->id, i, j);
      ghost->image[i][j++] = getImage(str);
    }
    ++i;
  }
  j = 0;
  while (j < 2) {
    snprintf(str, 13, "scared0%d.gif", j);
    ghost->scared[j++] = getImage(str);
  }
  j = 0;
  while (j < 8) {
    snprintf(str, 13, "scared1%d.gif", j);
    ghost->scared2[j++] = getImage(str);
  }
  free(str);
  str = malloc(10 * sizeof(char));
  j = 0;
  while (j < 4) {
    snprintf(str, 10, "eyes%d.png", j);
    ghost->eyes[j++] = getImage(str);
  }
  free(str);
}
void Ghost_move() {
  Ghost_unscare();
  int i = 0;
  while (i < 4) {
    Ghost_checkWay(game->ghosts);
    if (game->ghosts->state == 3) {
      if (game->ghosts->x == 220 && game->ghosts->y == 200) {
        game->ghosts->dir = UP;
        game->ghosts->state = 0;
      }
      Ghost_goHome(game->ghosts);
    }
    else if (game->ghosts->state == 4) {
      if (game->ghosts->id == PINKY) {
        if (game->ghosts->ways[UP] == 1 && Ghost_moveUp(game->ghosts) == 1)
          game->ghosts->dir = UP;
        else {
          game->ghosts->state = 0;
	  Ghost_chase(game->ghosts);
        }
      }
      else if (game->ghosts->id == INKY || game->ghosts->id == CLYDE) {
        int i = (game->ghosts->id == INKY) ? 3 : 5;
        if (game->ghosts->initloop < i) {
	  if (game->ghosts->dir == UP) {
	    if (Ghost_moveUp(game->ghosts) == 0) {
	      game->ghosts->dir = DOWN;
	      Ghost_moveDown(game->ghosts);
	      ++(game->ghosts->initloop);
	    }
	  }
	  else {
	    if (Ghost_moveDown(game->ghosts) == 0) {
	      game->ghosts->dir = UP;
	      Ghost_moveUp(game->ghosts);
	    }
	  }
	}
	else if (game->ghosts->x != 220)
	  (game->ghosts->id == INKY) ? Ghost_moveRight(game->ghosts) : Ghost_moveLeft(game->ghosts);
	else if (game->ghosts->y != 170)
	  Ghost_moveUp(game->ghosts);
	else {
	  game->ghosts->state = 0;
	  Ghost_chase(game->ghosts);
	}
      }
    }
    else if (game->ghosts->state == 1 || game->ghosts->state == 2) {
      if (game->ghosts->lowspeed == 1)
        Ghost_chase(game->ghosts);
      game->ghosts->lowspeed ^= 1;
    }
    else
      Ghost_chase(game->ghosts);
    ++i;
    game->ghosts = game->ghosts->next;
  }
}
int Ghost_moveDown(Ghost *ghost) {
  int Y = (int) ghost->y / 10 + 3;
  int X = (int) ghost->x / 10;
  if ((ghost->state == 3 && Y == 20 && X == 22) || (game->walls[Y][X++] != -1 && game->walls[Y][X++] != -1 && game->walls[Y][X] != -1)) {
    ghost->y += 1;
    return 1;
  }
  return 0;
}
int Ghost_moveLeft(Ghost *ghost) {
  if (ghost->x == 10 && ghost->y == 230)
    ghost->x = 425;
  else {
    int Y = (int) ghost->y / 10;
    int X = (int) (ghost->x - 1) / 10;
    if (game->walls[Y++][X] != -1 && game->walls[Y++][X] != -1 && game->walls[Y][X] != -1)
      ghost->x -= 1;
    else
      return 0;
  }
  return 1;
}
int Ghost_moveRight(Ghost *ghost) {
  if (ghost->x == 429 && ghost->y == 230)
    ghost->x = 10;
  else {
    int Y = (int) ghost->y / 10;
    int X = (int) ghost->x / 10 + 3;
    if (game->walls[Y++][X] != -1 && game->walls[Y++][X] != -1 && game->walls[Y][X] != -1)
      ghost->x += 1;
    else
      return 0;
  }
  return 1;
}
int Ghost_moveUp(Ghost *ghost) {
  int Y = (int) (ghost->y - 1) / 10;
  int X = (int) ghost->x / 10;
  if ((Y == 20 && (X == 21 || X == 22)) || (game->walls[Y][X++] != -1 && game->walls[Y][X++] != -1 && game->walls[Y][X] != -1)) {
    ghost->y -= 1;
    return 1;
  }
  return 0;
}
void Ghost_scare(Ghost *ghost, int s) {
  if ((ghost->state == 2 && s == 0) || (s == 2 && ghost->state == 1) || (ghost->state == 2 && s == 1) || (s == 1 && ghost->state == 0) || (s == 3 && (ghost->state == 1 || ghost->state == 2)))
    ghost->state = s;
  if (s != 2)
    ghost->image_index = 0;
}
void Ghost_unscare() {
  clock_t now = clock();
  double elapsed = ((double) now - game->ticks) / CLOCKS_PER_SEC;
  int i = 0;
  while (i < 4) {
    if (game->ghosts->state == 1 || game->ghosts->state == 2) {
      if (elapsed >= 0.3)
        Ghost_scare(game->ghosts, 0);
      else if (elapsed >= 0.2)
        Ghost_scare(game->ghosts, 2);
    }
    ++i;
    game->ghosts = game->ghosts->next;
  }
}
void Image_init() {
  game->img->life = getImage("pacman11.gif");
  game->img->lives = getImage("lives.png");
  game->img->gameover = getImage("gameover.png");
  game->img->paused = getImage("paused.png");
  game->img->getready = getImage("getready.png");
  game->img->level = getImage("level.png");
  game->img->score = getImage("score.png");
  char *str = malloc(11 * sizeof(char));
  int i = 0;
  while (i < 4) {
    snprintf(str, 11, "fruit%d.png", i);
    game->img->levels[i++] = getImage(str);
  }
  char *str2 = malloc(6 * sizeof(char));
  i = 0;
  while (i < 10) {
    snprintf(str, 6, "%d.png", i);
    game->img->digits[i++] = getImage(str);
  }
  free(str2);
  game->img->dot = getImage("dot.png");
  game->img->bonus100 = getImage("100.png");
  game->img->bonus200 = getImage("200.png");
  game->img->bonus300 = getImage("300.png");
  game->img->bonus400 = getImage("400.png");
  game->img->bonus500 = getImage("500.png");
  game->img->bonus700 = getImage("700.png");
  game->img->bonus800 = getImage("800.png");
  game->img->bonus1600 = getImage("1600.png");
  i = 0;
  while (i < 39) {
    snprintf(str, 11, "anim%d.gif", i);
    game->img->anim[i++] = getImage(str);
  }
  free(str);
  game->img->logo = getImage("logo.png");
  game->img->legal = getImage("legal.png");
  game->img->notice = getImage("notice.png");
  game->img->playgame[0] = getImage("playgame0.png");
  game->img->playgame[1] = getImage("playgame1.png");
  game->img->highscores[0] = getImage("highscores0.png");
  game->img->highscores[1] = getImage("highscores1.png");
  game->img->rules[0] = getImage("rules0.png");
  game->img->rules[1] = getImage("rules1.png");
  game->img->rules_main = getImage("rules.png");
  game->img->quitgame[0] = getImage("quitgame0.png");
  game->img->quitgame[1] = getImage("quitgame1.png");
  game->img->back = getImage("back.png");
  game->img->enter = getImage("enter.png");
}
void Inky_chase(Ghost *ghost) {
  Ghost *blinky = ghost->next->next;
  int x = game->pacman->x + 20 + abs(game->pacman->x + 20 - blinky->x);
  int y = game->pacman->y + 20 + abs(game->pacman->y + 20 - blinky->y);
  if (ghost->state == 1 || ghost->state == 2) {
    x = 461 - x;
    y = 580 - x;
  }
  if (x > ghost->x && ghost->ways[RIGHT] == 1 && Ghost_moveRight(ghost) == 1)
    ghost->dir = RIGHT;
  else if (y <= ghost->y && ghost->ways[UP] == 1 && Ghost_moveUp(ghost) == 1)
    ghost->dir = UP;
  else if (y > ghost->y && ghost->ways[DOWN] == 1 && Ghost_moveDown(ghost) == 1)
    ghost->dir = DOWN;
  else if (ghost->dir == LEFT && Ghost_moveLeft(ghost) == 0) {
    if (y >= ghost->y && ghost->ways[DOWN] == 1 && Ghost_moveDown(ghost) == 1)
      ghost->dir = DOWN;
    else if (ghost->ways[UP] == 1 && Ghost_moveUp(ghost) == 1)
      ghost->dir = UP;
    else {
      ghost->dir = DOWN;
      Ghost_moveDown(ghost);
    }
  }
  else if (ghost->dir == DOWN && Ghost_moveDown(ghost) == 0) {
    if (x >= ghost->x && ghost->ways[RIGHT] == 1 && Ghost_moveRight(ghost) == 1)
      ghost->dir = RIGHT;
    else if (ghost->ways[LEFT] == 1 && Ghost_moveLeft(ghost) == 1)
      ghost->dir = LEFT;
    else {
      ghost->dir = RIGHT;
      Ghost_moveRight(ghost);
    }
  }
  else if (ghost->dir == RIGHT && Ghost_moveRight(ghost) == 0) {
    if (y >= ghost->y && ghost->ways[DOWN] == 1 && Ghost_moveDown(ghost) == 1)
      ghost->dir = DOWN;
    else if (ghost->ways[UP] == 1 && Ghost_moveUp(ghost) == 1)
      ghost->dir = UP;
    else {
      ghost->dir = DOWN;
      Ghost_moveDown(ghost);
    }
  }
  else if (ghost->dir == UP && Ghost_moveUp(ghost) == 0) {
    if (x >= ghost->x && ghost->ways[RIGHT] == 1 && Ghost_moveRight(ghost) == 1)
      ghost->dir = RIGHT;
    else if (ghost->ways[LEFT] == 1 && Ghost_moveLeft(ghost) == 1)
      ghost->dir = LEFT;
    else {
      ghost->dir = RIGHT;
      Ghost_moveRight(ghost);
    }
  }
}
void Pacman_checkDir() {
  int YY = (int) game->pacman->y / 10;
  int XX = (int) game->pacman->x / 10;
  if (game->pacman->y % 10 == 0) {
    int Y = YY;
    int X = XX + 3;
    if (game->pacman->nextDir == RIGHT && game->walls[Y++][X] != -1 && game->walls[Y++][X] != -1 && game->walls[Y][X] != -1)
      game->pacman->dir = RIGHT;
    Y = YY;
    X = XX - 1;
    if (game->pacman->nextDir == LEFT && game->walls[Y++][X] != -1 && game->walls[Y++][X] != -1 && game->walls[Y][X] != -1)
      game->pacman->dir = LEFT;
  }
  if (game->pacman->x % 10 == 0) {
    int Y = YY - 1;
    int X = XX;
    if (game->pacman->nextDir == UP && game->walls[Y][X++] != -1 && game->walls[Y][X++] != -1 && game->walls[Y][X] != -1)
      game->pacman->dir = UP;
    X = XX;
    Y = YY + 3;
    if (game->pacman->nextDir == DOWN && game->walls[Y][X++] != -1 && game->walls[Y][X++] != -1 && game->walls[Y][X] != -1)
      game->pacman->dir = DOWN;
  }
}
void Pacman_draw() {
  int x = (game->pacman->stuck == 1 || game->pacman->dir == DOWN || game->pacman->dir == UP) ? game->pacman->x + 1 : game->pacman->x;
  int y = (game->pacman->stuck == 1 || game->pacman->dir == RIGHT || game->pacman->dir == LEFT) ? game->pacman->y + 1 : game->pacman->y;
  SDL_Rect dest = { x, y, 0, 0 };
  SDL_Surface *image = (game->state == 2) ? game->pacman->dead[game->pacman->image_index] : game->pacman->image[game->pacman->dir][game->pacman->image_index];
  SDL_BlitSurface(image, NULL, game->screen, &dest);
  game->pacman->image_index = (game->pacman->stuck == 0) ? (game->pacman->image_index + 1) : 1;
  if (game->pacman->image_index == 4)
  	game->pacman->image_index = 0;
}
void Pacman_init() {
  game->pacman->x = 217;
  game->pacman->y = 390;
  Pacman_load();
  game->pacman->image_index = 1;
  game->pacman->dir = LEFT;
  game->pacman->nextDir = LEFT;
}
void Pacman_load() {
  char *str = malloc(13 * sizeof(char));
  int i = 0, j;
  while (i < 4) {
    j = 0;
    while (j < 4) {
      snprintf(str, 13, "pacman%d%d.gif", i, j);
      game->pacman->image[i][j++] = getImage(str);
    }
    ++i;
  }
  free(str);
  str = malloc(11 * sizeof(char));
  i = 0;
  while (i < 12) {
    snprintf(str, 11, "dead%d.gif", i);
    game->pacman->dead[i++] = getImage(str);
  }
  free(str);
}
void Pacman_move() {
  Pacman_checkDir();
  game->pacman->stuck = 0;
  if (game->pacman->dir == RIGHT) {
    if (game->pacman->x == 429 && game->pacman->y == 230)
      game->pacman->x = 10;
    else {
      int Y = (int) game->pacman->y / 10;
      int X = (int) game->pacman->x / 10 + 3;
      if (game->walls[Y++][X] != -1 && game->walls[Y++][X] != -1 && game->walls[Y][X] != -1) {
        game->pacman->x += 1;
	if (game->walls[Y--][X] == 1 || game->walls[Y--][X] == 1 || game->walls[Y][X] == 1) {
	  Y += 2;
	  if (Y == 41 && X == 43) {
	    game->score += 40;
	    game->ticks = clock();
	    Ghost_scare(game->ghosts, 1);
	    Ghost_scare(game->ghosts->next, 1);
	    Ghost_scare(game->ghosts->next->next, 1);
	    Ghost_scare(game->ghosts->next->next->next, 1);
	  }
	  game->walls[Y--][X] = 0;
	  game->walls[Y--][X] = 0;
	  game->walls[Y][X] = 0;
	  game->score += 10;
	}
      }
      else
        game->pacman->stuck = 1;
    }
  }
  else if (game->pacman->dir == LEFT) {
    if (game->pacman->x == 10 && game->pacman->y == 230)
      game->pacman->x = 425;
    else {
      int Y = (int) game->pacman->y / 10;
      int X = (int) (game->pacman->x - 1) / 10;
      if (game->walls[Y++][X] != -1 && game->walls[Y++][X] != -1 && game->walls[Y][X] != -1) {
        game->pacman->x -= 1;
	if (game->walls[Y--][X] == 1 || game->walls[Y--][X] == 1 || game->walls[Y][X] == 1) {
	  Y += 2;
	  if (Y == 41 && X == 2) {
	    game->score += 40;
	    game->ticks = clock();
	    Ghost_scare(game->ghosts, 1);
	    Ghost_scare(game->ghosts->next, 1);
	    Ghost_scare(game->ghosts->next->next, 1);
	    Ghost_scare(game->ghosts->next->next->next, 1);
	  }
	  game->walls[Y--][X] = 0;
	  game->walls[Y--][X] = 0;
	  game->walls[Y][X] = 0;
	  game->score += 10;
	}
      }
      else
        game->pacman->stuck = 1;
    }
  }
  else if (game->pacman->dir == UP) {
    int Y = (int) (game->pacman->y - 1) / 10;
    int X = (int) game->pacman->x / 10;
    if (game->walls[Y][X++] != -1 && game->walls[Y][X++] != -1 && game->walls[Y][X] != -1) {
      game->pacman->y -= 1;
      if (game->walls[Y][X--] == 1 || game->walls[Y][X--] == 1 || game->walls[Y][X] == 1) {
        X += 2;
	if (Y == 4 && (X == 3 || X == 44)) {
	  game->score += 40;
	  game->ticks = clock();
	  Ghost_scare(game->ghosts, 1);
	  Ghost_scare(game->ghosts->next, 1);
	  Ghost_scare(game->ghosts->next->next, 1);
	  Ghost_scare(game->ghosts->next->next->next, 1);
	}
	game->walls[Y][X--] = 0;
	game->walls[Y][X--] = 0;
	game->walls[Y][X] = 0;
	game->score += 10;
      }
    }
    else
      game->pacman->stuck = 1;
  }
  else {
    int Y = (int) game->pacman->y / 10 + 3;
    int X = (int) game->pacman->x / 10;
    if (game->walls[Y][X++] != -1 && game->walls[Y][X++] != -1 && game->walls[Y][X] != -1) {
      game->pacman->y += 1;
      if (game->walls[Y][X--] == 1 || game->walls[Y][X--] == 1 || game->walls[Y][X] == 1) {
        X += 2;
	if ((Y == 4 || Y == 40) && (X == 3 || X == 44)) {
	  game->score += 40;
	  game->ticks = clock();
	  Ghost_scare(game->ghosts, 1);
	  Ghost_scare(game->ghosts->next, 1);
	  Ghost_scare(game->ghosts->next->next, 1);
	  Ghost_scare(game->ghosts->next->next->next, 1);
	}
	game->walls[Y][X--] = 0;
	game->walls[Y][X--] = 0;
	game->walls[Y][X] = 0;
	game->score += 10;
      }
    }
    else
      game->pacman->stuck = 1;
  }
}
void Pinky_chase(Ghost *ghost) {
  int x = game->pacman->x;
  int y = game->pacman->y;
  if (ghost->state == 1 || ghost->state == 2) {
    x = 461 - x;
    y = 580 - y;
  }
  if (y < ghost->y && ghost->ways[UP] == 1 && Ghost_moveUp(ghost) == 1)
    ghost->dir = UP;
  else if (x >= ghost->x && ghost->ways[RIGHT] == 1 && Ghost_moveRight(ghost) == 1)
    ghost->dir = RIGHT;
  else if (y >= ghost->y && ghost->ways[DOWN] == 1 && Ghost_moveDown(ghost) == 1)
    ghost->dir = DOWN;
  else if (x < ghost->x && ghost->ways[LEFT] == 1 && Ghost_moveLeft(ghost) == 1)
    ghost->dir = LEFT;
  else if (ghost->dir == LEFT && Ghost_moveLeft(ghost) == 0) {
    if (y >= ghost->y && ghost->ways[DOWN] == 1 && Ghost_moveDown(ghost) == 1)
      ghost->dir = DOWN;
    else if (ghost->ways[UP] == 1 && Ghost_moveUp(ghost) == 1)
      ghost->dir = UP;
    else {
      ghost->dir = DOWN;
      Ghost_moveDown(ghost);
    }
  }
  else if (ghost->dir == DOWN && Ghost_moveDown(ghost) == 0) {
    if (x >= ghost->x && ghost->ways[RIGHT] == 1 && Ghost_moveRight(ghost) == 1)
      ghost->dir = RIGHT;
    else if (ghost->ways[LEFT] == 1 && Ghost_moveLeft(ghost) == 1)
      ghost->dir = LEFT;
    else {
      ghost->dir = RIGHT;
      Ghost_moveRight(ghost);
    }
  }
  else if (ghost->dir == RIGHT && Ghost_moveRight(ghost) == 0) {
    if (y >= ghost->y && ghost->ways[DOWN] == 1 && Ghost_moveDown(ghost) == 1)
      ghost->dir = DOWN;
    else if (ghost->ways[UP] == 1 && Ghost_moveUp(ghost) == 1)
      ghost->dir = UP;
    else {
      ghost->dir = DOWN;
      Ghost_moveDown(ghost);
    }
  }
  else if (ghost->dir == UP && Ghost_moveUp(ghost) == 0) {
    if (x >= ghost->x && ghost->ways[RIGHT] == 1 && Ghost_moveRight(ghost) == 1)
      ghost->dir = RIGHT;
    else if (ghost->ways[LEFT] == 1 && Ghost_moveLeft(ghost) == 1)
      ghost->dir = LEFT;
    else {
      ghost->dir = RIGHT;
      Ghost_moveRight(ghost);
    }
  }
}
void raiseWalls() {
  int i = 1;
  while (i < 46) {
    game->walls[0][i] = -1;
    game->walls[52][i++] = -1;
  }
  i = 0;
  while (i < 53) {
    game->walls[i][0] = -1;
    game->walls[i++][45] = -1;
  }
  int j = 1;
  while (j < 4) {
    i = 1;
    while (i < 22)
      game->walls[j][i++] = 0;
    while (i < 24)
      game->walls[j][i++] = -1;
    while (i < 45)
      game->walls[j][i++] = 0;
    ++j;
  }
  while (j < 7) {
    i = 1;
    while (i < 4)
      game->walls[j][i++] = 0;
    while (i < 9)
      game->walls[j][i++] = -1;
    while (i < 12)
      game->walls[j][i++] = 0;
    while (i < 19)
      game->walls[j][i++] = -1;
    while (i < 22)
      game->walls[j][i++] = 0;
    while (i < 24)
      game->walls[j][i++] = -1;
    while (i < 27)
      game->walls[j][i++] = 0;
    while (i < 34)
      game->walls[j][i++] = -1;
    while (i < 37)
      game->walls[j][i++] = 0;
    while (i < 42)
      game->walls[j][i++] = -1;
    while (i < 45)
      game->walls[j][i++] = 0;
    ++j;
  }
  j = 10; 
  while (j < 12) {
    i = 1;
    while (i < 4)
      game->walls[j][i++] = 0;
    while (i < 9)
      game->walls[j][i++] = -1;
    while (i < 12)
      game->walls[j][i++] = 0;
    while (i < 14)
      game->walls[j][i++] = -1;
    while (i < 17)
      game->walls[j][i++] = 0;
    while (i < 29)
      game->walls[j][i++] = -1;
    while (i < 32)
      game->walls[j][i++] = 0;
    while (i < 34)
      game->walls[j][i++] = -1;
    while (i < 37)
      game->walls[j][i++] = 0;
    while (i < 42)
      game->walls[j][i++] = -1;
    while (i < 45)
      game->walls[j][i++] = 0;
    ++j;
  }
  while (j < 15) {
    i = 1;
    while (i < 12)
      game->walls[j][i++] = 0;
    while (i < 14)
      game->walls[j][i++] = -1;
    while (i < 22)
      game->walls[j][i++] = 0;
    while (i < 24)
      game->walls[j][i++] = -1;
    while (i < 32)
      game->walls[j][i++] = 0;
    while (i < 34)
      game->walls[j][i++] = -1;
    while (i < 45)
      game->walls[j][i++] = 0;
    ++j;
  }
  while (j < 17) {
    i = 1;
    while (i < 9)
      game->walls[j][i++] = -1;
    while (i < 12)
      game->walls[j][i++] = 0;
    while (i < 19)
      game->walls[j][i++] = -1;
    while (i < 22)
      game->walls[j][i++] = 0;
    while (i < 24)
      game->walls[j][i++] = -1;
    while (i < 27)
      game->walls[j][i++] = 0;
    while (i < 34)
      game->walls[j][i++] = -1;
    while (i < 37)
      game->walls[j][i++] = 0;
    while (i < 45)
      game->walls[j][i++] = -1;
    ++j;
  }
  while (j < 20) {
    i = 8;
    game->walls[j][i++] = -1;
    while (i < 12)
      game->walls[j][i++] = 0;
    while (i < 14)
      game->walls[j][i++] = -1;
    while (i < 32)
      game->walls[j][i++] = 0;
    while (i < 34)
      game->walls[j][i++] = -1;
    while (i < 37)
      game->walls[j][i++] = 0;
    game->walls[j++][i] = -1;
  }
  i = 8;
  game->walls[j][i++] = -1;
  while (i < 12)
    game->walls[j][i++] = 0;
  while (i < 14)
    game->walls[j][i++] = -1;
  while (i < 17)
    game->walls[j][i++] = 0;
  while (i < 29)
    game->walls[j][i++] = -1;
  while (i < 32)
    game->walls[j][i++] = 0;
  while (i < 34)
    game->walls[j][i++] = -1;
  while (i < 37)
    game->walls[j][i++] = 0;
  game->walls[j++][i] = -1;
  while (j < 23) {
    i = 1;
    while (i < 9)
      game->walls[j][i++] = -1;
    while (i < 12)
      game->walls[j][i++] = 0;
    while (i < 14)
      game->walls[j][i++] = -1;
    while (i < 17)
      game->walls[j][i++] = 0;
    game->walls[j][i++] = -1;
    while (i < 28)
      game->walls[j][i++] = 0;
    game->walls[j][i++] = -1;
    while (i < 32)
      game->walls[j][i++] = 0;
    while (i < 34)
      game->walls[j][i++] = -1;
    while (i < 37)
      game->walls[j][i++] = 0;
    while (i < 45)
      game->walls[j][i++] = -1;
    ++j;
  }
  while (j < 26) {
    i = 0;
    while (i < 17)
      game->walls[j][i++] = 0;
    game->walls[j][i++] = -1;
    while (i < 28)
      game->walls[j][i++] = 0;
    game->walls[j][i++] = -1;
    while (i < 46)
      game->walls[j][i++] = 0;
    ++j;
  }
  while (j < 28) {
    i = 1;
    while (i < 9)
      game->walls[j][i++] = -1;
    while (i < 12)
      game->walls[j][i++] = 0;
    while (i < 14)
      game->walls[j][i++] = -1;
    while (i < 17)
      game->walls[j][i++] = 0;
    game->walls[j][i++] = -1;
    while (i < 28)
      game->walls[j][i++] = 0;
    game->walls[j][i++] = -1;
    while (i < 32)
      game->walls[j][i++] = 0;
    while (i < 34)
      game->walls[j][i++] = -1;
    while (i < 37)
      game->walls[j][i++] = 0;
    while (i < 45)
      game->walls[j][i++] = -1;
    ++j;
  }
  i = 8;
  game->walls[j][i++] = -1;
  while (i < 12)
    game->walls[j][i++] = 0;
  while (i < 14)
    game->walls[j][i++] = -1;
  while (i < 17)
    game->walls[j][i++] = 0;
  while (i < 29)
    game->walls[j][i++] = -1;
  while (i < 32)
    game->walls[j][i++] = 0;
  while (i < 34)
    game->walls[j][i++] = -1;
  while (i < 37)
    game->walls[j][i++] = 0;
  game->walls[j++][i] = -1;
  while (j < 32) {
    i = 8;
    game->walls[j][i++] = -1;
    while (i < 12)
      game->walls[j][i++] = 0;
    while (i < 14)
      game->walls[j][i++] = -1;
    while (i < 32)
      game->walls[j][i++] = 0;
    while (i < 34)
      game->walls[j][i++] = -1;
    while (i < 37)
      game->walls[j][i++] = 0;
    game->walls[j++][i] = -1;
  }
  while (j < 34) {
    i = 1;
    while (i < 9)
      game->walls[j][i++] = -1;
    while (i < 12)
      game->walls[j][i++] = 0;
    while (i < 14)
      game->walls[j][i++] = -1;
    while (i < 17)
      game->walls[j][i++] = 0;
    while (i < 29)
      game->walls[j][i++] = -1;
    while (i < 32)
      game->walls[j][i++] = 0;
    while (i < 34)
      game->walls[j][i++] = -1;
    while (i < 37)
      game->walls[j][i++] = 0;
    while (i < 45)
      game->walls[j][i++] = -1;
    ++j;
  }
  while (j < 37) {
    i = 1;
    while (i < 22)
      game->walls[j][i++] = 0;
    while (i < 24)
      game->walls[j][i++] = -1;
    ++j;
  }
  while (j < 39) {
    i = 1;
    while (i < 4)
      game->walls[j][i++] = 0;
    while (i < 9)
      game->walls[j][i++] = -1;
    while (i < 12)
      game->walls[j][i++] = 0;
    while (i < 19)
      game->walls[j][i++] = -1;
    while (i < 22)
      game->walls[j][i++] = 0;
    while (i < 24)
      game->walls[j][i++] = -1;
    while (i < 27)
      game->walls[j][i++] = 0;
    while (i < 34)
      game->walls[j][i++] = -1;
    while (i < 37)
      game->walls[j][i++] = 0;
    while (i < 42)
      game->walls[j][i++] = -1;
    ++j;
  }
  while (j < 42) {
    i = 1;
    while (i < 7)
      game->walls[j][i++] = 0;
    while (i < 9)
      game->walls[j][i++] = -1;
    while (i < 37)
      game->walls[j][i++] = 0;
    while (i < 39)
      game->walls[j][i++] = -1;
    while (i < 45)
      game->walls[j][i++] = 0;
    ++j;
  }
  while (j < 44) {
    i = 1;
    while (i < 4)
      game->walls[j][i++] = -1;
    while (i < 7)
      game->walls[j][i++] = 0;
    while (i < 9)
      game->walls[j][i++] = -1;
    while (i < 12)
      game->walls[j][i++] = 0;
    while (i < 14)
      game->walls[j][i++] = -1;
    while (i < 17)
      game->walls[j][i++] = 0;
    while (i < 29)
      game->walls[j][i++] = -1;
    while (i < 32)
      game->walls[j][i++] = 0;
    while (i < 34)
      game->walls[j][i++] = -1;
    while (i < 37)
      game->walls[j][i++] = 0;
    while (i < 39)
      game->walls[j][i++] = -1;
    while (i < 42)
      game->walls[j][i++] = 0;
    while (i < 45)
      game->walls[j][i++] = -1;
    ++j;
  }
  while (j < 47) {
    i = 1;
    while (i < 12)
      game->walls[j][i++] = 0;
    while (i < 14)
      game->walls[j][i++] = -1;
    while (i < 22)
      game->walls[j][i++] = 0;
    while (i < 24)
      game->walls[j][i++] = -1;
    while (i < 32)
      game->walls[j][i++] = 0;
    while (i < 34)
      game->walls[j][i++] = -1;
    ++j;
  }
  while (j < 49) {
    i = 1;
    while (i < 4)
      game->walls[j][i++] = 0;
    while (i < 19)
      game->walls[j][i++] = -1;
    while (i < 22)
      game->walls[j][i++] = 0;
    while (i < 24)
      game->walls[j][i++] = -1;
    while (i < 27)
      game->walls[j][i++] = 0;
    while (i < 42)
      game->walls[j][i++] = -1;
    ++j;
  }
  game->walls[2][2] = 1;
  game->walls[2][4] = 1;
  game->walls[2][5] = 1;
  game->walls[2][7] = 1;
  game->walls[2][8] = 1;
  game->walls[2][10] = 1;
  game->walls[2][12] = 1;
  game->walls[2][13] = 1;
  game->walls[2][15] = 1;
  game->walls[2][17] = 1;
  game->walls[2][18] = 1;
  game->walls[2][20] = 1;
  game->walls[2][25] = 1;
  game->walls[2][27] = 1;
  game->walls[2][28] = 1;
  game->walls[2][30] = 1;
  game->walls[2][31] = 1;
  game->walls[2][33] = 1;
  game->walls[2][35] = 1;
  game->walls[2][37] = 1;
  game->walls[2][38] = 1;
  game->walls[2][40] = 1;
  game->walls[2][41] = 1;
  game->walls[2][43] = 1;
  i = 4;
  while (i < 7) {
    game->walls[i][2] = 1;
    game->walls[i][10] = 1;
    game->walls[i][20] = 1;
    game->walls[i][25] = 1;
    game->walls[i][35] = 1;
    game->walls[i++][43] = 1;
  }
  game->walls[8][2] = 1;
  game->walls[8][4] = 1;
  game->walls[8][5] = 1;
  game->walls[8][7] = 1;
  game->walls[8][8] = 1;
  game->walls[8][10] = 1;
  game->walls[8][12] = 1;
  game->walls[8][13] = 1;
  game->walls[8][15] = 1;
  game->walls[8][17] = 1;
  game->walls[8][18] = 1;
  game->walls[8][20] = 1;
  game->walls[8][22] = 1;
  game->walls[8][23] = 1;
  game->walls[8][25] = 1;
  game->walls[8][27] = 1;
  game->walls[8][28] = 1;
  game->walls[8][30] = 1;
  game->walls[8][31] = 1;
  game->walls[8][33] = 1;
  game->walls[8][35] = 1;
  game->walls[8][37] = 1;
  game->walls[8][38] = 1;
  game->walls[8][40] = 1;
  game->walls[8][41] = 1;
  game->walls[8][43] = 1;
  i = 10;
  while (i < 12) {
    game->walls[i][2] = 1;
    game->walls[i][10] = 1;
    game->walls[i][15] = 1;
    game->walls[i][30] = 1;
    game->walls[i][35] = 1;
    game->walls[i++][43] = 1;
  }
  game->walls[13][2] = 1;
  game->walls[13][4] = 1;
  game->walls[13][5] = 1;
  game->walls[13][7] = 1;
  game->walls[13][8] = 1;
  game->walls[13][10] = 1;
  game->walls[13][15] = 1;
  game->walls[13][17] = 1;
  game->walls[13][18] = 1;
  game->walls[13][20] = 1;
  game->walls[13][25] = 1;
  game->walls[13][27] = 1;
  game->walls[13][28] = 1;
  game->walls[13][30] = 1;
  game->walls[13][35] = 1;
  game->walls[13][37] = 1;
  game->walls[13][38] = 1;
  game->walls[13][40] = 1;
  game->walls[13][41] = 1;
  game->walls[13][43] = 1;
  i = 15;
  while (i < 34) {
    game->walls[i][10] = 1;
    game->walls[i++][35] = 1;
    if (i == 17 || i == 19 || i == 23 || i == 25 || i == 29 || i == 31)
      ++i;
  }
  game->walls[35][2] = 1;
  game->walls[35][4] = 1;
  game->walls[35][5] = 1;
  game->walls[35][7] = 1;
  game->walls[35][8] = 1;
  game->walls[35][10] = 1;
  game->walls[35][12] = 1;
  game->walls[35][13] = 1;
  game->walls[35][15] = 1;
  game->walls[35][17] = 1;
  game->walls[35][18] = 1;
  game->walls[35][20] = 1;
  game->walls[35][25] = 1;
  game->walls[35][27] = 1;
  game->walls[35][28] = 1;
  game->walls[35][30] = 1;
  game->walls[35][31] = 1;
  game->walls[35][33] = 1;
  game->walls[35][35] = 1;
  game->walls[35][37] = 1;
  game->walls[35][38] = 1;
  game->walls[35][40] = 1;
  game->walls[35][41] = 1;
  game->walls[35][43] = 1;
  i = 37;
  while (i < 39) {
    game->walls[i][2] = 1;
    game->walls[i][10] = 1;
    game->walls[i][20] = 1;
    game->walls[i][25] = 1;
    game->walls[i][35] = 1;
    game->walls[i++][43] = 1;
  }
  game->walls[40][2] = 1;
  game->walls[40][4] = 1;
  game->walls[40][5] = 1;
  game->walls[40][10] = 1;
  game->walls[40][12] = 1;
  game->walls[40][13] = 1;
  game->walls[40][15] = 1;
  game->walls[40][17] = 1;
  game->walls[40][18] = 1;
  game->walls[40][20] = 1;
  game->walls[40][25] = 1;
  game->walls[40][27] = 1;
  game->walls[40][28] = 1;
  game->walls[40][30] = 1;
  game->walls[40][31] = 1;
  game->walls[40][33] = 1;
  game->walls[40][35] = 1;
  game->walls[40][40] = 1;
  game->walls[40][41] = 1;
  game->walls[40][43] = 1;
  i = 42;
  while (i < 44) {
    game->walls[i][5] = 1;
    game->walls[i][10] = 1;
    game->walls[i][15] = 1;
    game->walls[i][30] = 1;
    game->walls[i][35] = 1;
    game->walls[i++][40] = 1;
  }
  game->walls[45][2] = 1;
  game->walls[45][4] = 1;
  game->walls[45][5] = 1;
  game->walls[45][7] = 1;
  game->walls[45][8] = 1;
  game->walls[45][10] = 1;
  game->walls[45][15] = 1;
  game->walls[45][17] = 1;
  game->walls[45][18] = 1;
  game->walls[45][20] = 1;
  game->walls[45][25] = 1;
  game->walls[45][27] = 1;
  game->walls[45][28] = 1;
  game->walls[45][30] = 1;
  game->walls[45][35] = 1;
  game->walls[45][37] = 1;
  game->walls[45][38] = 1;
  game->walls[45][40] = 1;
  game->walls[45][41] = 1;
  game->walls[45][43] = 1;
  i = 47;
  while (i < 49) {
    game->walls[i][2] = 1;
    game->walls[i][20] = 1;
    game->walls[i][25] = 1;
    game->walls[i++][43] = 1;
  }
  game->walls[50][2] = 1;
  game->walls[50][4] = 1;
  game->walls[50][5] = 1;
  game->walls[50][7] = 1;
  game->walls[50][8] = 1;
  game->walls[50][10] = 1;
  game->walls[50][12] = 1;
  game->walls[50][13] = 1;
  game->walls[50][15] = 1;
  game->walls[50][17] = 1;
  game->walls[50][18] = 1;
  game->walls[50][20] = 1;
  game->walls[50][22] = 1;
  game->walls[50][23] = 1;
  game->walls[50][25] = 1;
  game->walls[50][27] = 1;
  game->walls[50][28] = 1;
  game->walls[50][30] = 1;
  game->walls[50][31] = 1;
  game->walls[50][33] = 1;
  game->walls[50][35] = 1;
  game->walls[50][37] = 1;
  game->walls[50][38] = 1;
  game->walls[50][40] = 1;
  game->walls[50][41] = 1;
  game->walls[50][43] = 1;
}
void sort() {
  int unsorted = 1, i;
  while (unsorted) {
    unsorted = 0;
    i = 10;
    while (i > 0) {
      if (toInt(game->highscores[1][i]) > toInt(game->highscores[1][i - 1])) {
	swap(i);
	unsorted = 1;
      }
      --i;
    }
  }
  writeScores();
}
void swap(int i) {
  char temp[2][20];
  snprintf(temp[0], 20, "%s", game->highscores[0][i]);
  snprintf(temp[1], 20, "%s", game->highscores[1][i]);
  snprintf(game->highscores[0][i], 20, "%s", game->highscores[0][i - 1]);
  snprintf(game->highscores[1][i], 20, "%s", game->highscores[1][i - 1]);
  snprintf(game->highscores[0][i - 1], 20, "%s", temp[0]);
  snprintf(game->highscores[1][i - 1], 20, "%s", temp[1]);
}
int toInt(char score[20]) {
  int i = 0, r = 0, j = 0;
  while (i < 20) {
    if (score[19 - i] > 47 && score[19 - i] < 58)
      r += (score[19 - i] - 48) * (int) pow(10, j++);
    ++i;
  }
  return r;
}
void writeScores() {
  FILE *f;
  if ((f = fopen(game->scores_file, "w")) == NULL) {
    printf("Cannot open file %s\n", game->scores_file);
    return;
  }
  char *str = malloc(41 * sizeof(char));
  int i = 0;
  while (i < 10) {
    snprintf(str, 41, "%s:%s\n", game->highscores[0][i], game->highscores[1][i]);
    fwrite(str, sizeof(char), 40, f);
    ++i;
  }
  free(str);
  fclose(f);
}
