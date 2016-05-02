/********************
www.r-site.net
Nov. 2014 Rui Azevedo - ruihfazevedo(@rrob@)gmail.com
implementing menu fields as options that show a value
value (variable reference) can be changed by either using:
	menuField - for numeric varibles between range and optinally with a tune speed
	menuChoose - Use menu like navigation to select variable value
	menuToggle - cycle list of possible values
	
class menuValue is used as a menu prompt with an associated value for menuChoose and menuToggle

this classes are implemented as templates to accomodate virtually any value type

creative commons license 3.0: Attribution-ShareAlike CC BY-SA
This software is furnished "as is", without technical support, and with no 
warranty, express or implied, as to its usefulness for any purpose.

Thread Safe: No
Extensible: Yes

***/
#include <Value.h>
#include <EEPROMex.h>
#include <ValueEeprom.h>
#include <ValueProgmemString.h>

#ifndef RSITE_ARDUINOP_MENU_FIELDS
#define RSITE_ARDUINOP_MENU_FIELDS

	#include "menu.h"

//prompts holding values for choose and toggle
//TODO: 'action' is not needed here, we could implement it but its kinda redundant
//	either implement it or remove it (by forking class derivation earlier)
//	not that we dont need a function to be called on variable change,
//	but this function MUST be defined on menu levell, not on setting level
// rah Nov.2014
template<typename T>
class menuValue:public prompt {
  public:
	  T value;
    inline menuValue(ValueBase* label, T value):prompt(label), value(value) {}
    
};
  
//Prompt linked to a variable
//TODO: implement Escape on a field cancels the editting (undo) restoring the value
static const char* numericChars="0123456789.";
  
template <typename T>
class menuField:public menuNode {
	public:
		Value<T>* setting;
		ValueBase* units;
		T low, high, step, tune;
		bool tunning;
		char ch;
		T tmp;

		menuField(Value<T> * setting, ValueBase* label, ValueBase* units, T low, T high, T step, T tune=0)
			:menuNode(label), setting(setting), units(units), low(low), high(high), step(step), tune(tune), tunning(false), ch(0), tmp(0) {}
      
		virtual void printTo(menuOut& p) {
      char buff[16];

      char labelBuffer[21];
      label->getValueString(labelBuffer);
      p.print(labelBuffer);
			//p.print(text);

      
			p.print(activeNode==this?(tunning?'>':':'):' ');

      setting->getValueString(buff);
			p.print(buff);
      
			units->getValueString(buff);
      p.print(buff);
		}
    
		void clamp() {
      if (setting->getValue() < low) setting->setValue(low);
      else if (setting->getValue() > high) setting->setValue(high);
		}
    
		//lazy drawing, we have no drawing position here... so we will ask the menu to redraw
		virtual void activate(menuOut& p, Stream&c, bool canExit=false) {
			if (activeNode!=this) {
			  ox=activeNode->ox;
			  oy=activeNode->oy;
				previousMenu=(menu*)activeNode;
				activeNode=this;
      	p.lastSel=-1;
      	previousMenu->printMenu(p, previousMenu->canExit);
			}
			if (!c.available()) return;
			if (strchr(numericChars, c.peek())) {//a numeric value was entered
      	setting->setValue(c.parseFloat());
    		tunning=false;
    		activeNode=previousMenu;
    		c.flush();
    		ch=menu::enterCode;
      } else {
			  ch=c.read();
		    if (ch==menu::enterCode) {
		    	if (tunning||!tune) {//then exit edition
		    		tunning=false;
		    		activeNode=previousMenu;
		    		c.flush();
            setting->persistValue();
		    	} else tunning=true;
		    } else if (ch=='+') {
          if ( setting->getValue() + (tunning?tune:step) > setting->getValue() ) {
            // allow for wrap-around
            setting->setValue(setting->getValue() + (tunning?tune:step));
          }
        }
		    else if (ch=='-') {
          if ((setting->getValue() - (tunning?tune:step)) < setting->getValue()) {
            // allow for wrap-around
            setting->setValue(setting->getValue() - (tunning?tune:step));
          }
        }
		    else if (ch==menu::escCode) {
          setting->revertValue();
          tunning=false;
          activeNode=previousMenu;
          c.flush();
          p.lastSel=-1;
        }
		  }
      clamp();
      if (setting->hasChanged() || ch==menu::enterCode) {
      	p.lastSel=-1;
      	previousMenu->printMenu(p, previousMenu->canExit);
      }
		}
    
};
	
