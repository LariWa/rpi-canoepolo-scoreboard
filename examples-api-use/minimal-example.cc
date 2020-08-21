// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// Small example how to use the library.
// For more examples, look at demo-main.cc
//
// This code is public domain
// (but note, that the led-matrix library this depends on is GPL v2)

#include "led-matrix.h"
#include "graphics.h"

#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <thread>
#include "mytimer.h"
#include <time.h>
#include <iostream>
#include <string>
 
using rgb_matrix::GPIO;
using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;

volatile bool bUpdateDisplay = true;
char sScoreA[24], sScoreB[24], sTime[24];
typedef enum states { idle, running, paused } states_t;
typedef enum colors { white, yellow, red, blue, green } colors_t;
#define NUM_COLORS 5

class DisplayData
{
public:
  DisplayData() : m_nScoreA(0), m_nScoreB(0), m_nPlayTimeSec(600), m_bTimerStarted(false), m_state(idle), m_teamAColorIndex(white), m_teamBColorIndex(white)  { }

  int getScoreA()  { return m_nScoreA; } 
  
  int getScoreB()  { return m_nScoreB; }

  void setScoreA(int score)  { m_nScoreA = score; }

  void setScoreB(int score)  { m_nScoreB = score; }

  void incScoreA()  { m_nScoreA++; }

  void incScoreB()  { m_nScoreB++; }

  void decScoreA()  {
    if(m_nScoreA > 0)
      m_nScoreA--;
  }

  void decScoreB()
  {
    if(m_nScoreB > 0)
      m_nScoreB--;
  }

  void resetScore()
  {
    m_nScoreA = 0;
    m_nScoreB = 0;
  }

  int getMin()  { return m_nPlayTimeSec/60; }

  int getSec()  { return m_nPlayTimeSec%60; }

  void setTime(int nSec)
  {
    if(!m_bTimerStarted)
      m_nPlayTimeSec = nSec;
  }

  void setTime(int nMin, int nSec)
  {
    if(!m_bTimerStarted)
      m_nPlayTimeSec = nMin*60 + nSec;
  }

  void decTime()  { m_nPlayTimeSec--; }
  
  void modifyTime(int nValue)
  {
    if(m_state != running){
      m_nPlayTimeSec += nValue;
      
      if(m_nPlayTimeSec == -55)
        m_nPlayTimeSec = 0;
      else if(m_nPlayTimeSec <= 0)
        m_nPlayTimeSec = 5;
    }
  }

  void startTimer()
  {
    m_nStartTime = time(NULL);  // get current time
    m_nSeconds = 0;
    m_bTimerStarted = true;
  }

  void stopTimer()  { m_bTimerStarted = false; }

  void updateTime()
  {
    if(m_bTimerStarted)
    {
      m_nSecondsLast = m_nSeconds;
      m_nSeconds = (int)difftime(time(NULL), m_nStartTime);
      if(m_nSecondsLast != m_nSeconds)
      {
        m_nPlayTimeSec -= (m_nSeconds - m_nSecondsLast);
        if(m_nPlayTimeSec <= 0)
        {
          m_nPlayTimeSec = 0;
          m_bTimerStarted = false;
          m_state = idle;
        }
        bUpdateDisplay = true;
      }
    }
  }
  void setState(states_t state)  { m_state = state; }

  states_t getState(void)
  {
    return m_state;
  }
  
  void start_pause(){    
    if(m_state == running){
      stopTimer();
      setState(paused);
    } else {
      startTimer(); 
      setState(running);
    }    
  }
  
  colors_t getColorIndexA(void){ return m_teamAColorIndex; }    
  
  colors_t getColorIndexB(void){ return m_teamBColorIndex; }
  
  void nextColorIndexA(void){    
    m_teamAColorIndex = (colors_t)((int)m_teamAColorIndex+1);    
    if(m_teamAColorIndex >= NUM_COLORS)
      m_teamAColorIndex = (colors_t)0;
  }
   
  void nextColorIndexB(void){    
    m_teamBColorIndex = (colors_t)((int)m_teamBColorIndex+1);    
    if(m_teamBColorIndex >= NUM_COLORS)
      m_teamBColorIndex = (colors_t)0;
  }         
private:
  int m_nScoreA, m_nScoreB, m_nPlayTimeSec;
  colors_t m_teamAColorIndex, m_teamBColorIndex;
  time_t m_nStartTime;
  int m_nSeconds, m_nSecondsLast;
  bool m_bTimerStarted;
  states_t m_state;  

};

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

