/********************
Sept. 2014 Rui Azevedo - ruihfazevedo(@rrob@)gmail.com
creative commons license 3.0: Attribution-ShareAlike CC BY-SA
This software is furnished "as is", without technical support, and with no 
warranty, express or implied, as to its usefulness for any purpose.

Thread Safe: No
Extensible: Yes

Arduino generic menu system
www.r-site.net
*/
#include <Arduino.h>
#include "menu.h"

//#define DEBUG_MENU

//char prompt::labelBuffer[21];


const char* menu::exit="Exit";
char menu::escCode='/';
char menu::enterCode='*';
char menu::upCode='+';
char menu::downCode='-';
char menu::enabledCursor='>';
char menu::disabledCursor='-';

const char strExit[] PROGMEM = "Exit";
ValueProgmemString labelExit = ValueProgmemString(strExit);

prompt menu::exitOption(&labelExit);
//prompt menu::exitOption(menu::exit);

menuNode* menuNode::activeNode=NULL;


bool prompt::shouldBeEnabled() { 
  return true; 
}


bool menuOut::needRedraw(menu& m, int8_t i) { 
  return (drawn!=&m) || (top!=lastTop) || ( m.sel!=lastSel && ( (i==m.sel) || (i==lastSel) ) ); 
}

//menu navigation engine
//iteract with input until a selection is done, 
//  return the (0 indexed) number of the selection
//  OR -2 if navigating up/dowm (ignore)
//  OR -1 to exit

int8_t menu::menuKeys(menuOut &p, Stream& c, bool canExit) {
  int8_t op=-2;
  if (!c.available()) return op;//only work when stream data is available

  if (c.peek() >= 0) {
#ifdef DEBUG_MENU
    Serial.print(F("key: "));
    Serial.println(c.peek());
#endif
  
    if (c.peek()!=menu::enterCode) {
      int8_t ch=c.read();
      
#ifdef DEBUG_MENU
      Serial.print(F("ch is "));
      Serial.println(ch);
#endif
      
      if ((ch==menu::upCode) && (sel>0)) {
        sel--;
      } 
      //else if ((ch==menu::downCode) && (sel<(sz-(canExit?0:1))) )
      else if ((ch==menu::downCode) && (sel<(sz-1)) )
      {
        sel++;
      } 
      else if (ch==menu::escCode) {
        op=-1;
#ifdef DEBUG_MENU
        Serial.print(F("menuKeys escCode detected, op set to "));
        Serial.println(op);
#endif
      } 
      else {
        op=ch-'1';
      }
    } 
    else {
      op = (sel==sz) ? -1 : sel;
      
#ifdef DEBUG_MENU
        Serial.print(F("menuKeys enterCode detected, sel is "));
        Serial.print(sel);
        Serial.print(F(", op set to "));
        Serial.println(op);
#endif
    }
    
#ifdef DEBUG_MENU
    Serial.print(F("menuKeys op is: "));
    Serial.println(op);
#endif
      
  }

  if (!((op>=0 && op<sz) || (canExit && op==-1))) op=-2;//validate the option
  
  //add some delays to be sure we do not have more characters NL or CR on the way
  //delay might be adjusted to cope with stream speed
  //TODO: guess we dont need this.. check it out
  delay(50);while (c.peek()==menu::enterCode/* || c.peek()==10*/) {c.read();delay(50);}//discard ENTER and CR
  return op;
}



void menu::onEnter(menuOut& p, Stream& c, bool canExit) { 
#ifdef DEBUG_MENU
  Serial.println(F("activated menu"));
#endif
  previousMenu=(menu*)activeNode;
  activeNode=this;
  sel=0;
  p.top=0;
  this->canExit=canExit;
  
  int8_t i=0;
  for(i=0;i<sz;i++) {
    data[i]->enabled = data[i]->shouldBeEnabled();
  }
  
}

bool menu::update(menuOut& p, Stream& c, bool canExit) {
  printMenu(p, false);
}

bool menu::shouldExit(menuOut& p, Stream& c, bool canExit) {
  //return true;
  return false;
}

void menu::onExit(menuOut& p, Stream& c, bool canExit) {
}

void menu::onExitUp(menuOut& p, Stream& c, bool canExit) {
  onExit(p, c, canExit);
  
#ifdef DEBUG_MENU
  Serial.println(F("ascending to parent"));
#endif

  p.clear();
  activeNode=previousMenu;
  c.flush();//reset the encoder
}

void menu::onExitDown(menuOut& p, Stream& c, bool canExit, int8_t op) {
  onExit(p, c, canExit);
  
#ifdef DEBUG_MENU
  Serial.println(F("descending to child"));
#endif
  printMenu(p, canExit);//clearing old selection
  data[op]->activate(p, c, true);

}


//execute the menu
//cycle:
//	...->draw -> input scan -> iterations -> [activate submenu or user function] -> ...
// draw: call target device object
//input scan: call the navigation function (self)
void menu::activate(menuOut& p, Stream& c, bool canExit) {
  if (_timer->isComplete()) {
#ifdef DEBUG_MENU
    Serial.println(F("  requesting redraw"));
#endif
    p.drawn=0;  // force refresh
    _timer->start(refreshDelay);
  }

  if (activeNode!=this) {
    onEnter(p, c, canExit);
  }  
  
  update(p, c, canExit);
  
  int8_t op=-1;

  if (forceExit) {
    forceExit = false;
    op=-1;
  }  
  else {
    op=menuKeys(p, c, canExit);
  }
  
#ifdef DEBUG_MENU
  if (op > -2) {
    Serial.print(F("op is: "));
    Serial.println(op);
  }
#endif

  if (shouldExit(p, c, canExit)) {
    op=-1;
  }
  
  if (op>=0 && op<sz) {
  	sel=op;
    if (data[op]->enabled) {
      onExitDown(p, c, canExit, op);
    }
  } 
  else if (op==-1) {//then exit
    // ascend to parent
    onExitUp(p, c, canExit);
  }
}

void menu::poll(menuOut& p, Stream& c, bool canExit) {
	if (!activeNode) activeNode=this;
  
	activeNode->activate(p, c, activeNode==this?canExit:true);
}


