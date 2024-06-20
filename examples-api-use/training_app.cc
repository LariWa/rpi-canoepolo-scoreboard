/*
  Training timer which runs on Shotclock display

  Runs on a 48x32 pixel LED-matrix. (Three 16x32 modules in Z-alignment).

  Copyright 2021 Hans Unzner (hansunzner@gmail.com)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 */


#include "led-matrix.h"
#include "graphics.h"

#include <unistd.h>
// #include <math.h>
#include <stdio.h>
// #include <signal.h>
// #include <stdlib.h>
#include <string.h>
// #include <thread>
// #include <iostream>
// #include <string>
#include <ctime>
#include <pigpio.h>
#include <thread>         // std::thread

using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;
using namespace std;
bool run = false, reset = false;

#define PLAY_TIME 600 // in sec
#define PAUSE_TIME 10 // in sec
#define POWERSAVE_TIME_1 100 // time until brightness switches to 50%
#define POWERSAVE_TIME_2 120 // time until time display turns off

void pollButtons()
{

  while(1)
  {
    // printf("GPIO 21 (3): %d\n", gpioRead(21));
    if (!gpioRead(21)) {
      int longPressCounter = 0;
      while (!gpioRead(21)){  // Waiting for release
        longPressCounter ++;
        if (longPressCounter > 2000) break;
        usleep(1000);
      }
      if (longPressCounter > 2000) {
        reset = true;
        longPressCounter = 0;
        printf("resetting ..\n");
        while (!gpioRead(21));  // Waiting for release
      } else {
        run = !run;
        printf("run = %d (button state: %d)\n", run, !gpioRead(21));
      }
    }
    usleep(50000);
  }
}