void KeyboardInput(DisplayData& dispData);
volatile bool bExit = false;


rgb_matrix::Color color_red(255, 0, 0);
rgb_matrix::Color color_yellow(214, 183, 0);
rgb_matrix::Color color_blue(0, 30, 220);
rgb_matrix::Color color_green(0, 200, 0);
rgb_matrix::Color color_white(180, 180, 180);

rgb_matrix::Color* pTimeColor;
rgb_matrix::Color* pTeamAColor;
rgb_matrix::Color* pTeamBColor;


rgb_matrix::Color* GetPColor(colors_t nColorIndex){
  switch(nColorIndex){
    case red:    return &color_red;
    case yellow: return &color_yellow;
    case blue:   return &color_blue;
    case green:  return &color_green;
    case white: 
    default: 
                 return &color_white; 
    }  
}




int main(int argc, char *argv[]) {

  rgb_matrix::RuntimeOptions runtime;  
  runtime.gpio_slowdown = 4;
  
  RGBMatrix::Options options;
  options.hardware_mapping = "regular";  // or e.g. "adafruit-hat"
  options.rows = 16;
  options.cols = 32;
  options.chain_length = 5;
  options.parallel = 2;
  //options.pixel_mapper_config = "V-mapper:Z;Rotate:90";
  options.show_refresh_rate = false;
  //options.pwm_lsb_nanoseconds = 200;
  options.brightness = 20;
  options.multiplexing = 3;
  options.inverse_colors = false;
  options.led_rgb_sequence = "RGB";

  // initializing curses lib
  initscr();
  noecho();
  timeout(-1); // set to blocking mode - otherwise time out value

  Canvas *canvas = rgb_matrix::CreateMatrixFromFlags(&argc, &argv, &options);
  if (canvas == NULL)
    return 1;

  // It is always good to set up a signal handler to cleanly exit when we
  // receive a CTRL-C for instance. The DrawOnCanvas() routine is looking
  // for that.
  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);


  rgb_matrix::Color bg_color(0, 0, 0);
  rgb_matrix::Color outline_color(255,255,255);
  int letter_spacing = 0;

  /*
   * Load bdf bitmap fonts.
   */
  rgb_matrix::Font font_std, font_narr;
  
  if (!font_std.LoadFont("../fonts2/LiberationSansNarrow_bb32.bdf")) {
    fprintf(stderr, "Couldn't load std font '%s'\n", "../fonts2/LiberationSansNarrow_bb32.bdf");
    return 1;
  }
  if (!font_narr.LoadFont("../fonts2/antonio_b32.bdf")) {
    fprintf(stderr, "Couldn't load narrow font '%s'\n", "../fonts2/antonio_b32.bdf");
    return 1;
  }
  
  pTimeColor = &color_red;;
  pTeamAColor = &color_white;
  pTeamBColor = &color_white;
  
  DisplayData dispData;
  std::thread inputThread(KeyboardInput, std::ref(dispData));


  while(1){
    dispData.updateTime();

    if(bUpdateDisplay){

      canvas->Clear();
      
      if(dispData.getScoreA() < 10){
        sprintf(sScoreA, "%d ", dispData.getScoreA());
        rgb_matrix::DrawText(canvas, font_std, 0, 32, *GetPColor(dispData.getColorIndexA()), &bg_color, sScoreA, letter_spacing);
      } else{      
        sprintf(sScoreA, "%2d ", dispData.getScoreA());
        rgb_matrix::DrawText(canvas, font_narr, 0, 32, *GetPColor(dispData.getColorIndexA()), &bg_color, sScoreA, letter_spacing);
      }
      
      sprintf(sTime, "%2d:%02d", dispData.getMin(), dispData.getSec());    
      if(dispData.getMin() < 10)
        rgb_matrix::DrawText(canvas, font_std, 29, 32, *pTimeColor,  &bg_color, sTime,   letter_spacing);
      else if(dispData.getMin() < 20)
        rgb_matrix::DrawText(canvas, font_std, 36, 32, *pTimeColor,  &bg_color, sTime,   letter_spacing);
      else
        rgb_matrix::DrawText(canvas, font_std, 33, 32, *pTimeColor,  &bg_color, sTime,   letter_spacing);
                 
      if(dispData.getScoreB() < 10){
        sprintf(sScoreB, "%d", dispData.getScoreB());
        rgb_matrix::DrawText(canvas, font_std, 139, 32, *GetPColor(dispData.getColorIndexB()), &bg_color, sScoreB, letter_spacing);
      } else {
        sprintf(sScoreB, "%2d", dispData.getScoreB());
        if(dispData.getScoreB() < 20)
          rgb_matrix::DrawText(canvas, font_narr, 128+5, 32, *GetPColor(dispData.getColorIndexB()), &bg_color, sScoreB, letter_spacing);
        else
          rgb_matrix::DrawText(canvas, font_narr, 128, 32, *GetPColor(dispData.getColorIndexB()), &bg_color, sScoreB, letter_spacing);
      }    
      
//      Für Stromaufnahme-Test      
//      canvas->Clear();
//      for(int i=0; i<16; i++){
//       rgb_matrix::DrawLine(canvas, 0, i, 768, i, outline_color);
//      }
      
      bUpdateDisplay = false;
    }
    if(bExit)
    {
      break;
    }
    usleep(200 * 1000);
  }

  endwin();
  canvas->Clear();
  delete canvas;
  return 0;
}