template<typename T>
class menuSelect : public menu {
  public:
		Value<T> * setting;
		
    menuSelect(Value<T> * setting, ValueBase* label, byte sz, menuValue<T>* const data[]):
	    menu(label, sz, (prompt**)data), setting(setting) {sync();}
      
    void sync() {//if possible make selection match the target value
			sel=0;
	  	for(byte n=0;n<sz;n++)
	  		if (((menuValue<T>*)data[n])->value==setting->getValue())
	  			sel=n;
    }	
		
    virtual void printTo(menuOut& p) {
			menuSelect<T>::sync();
      char labelBuffer[21];
      label->getValueString(labelBuffer);
      p.print(labelBuffer);
      
      ((menuValue<T>*)menu::data[menu::sel])->label->getValueString(labelBuffer);
      p.print(labelBuffer);
		}
};

template<typename T>
class menuChoice : public menuSelect<T> { 
  public:
		menuChoice(ValueBase* label, byte sz, menuValue<T>* const data[], Value<T> * setting):
	    menuSelect<T>(setting, label, sz, data) {menuSelect<T>::sync();}
      
		void activate(menuOut& p, Stream& c, bool canExit) {
			if (menu::activeNode!=this) {
			  this->setPosition(menuNode::activeNode->ox, menuNode::activeNode->oy);
				this->menu::previousMenu=(menu*)menu::activeNode;
				menu::activeNode=this;
			 	this->canExit=canExit;
			}
			byte op=-1;
			menu::printMenu(p, false);
			op=menu::menuKeys(p, c, canExit);
			if (op>=0 && op<this->menu::sz) {
				this->menu::sel=op;
				if (this->menu::data[op]->enabled) {
					this->menuSelect<T>::setting->setValue(((menuValue<T>*)this->menu::data[op])->value);
					this->menu::data[op]->activate(p, c, true);
					//and exit
					this->menu::activeNode=this->menu::previousMenu;
				 	c.flush();//reset the encoder
          this->menuSelect<T>::setting->persistValue();
				}
			}
      else if (op == -1) {
        this->menu::activeNode=this->menu::previousMenu;
        this->menuSelect<T>::setting->revertValue();
      }
		}
};

template<typename T>
class menuToggle : public menuSelect<T> {
  public:
		menuToggle(ValueBase* label, byte sz, menuValue<T>* const data[], Value<T> * setting):
	    menuSelect<T>(setting, label, sz, data) {menuSelect<T>::sync();}      
      
		void activate(menuOut& p, Stream& c, bool canExit) {
			this->menu::sel++;
			if (this->menu::sel>=this->menu::sz) this->menu::sel=0;
		 	p.lastSel=-1;//redraw only affected option
			this->menuSelect<T>::setting->setValue(((menuValue<T>*)this->menu::data[menu::sel])->value);
      this->menuSelect<T>::setting->persistValue();  // persist immediately (grrr)
			this->menu::data[this->menu::sel]->activate(p, c, true);
		}
};

class menuPage : public menu { 
  public:
    menuPage(ValueBase* label, byte sz, prompt* const data[]) : 
      menu(label, sz, data) {}
      
    virtual void onEnter(menuOut& p, Stream& c, bool canExit) {
      this->setPosition(menuNode::activeNode->ox, menuNode::activeNode->oy);
      this->menu::previousMenu=(menu*)menu::activeNode;
      menu::activeNode=this;
      this->canExit=canExit;
    }
    
    virtual bool update(menuOut& p, Stream& c, bool canExit) {
      menu::printMenu(p, false);
    }
    
    virtual bool shouldExit(menuOut& p, Stream& c, bool canExit) {
      return true;
    }
    
    virtual void onExit(menuOut& p, Stream& c, bool canExit) {
      this->menu::activeNode=this->menu::previousMenu;
    }
    
		void activate(menuOut& p, Stream& c, bool canExit) {
			if (menu::activeNode!=this) {
        onEnter(p, c, canExit);
			}
      
      update(p, c, canExit);
      
      if (shouldExit(p, c, canExit)) {
        onExit(p, c, canExit);
      }
		}
};