int Training_Application(RGBMatrix *matrix)
{
  rgb_matrix::Color color_red(255, 0, 0);
  rgb_matrix::Color color_yellow(250, 190, 0);
  rgb_matrix::Color color_yellow_50(125, 85, 0);
  rgb_matrix::Color color_blue(0, 50, 255);
  rgb_matrix::Color color_green(0, 200, 0);
  rgb_matrix::Color color_white(200, 200, 200);
  rgb_matrix::Color color_white_50(100, 100, 100);
  rgb_matrix::Color color_orange(250, 130, 0);
  rgb_matrix::Color bg_color(0, 0, 0);
  rgb_matrix::Color outline_color(255,255,255);

  rgb_matrix::Color *CounterColor_normal;
  rgb_matrix::Color *CounterColor_pause;
  CounterColor_normal = &color_white;
  CounterColor_pause = &color_yellow;

#define FONT_TIME_NARROW_PATH "fonts2/LiberationSansNarrow_b11x17.bdf"
#define FONT_TIME_WIDE_PATH "fonts2/LiberationSans_b13x17.bdf"
// #define FONT_CLOCK_PATH "fonts2/LiberationSans_b10x12.bdf"
#define FONT_CLOCK_PATH "fonts2/LiberationSansNarrow_b8x12.bdf"

    /*
   * Load bdf bitmap fonts.
   */
  rgb_matrix::Font font_clock, font_time_narrow, font_time_wide;

  if (!font_clock.LoadFont("../" FONT_CLOCK_PATH)) {
    if (!font_clock.LoadFont("Scoreboard/" FONT_CLOCK_PATH)) {
        fprintf(stderr, "Couldn't load font '%s'\n", FONT_CLOCK_PATH);
        return 1;
    }
  }
  if (!font_time_narrow.LoadFont("../" FONT_TIME_NARROW_PATH)) {
    if (!font_time_narrow.LoadFont("Scoreboard/" FONT_TIME_NARROW_PATH)) {
        fprintf(stderr, "Couldn't load font '%s'\n", FONT_TIME_NARROW_PATH);
        return 1;
    }
  }
  if (!font_time_wide.LoadFont("../" FONT_TIME_WIDE_PATH)) {
    if (!font_time_wide.LoadFont("Scoreboard/" FONT_TIME_WIDE_PATH)) {
        fprintf(stderr, "Couldn't load font '%s'\n", FONT_TIME_WIDE_PATH);
        return 1;
    }
  }

  char sTime[24];
  int GameCounter = PLAY_TIME, BreakCounter = PAUSE_TIME; //, Clock = 18 * 3600;
  bool Pause = true;
  std::time_t time_now;
  std::tm *tm;
  gpioSetMode(20, PI_INPUT);
  gpioSetMode(21, PI_INPUT);
  gpioSetPullUpDown(20, PI_PUD_UP);
  gpioSetPullUpDown(21, PI_PUD_UP);

  printf("GPIO 21 (1): %d\n", gpioRead(21));
  for(int i=0; i<50; i++){
    if(gpioRead(21)) break;
    usleep(10000);
  }
  printf("GPIO 21 (2): %d\n", gpioRead(21));
  std::thread poll_thread (pollButtons);
  int powersaveCounter = 0;

  while(1){
    matrix->Clear();
    // sprintf(sTime, "%2d:%02d", Clock/3600, (Clock/60)%60);
    // Clock ++;
    std::time(&time_now); // update time
    tm = std::localtime(&time_now);
    strftime(sTime, sizeof(sTime), "%H:%M\n", tm);

    // // Don't display clock if probably wrong
    // if((tm->tm_hour >= 17) && (tm->tm_hour < 21)){
    //   rgb_matrix::DrawText(matrix, font_clock, 9, 13, color_red,  &bg_color, sTime);
    // }
    rgb_matrix::DrawText(matrix, font_clock, 8, 13, color_red,  &bg_color, sTime);

    if(reset) {
      reset = false;
      run = false;
      Pause = false;
      GameCounter = PLAY_TIME;
      sprintf(sTime, "%2d:%02d", GameCounter/60, GameCounter%60);
      rgb_matrix::DrawText(matrix, font_time_narrow, -4, 32, *CounterColor_normal,  &bg_color, sTime);
    }

    // Decrease brightness after POWERSAVE_TIME_1 seconds and turn countdown time off after POWERSAVE_TIME_2 seconds
    if(!run){
      if(powersaveCounter <= POWERSAVE_TIME_2) {
        powersaveCounter ++;
        if(powersaveCounter == POWERSAVE_TIME_1) {
          CounterColor_normal = &color_white_50;
          CounterColor_pause = &color_yellow_50;
          printf("Set brightness to 50%%\n");
        }
        if(powersaveCounter == POWERSAVE_TIME_2) {
          CounterColor_normal = &bg_color;
          CounterColor_pause = &bg_color;
          printf("Set brightness to 0%%\n");
        }
      }
    } else if(powersaveCounter) {
      CounterColor_normal = &color_white;
      CounterColor_pause = &color_yellow;
      printf("Set brightness to 100%%\n");
      powersaveCounter = 0;
    }

    if(Pause) {
      if(BreakCounter == 0) {
        Pause = false;
        BreakCounter = PAUSE_TIME;
        continue;
      } else {
        if(run) BreakCounter--;
      }
      if(BreakCounter < 600) {
        sprintf(sTime, "%1d:%02d", BreakCounter/60, BreakCounter%60);
        rgb_matrix::DrawText(matrix, font_time_wide, 0, 32, *CounterColor_pause,  &bg_color, sTime);
      } else {
        sprintf(sTime, "%2d:%02d", BreakCounter/60, BreakCounter%60);
        rgb_matrix::DrawText(matrix, font_time_narrow, 0, 32, *CounterColor_pause,  &bg_color, sTime);
      }
    } else {
      if(GameCounter == 0) {
        Pause = true;
        GameCounter = PLAY_TIME;
        continue;
      } else {
        if(run) GameCounter--;
      }
      if(GameCounter < 600) {
        sprintf(sTime, "%1d:%02d", GameCounter/60, GameCounter%60);
        rgb_matrix::DrawText(matrix, font_time_wide, 0, 32, *CounterColor_normal,  &bg_color, sTime);
      } else {
        sprintf(sTime, "%2d:%02d", GameCounter/60, GameCounter%60);
        rgb_matrix::DrawText(matrix, font_time_narrow, -4, 32, *CounterColor_normal,  &bg_color, sTime);
      }
    }

    sleep(1);
  }


}