/*
 * Search for str2 in str1
 * Return true if str2 found at the end of str1 
 */
bool rfind_str(std::string &str1, std::string str2){
  return (str1.substr(str1.size()-str2.size(), str2.size()) == str2);
}

void KeyboardInput(DisplayData& dispData)
{
  char nInput;
  static int nResetCnt=0, nShutdownCnt=0;
  static std::string sBuf = "    ";

  const std::string sIncScoreA =    {"\e[A"};  // up
  const std::string sDecScoreA =    {"\e[B"};  // down  
  const std::string sIncScoreB =    {"\e[5~"}; // pg up
  const std::string sDecScoreB =    {"\e[6~"}; // pg down
  const std::string sColorChangeA = {"\e[G"};  // keypad 5
  const std::string sColorChangeB = {"\e[C"};  // right

   
  while(1){
    // Check input
    nInput = getch();
    switch(nInput){
      case '8': dispData.incScoreA(); break;
      case '2': dispData.decScoreA(); break;            
      case '9': dispData.incScoreB(); break;
      case '3': dispData.decScoreB(); break;
      case 'r': 
        nShutdownCnt++;
        if(nShutdownCnt > 150)
        {
          //system("echo raspberry | sudo -S poweroff");
          system("sudo shutdown now");
        }  
        break;
      case 127: // backspace     
        break;       
      // Swap fields/teams
      case '*':         
        if (dispData.getState() == idle)
        {
          dispData.setTime(600); 
          int nScoreACopy = dispData.getScoreA();
          dispData.setScoreA(dispData.getScoreB());
          dispData.setScoreB(nScoreACopy);
           
          rgb_matrix::Color* pTeamAColorCopy = pTeamAColor;          
          pTeamAColor = pTeamBColor;
          pTeamBColor = pTeamAColorCopy;
          
          bUpdateDisplay = true;
        }     
        break;
      // Start/pause
      case 10: // return
     // case '0': 
        dispData.start_pause();    
        break;      
      case '+':
        dispData.modifyTime(60);
        break;
      case '-':
        dispData.modifyTime(-60);
        break;
      case 'q': 
        bExit = true; 
        break;
      // Stop
      case '/':
        dispData.stopTimer();
        nResetCnt++;
        if(nResetCnt > 40) // 2 seconds
        {
          dispData.resetScore();
          dispData.stopTimer();
          dispData.setTime(600);
          nResetCnt = 0;
        }      
        break; 
        
      // Change team colors
      case '5':
        dispData.nextColorIndexA();
        bUpdateDisplay = true;
        break;
      case '6':
        dispData.nextColorIndexB();
        bUpdateDisplay = true;
        break;
      default: 
        sBuf.push_back(nInput);  
        if(sBuf.size() > 5)
          sBuf.erase(0, 1);
          
         // alternative input when numlock is activated 
        if(rfind_str(sBuf, sIncScoreA))       dispData.incScoreA();
        else if(rfind_str(sBuf, sDecScoreA))  dispData.decScoreA();    
        else if(rfind_str(sBuf, sIncScoreB))  dispData.incScoreB();
        else if(rfind_str(sBuf, sDecScoreB))  dispData.decScoreB();   
        
        else if(rfind_str(sBuf, sColorChangeA)){
          dispData.nextColorIndexA();
          bUpdateDisplay = true;
        }
        else if(rfind_str(sBuf, sColorChangeB)){
          dispData.nextColorIndexB();
          bUpdateDisplay = true;
        }


        break;   
    }
   // printw("%2d %02d:%02d %2d\r", dispData.getScoreA(), dispData.getMin(), dispData.getSec(), dispData.getScoreB());          
    bUpdateDisplay = true;
  }
}

