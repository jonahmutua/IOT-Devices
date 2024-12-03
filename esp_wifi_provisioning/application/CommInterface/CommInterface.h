#ifndef COMMINTERFACE_H
#define COMMINTERFACE_H
#include "esp_system.h"

///> Communication Interface class 
class CommInterface
{
  public:
  //virtual esp_err_t init()  = 0;     
  virtual esp_err_t start() = 0;
  virtual esp_err_t stop()  = 0;
};

#endif