class timeoutPage : public menuPage { 
  protected:
    Timer* _timer;
    unsigned long _timerDelay = 1000;
  
  public:
    timeoutPage(ValueBase* label, byte sz, prompt* const data[],  unsigned long timerDelay) : _timerDelay(timerDelay), menuPage(label, sz, data) {
      _timer = new Timer();
    }
    ~timeoutPage() {
      delete _timer;
    }

    void onEnter(menuOut& p, Stream& c, bool canExit) override {
      menuPage::onEnter(p, c, canExit);
      
      _timer->start(_timerDelay);
    }

    bool shouldExit(menuOut& p, Stream& c, bool canExit) override {
      bool exitRequested = false;

      int8_t op = menuKeys(p, c, canExit);
      
      if (op==-1) {//then exit
        exitRequested = true;
      }
    
      return (exitRequested || _timer->isComplete());
    }

};

class messagePrompt : public prompt {//some basic information for menus and fields
  public:
    inline messagePrompt(ValueBase* label):prompt(label, false) {}
};

class displayFloatValue : public prompt {
  protected:
    float* _valueP;

  public:
    displayFloatValue(ValueBase* label, float* valueP) : _valueP(valueP), prompt(label, false) {}
    
    virtual void printTo(menuOut& p) {
      char labelBuffer[21];
      label->getValueString(labelBuffer);
      p.print(labelBuffer);

      p.print(*_valueP);
    }

};

template<typename T>
class displayValue : public prompt {
  protected:
    Value<T>* _valueP;

  public:
    displayValue(ValueBase* label, Value<T>* valueP) : _valueP(valueP), prompt(label, false) {}
    
    virtual void printTo(menuOut& p) {
      char labelBuffer[21];
      label->getValueString(labelBuffer);
      p.print(labelBuffer);

      char buff[10];
      _valueP->getValueString((char*) &buff);
      p.print(buff);
    }
};

/*
template<>
void displayValue<bool>::printTo(menuOut& p) {
  char buff[2];

  
  char labelBuffer[21];
  label->getValueString(labelBuffer);
  p.print(labelBuffer);
  //p.print(text);
  
  if (_valueP->getValue() == true) {
    strcpy(buff, "!");
  }
  else {
    strcpy(buff, " ");
  }
  p.print(buff);

}
*/

class displayText : public prompt {
  public:
    displayText(ValueBase* label) : prompt(label, false) {}
    
    virtual void printTo(menuOut& p) {
      char labelBuffer[21];
      label->getValueString(labelBuffer);
      p.print(labelBuffer);
      //p.print(text);
    }
};

const char strBlank[] PROGMEM = "";
ValueProgmemString labelBlank = ValueProgmemString(strBlank);

class displayValueSet : public prompt {
  protected:
    const byte _sz;
    prompt* const* _data;

  public:
    displayValueSet(byte sz, prompt* const data[]) : _sz(sz), _data(data), prompt(&labelBlank, false) {}
    
    virtual void printTo(menuOut& p) {
      for (byte i=0; i<_sz; i++) {
        _data[i]->printTo(p);
      }
    }
};


class statusPage : public menuPage { 
  protected:
    Timer* _timer;
    static const byte refreshDelay = 250;
  
  public:

  statusPage(ValueBase* label, byte sz, prompt* const data[]) : menuPage(label, sz, data) {
    _timer = new Timer();
  }
  
  ~statusPage() {
    delete _timer;
  }
  
  void onEnter(menuOut& p, Stream& c, bool canExit) override {
    menuPage::onEnter(p, c, canExit);
  }

  void activate(menuOut& p, Stream&c, bool canExit=false)  {
    if (_timer->isComplete()) {
      p.drawn=0;  // force refresh
      _timer->start(refreshDelay);
    }
    
    menuPage::activate(p, c, canExit);
    
    byte op = menu::menuKeys(p, c, canExit);
    if (op == -1) {
      this->menu::activeNode=this->menu::previousMenu;
    }
  }
  
  bool shouldExit(menuOut& p, Stream& c, bool canExit) override {
    bool _shouldExit = (c.peek()==menu::escCode);
    
    if (_shouldExit) {
      c.read();
    }
    
    return _shouldExit;
  }
  
};
  
#endif