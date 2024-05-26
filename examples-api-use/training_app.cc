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

using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;
using namespace std;



int Training_Application(Canvas *canvas)
{
  rgb_matrix::Color color_red(255, 0, 0);
  rgb_matrix::Color color_yellow(250, 190, 0);
  rgb_matrix::Color color_blue(0, 50, 255);
  rgb_matrix::Color color_green(0, 200, 0);
  rgb_matrix::Color color_white(200, 200, 200);
  rgb_matrix::Color color_orange(250, 130, 0);
  rgb_matrix::Color bg_color(0, 0, 0);
  rgb_matrix::Color outline_color(255,255,255);

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
  int GameCounter = 600, BreakCounter = 60; //, Clock = 18 * 3600;
  bool Pause = false;
  std::time_t time_now;
  std::tm *tm;

  while(1){
    canvas->Clear();
    // sprintf(sTime, "%2d:%02d", Clock/3600, (Clock/60)%60);
    // Clock ++;
    std::time(&time_now); // update time
    tm = std::localtime(&time_now);
    strftime(sTime, sizeof(sTime), "%H:%M\n", tm);

    // Don't display clock if probably wrong
    if((tm->tm_hour >= 17) && (tm->tm_hour < 21)){
      rgb_matrix::DrawText(canvas, font_clock, 9, 13, color_red,  &bg_color, sTime);
    }

    if(Pause) {
      if(BreakCounter < 600) {
        sprintf(sTime, "%1d:%02d", BreakCounter/60, BreakCounter%60);
        rgb_matrix::DrawText(canvas, font_time_wide, 0, 32, color_yellow,  &bg_color, sTime);
      } else {
        sprintf(sTime, "%2d:%02d", BreakCounter/60, BreakCounter%60);
        rgb_matrix::DrawText(canvas, font_time_narrow, 0, 32, color_yellow,  &bg_color, sTime);
      }
      if(BreakCounter == 0) {
        Pause = false;
        BreakCounter = 60;
        continue;
      } else {
        BreakCounter--;
      }
    } else {
      if(GameCounter < 600) {
        sprintf(sTime, "%1d:%02d", GameCounter/60, GameCounter%60);
        rgb_matrix::DrawText(canvas, font_time_wide, 0, 32, color_white,  &bg_color, sTime);
      } else {
        sprintf(sTime, "%2d:%02d", GameCounter/60, GameCounter%60);
        rgb_matrix::DrawText(canvas, font_time_narrow, -4, 32, color_white,  &bg_color, sTime);
      }
      if(GameCounter == 0) {
        Pause = true;
        GameCounter = 600;
        continue;
      } else {
        GameCounter--;
      }
    }
    sleep(1);
  }


}
