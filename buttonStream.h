/**************
July. 2015 Jon van Noort 
creative commons license 3.0: Attribution-ShareAlike CC BY-SA
This software is furnished "as is", without technical support, and with no 
warranty, express or implied, as to its usefulness for any purpose.

based on work by Rui Azevedo - ruihfazevedo(@rrob@)gmail.com

***/

#include <Button.h>

struct buttonMap {
  Button * button;
  int8_t code;
};

//emulate a stream keyboard, this is not using interrupts as a good driver should do
// AND is not using a buffer either!
template <int N>
class buttonLook:public Stream {
protected:
  buttonMap * buttons;
  Timer timer;
  
  int state = unPressedState;

public:
  buttonLook<N>(buttonMap k[]):buttons(k) {}
  
  static const int unPressedState = 0;
  static const int delayNextPressState = 1;
  static const int delayAfterPressState = 2;
  
  const int pressDelay = 500;
  const int afterPressDelay = 500;
  
  int available(void) {
    if (timer.isComplete()) {
      return (anyKeyPressed()?1:0);
    }
    return 0;
  }

  int anyKeyPressed() {
    boolean ret=false;
    for(int n=0;n<N;n++) {
      if (buttons[n].button->isPressed()) {
        ret = true;
        break;
      }
    }
    return ret;
  }

  
  int read() {
    int buttonCode = -1;
    
    switch(state) {
      
      case unPressedState:
        buttonCode = peek();
        
        if (buttonCode > -1) {
          state = delayNextPressState;
          timer.start(pressDelay);
        }
        break;

      case delayNextPressState:
        if (timer.isComplete()) {
          state = unPressedState;
        }
        else if (anyKeyPressed()) {
          state = delayAfterPressState;
          timer.start(afterPressDelay);
        }
        break;

      case delayAfterPressState:
        if (timer.isComplete()) {
          state = unPressedState;
        }
        break;
      
      default:
        break;
      
    }
    
    return buttonCode;
  }
  

  int peek(void) {
    for(int n=0;n<N;n++) {
      if (buttons[n].button->isPressed()) {
        return buttons[n].code; 
      }
    }

    return -1;
  }

  void flush() {}

  size_t write(uint8_t v) {return 0;}
};


