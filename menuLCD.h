/********************
Sept. 2014 Rui Azevedo - ruihfazevedo(@rrob@)gmail.com
creative commons license 3.0: Attribution-ShareAlike CC BY-SA
This software is furnished "as is", without technical support, and with no 
warranty, express or implied, as to its usefulness for any purpose.

Thread Safe: No
Extensible: Yes

Use standard arduino LCD (LiquidCrystal library) as menu output

www.r-site.net
***/

#ifndef RSITE_ARDUINOP_MENU_LCD
	#define RSITE_ARDUINOP_MENU_LCD
  #include <LiquidCrystal.h>
	#include "menu.h"

class menuLCD:public menuOut {
  public:
    LiquidCrystal& lcd;
    menuLCD(LiquidCrystal& lcd,int8_t x=16,int8_t y=1):lcd(lcd),menuOut(x,y) {}
    
    virtual void clearLine(int8_t ln) {
    	setCursor(0,ln);
    	for(int8_t n=0;n<maxX;n++) 
        print(' ');
      
    	setCursor(0,ln);
    }
    
    virtual void clear() {lcd.clear();}
    
    virtual void setCursor(int8_t x,int8_t y) {lcd.setCursor(x*resX,y*resY);}
    
    virtual size_t write(uint8_t ch) {return lcd.write(ch);}
    
		virtual void printPrompt(prompt &o,bool selected,int8_t idx,int8_t posY,int8_t width) {
			clearLine(posY);
      
			print(selected ? (o.enabled ? menu::enabledCursor : menu::disabledCursor) : ' ');
			o.printTo(*this);
		}
    
		virtual void printMenu(menu& m,bool drawExit) {
    
			if (drawn!=&m) clear();//clear screen when changing menu

      top = 0;
      if (m.sel >= (top + maxY)) {
        top = m.sel - maxY + 1;
      }
    
      int8_t i=0;
      for(i=0;i<m.sz;i++) {
				if ((i>=top)&&((i-top)<maxY)) {
				  if (needRedraw(m,i)) {
				  	printPrompt(*m.data[i],i==m.sel,i+1,i-top,m.width);
				  }
				}
      }
      
			if (drawExit&&i-top<maxY&&needRedraw(m,i)) {
				printPrompt(menu::exitOption,m.sel==m.sz,0,i-top,m.width);
      }
      
			lastTop=top;
			lastSel=m.sel;
			drawn=&m;
		}
};
#endif //RSITE_ARDUINOP_MENU_LCD