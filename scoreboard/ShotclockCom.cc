#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <iostream>
#include <string>
#include "DisplayData.hh"
#include "socket.hh"
#include <sstream>


void ShotclockCom1 (DisplayData& dispData)
{
  Socket csocket(true);

 //Create socket
  while(1){
    if(csocket.SocketCreate() == -1)
    {
      printf("Could not create socket\n");
      sleep(5);
    } else{
      printf("Socket is created\n");
      break;
    }
  }

  //Bind
  while(1){
    if(csocket.BindCreatedSocket(9000) < 0)
    {
      printf("bind failed");
      sleep(5);
    } else {
      printf("bind done\n");
      break;
    }
  }

  //Listen
  csocket.Listen();
  char tx_string[10];
  int retVal;

  while(1)
  {
      printf("Waiting for incoming connections...\n");
      //Accept incoming connection
      if (csocket.Accept() < 0)
      {
          perror("accept failed");
          break;
      }
      printf("Connection accepted\n");

      while(1){
        //if(dispData.NeedShotclockRefresh()){
        sprintf(tx_string, "%02d", dispData.getShotTimeout());
        printf("send %s\n", tx_string);

        if((retVal = csocket.SocketSend(std::string(tx_string))) < 0)
        {
          printf("send failed (error %d)\n", retVal);
          break;
        }
        usleep(250000);
      }
  }

}

void ShotclockCom2 (DisplayData& dispData)
{
  Socket csocket(true);

 //Create socket
  while(1){
    if(csocket.SocketCreate() == -1)
    {
      printf("[2] Could not create socket\n");
      sleep(5);
    } else{
      printf("[2] Socket is created\n");
      break;
    }
  }

  //Bind
  while(1){
    if(csocket.BindCreatedSocket(9001) < 0)
    {
      printf("[2] bind failed");
      sleep(5);
    } else {
      printf("[2] bind done\n");
      break;
    }
  }

  //Listen
  csocket.Listen();
  char tx_string[10];
  int retVal;

  while(1)
  {
      printf("[2] Waiting for incoming connections...\n");
      //Accept incoming connection
      if (csocket.Accept() < 0)
      {
          perror("[2] accept failed");
          break;
      }
      printf("[2] Connection accepted\n");

      while(1){
        //if(dispData.NeedShotclockRefresh()){
        sprintf(tx_string, "%02d", dispData.getShotTimeout());
        printf("[2] send %s\n", tx_string);

        if((retVal = csocket.SocketSend(std::string(tx_string))) < 0)
        {
          printf("[2] send failed (error %d)\n", retVal);
          break;
        }
        usleep(250000);
      }
  }

}
