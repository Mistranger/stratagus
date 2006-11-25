/*      _______   __   __   __   ______   __   __   _______   __   __                 
 *     / _____/\ / /\ / /\ / /\ / ____/\ / /\ / /\ / ___  /\ /  |\/ /\                
 *    / /\____\// / // / // / // /\___\// /_// / // /\_/ / // , |/ / /                 
 *   / / /__   / / // / // / // / /    / ___  / // ___  / // /| ' / /                  
 *  / /_// /\ / /_// / // / // /_/_   / / // / // /\_/ / // / |  / /                   
 * /______/ //______/ //_/ //_____/\ /_/ //_/ //_/ //_/ //_/ /|_/ /                    
 * \______\/ \______\/ \_\/ \_____\/ \_\/ \_\/ \_\/ \_\/ \_\/ \_\/                      
 *
 * Copyright (c) 2004, 2005 darkbits                        Js_./
 * Per Larsson a.k.a finalman                          _RqZ{a<^_aa
 * Olof Naess�n a.k.a jansem/yakslem                _asww7!uY`>  )\a//
 *                                                 _Qhm`] _f "'c  1!5m
 * Visit: http://guichan.darkbits.org             )Qk<P ` _: :+' .'  "{[
 *                                               .)j(] .d_/ '-(  P .   S
 * License: (BSD)                                <Td/Z <fP"5(\"??"\a.  .L
 * Redistribution and use in source and          _dV>ws?a-?'      ._/L  #'
 * binary forms, with or without                 )4d[#7r, .   '     )d`)[
 * modification, are permitted provided         _Q-5'5W..j/?'   -?!\)cam'
 * that the following conditions are met:       j<<WP+k/);.        _W=j f
 * 1. Redistributions of source code must       .$%w\/]Q  . ."'  .  mj$
 *    retain the above copyright notice,        ]E.pYY(Q]>.   a     J@\
 *    this list of conditions and the           j(]1u<sE"L,. .   ./^ ]{a
 *    following disclaimer.                     4'_uomm\.  )L);-4     (3=
 * 2. Redistributions in binary form must        )_]X{Z('a_"a7'<a"a,  ]"[
 *    reproduce the above copyright notice,       #}<]m7`Za??4,P-"'7. ).m
 *    this list of conditions and the            ]d2e)Q(<Q(  ?94   b-  LQ/
 *    following disclaimer in the                <B!</]C)d_, '(<' .f. =C+m
 *    documentation and/or other materials      .Z!=J ]e []('-4f _ ) -.)m]'
 *    provided with the distribution.          .w[5]' _[ /.)_-"+?   _/ <W"
 * 3. Neither the name of Guichan nor the      :$we` _! + _/ .        j?
 *    names of its contributors may be used     =3)= _f  (_yQmWW$#(    "
 *    to endorse or promote products derived     -   W,  sQQQQmZQ#Wwa]..
 *    from this software without specific        (js, \[QQW$QWW#?!V"".
 *    prior written permission.                    ]y:.<\..          .
 *                                                 -]n w/ '         [.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT       )/ )/           !
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY         <  (; sac    ,    '
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING,               ]^ .-  %
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF            c <   r
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR            aga<  <La
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE          5%  )P'-3L
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR        _bQf` y`..)a
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          ,J?4P'.P"_(\?d'.,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES               _Pa,)!f/<[]/  ?"
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT      _2-..:. .r+_,.. .
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     ?a.<%"'  " -'.a_ _,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION)                     ^
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * For comments regarding functions please see the header file. 
 */

#include "guichan/keyinput.h"
#include "guichan/mouseinput.h"
#include "guichan/widgets/textfield.h"
#include "guichan/exception.h"

static int GetClipboard(std::string &str);

namespace gcn
{
    TextField::TextField()
    {
        mCaretPosition = 0;
        mXScroll = 0;

        setFocusable(true);

        addMouseListener(this);
        addKeyListener(this);
        adjustHeight();
        setBorderSize(1);    
    }
  
    TextField::TextField(const std::string& text)
    {
        mCaretPosition = 0;
        mXScroll = 0;
    
        mText = text;
        adjustSize();
        setBorderSize(1);
        
        setFocusable(true);    
  
        addMouseListener(this);
        addKeyListener(this);    
    }

    void TextField::setText(const std::string& text)
    {
        if(text.size() < mCaretPosition )
        {
            mCaretPosition = text.size();
        }
    
        mText = text;    
    }
  
    void TextField::draw(Graphics* graphics)
    {
        Color faceColor = getBackgroundColor();
        graphics->setColor(faceColor);
        graphics->fillRectangle(Rectangle(0, 0, getWidth(), getHeight()));
    
        if (hasFocus())
        {      
            drawCaret(graphics, getFont()->getWidth(mText.substr(0, mCaretPosition)) - mXScroll);
        }
    
        graphics->setColor(getForegroundColor());
        graphics->setFont(getFont());
        graphics->drawText(mText, 1 - mXScroll, 1);    
    }

    void TextField::drawBorder(Graphics* graphics)
    {
        Color faceColor = getBaseColor();
        Color highlightColor, shadowColor;
        int alpha = getBaseColor().a;
        int width = getWidth() + getBorderSize() * 2 - 1;
        int height = getHeight() + getBorderSize() * 2 - 1;
        highlightColor = faceColor + 0x303030;
        highlightColor.a = alpha;
        shadowColor = faceColor - 0x303030;
        shadowColor.a = alpha;
        
        unsigned int i;
        for (i = 0; i < getBorderSize(); ++i)
        {
            graphics->setColor(shadowColor);
            graphics->drawLine(i,i, width - i, i);
            graphics->drawLine(i,i + 1, i, height - i - 1);
            graphics->setColor(highlightColor);
            graphics->drawLine(width - i,i + 1, width - i, height - i); 
            graphics->drawLine(i,height - i, width - i - 1, height - i);
        }
    }
    
    void TextField::drawCaret(Graphics* graphics, int x)
    {
        graphics->setColor(getForegroundColor());
        graphics->drawLine(x, getHeight() - 2, x, 1);    
    }
  
    void TextField::mousePress(int x, int y, int button)
    {
        if (hasMouse() && button == MouseInput::LEFT)
        {
            mCaretPosition = getFont()->getStringIndexAt(mText, x + mXScroll);
            fixScroll();
        }      
    }

	static int GetPrev(std::string text, int curpos)
	{
		--curpos;
		while (curpos >= 0) {
			if ((text[curpos] & 0xC0) != 0x80) {
				return curpos;
			}
			--curpos;
		}
		if (curpos < 0) {
			throw GCN_EXCEPTION("Invalid UTF8.");
		}
		return 0;
	}

	static int GetNext(std::string text, int curpos)
	{
		char c = text[curpos];
		if (!(c & 0x80)) {
			return curpos + 1;
		}
		if ((c & 0xE0) == 0xC0) {
			return curpos + 2;
		}
		if ((c & 0xF0) == 0xE0) {
			return curpos + 3;
		}
		throw GCN_EXCEPTION("Invalid UTF8.");
		return 0;
	}
  
    bool TextField::keyPress(const Key& key)
    {
        bool ret = false;

        if (key.getValue() == Key::LEFT && mCaretPosition > 0)
        {
            mCaretPosition = GetPrev(mText, mCaretPosition);
            ret = true;
        }

        else if (key.getValue() == Key::RIGHT && mCaretPosition < mText.size())
        {
            mCaretPosition = GetNext(mText, mCaretPosition);
            ret = true;
        }

        else if (key.getValue() == Key::DELETE && mCaretPosition < mText.size())
        {
			int newpos = GetNext(mText, mCaretPosition);
            mText.erase(mCaretPosition, newpos - mCaretPosition);
            ret = true;
        }

        else if ((key.getValue() == Key::BACKSPACE || key.getValue() == 'h' - 'a' + 1) &&
            mCaretPosition > 0)
        {
            int newpos = GetPrev(mText, mCaretPosition);
            mText.erase(newpos, mCaretPosition - newpos);
			mCaretPosition = newpos;
            ret = true;
        }

        else if (key.getValue() == Key::ENTER)
        {
            generateAction();
            ret = true;
        }

        else if (key.getValue() == Key::HOME || key.getValue() == 'a' - 'a' + 1) // ctrl-a
        {
            mCaretPosition = 0;
            ret = true;
        }    

        else if (key.getValue() == Key::END || key.getValue() == 'e' - 'a' + 1)  //ctrl-e
        {
            mCaretPosition = mText.size();
            ret = true;
        }    

        else if (key.getValue() == 'u' - 'a' + 1) // ctrl-u
        {
            setText("");
            ret = true;
        }

        else if (key.getValue() == 'v' - 'a' + 1) // ctrl-v
        {
            std::string str;
            if (GetClipboard(str) >= 0) {
                for (size_t i = 0; i < str.size(); ++i) {
                    keyPress(Key(str[i]));
                }
                ret = true;
            }
        }

        else if (key.isCharacter())
        {
            mText.insert(mCaretPosition,key.toString());
            mCaretPosition = GetNext(mText, mCaretPosition);
            ret = true;
        }

        fixScroll();
        return ret;
    }

    void TextField::adjustSize()
    {
        setWidth(getFont()->getWidth(mText) + 4);
        adjustHeight();

        fixScroll();    
    }
  
    void TextField::adjustHeight()
    {
        setHeight(getFont()->getHeight() + 2);    
    }

    void TextField::fixScroll()
    {
        if (hasFocus())
        {
            int caretX = getFont()->getWidth(mText.substr(0, mCaretPosition));

            if (caretX - mXScroll > getWidth() - 4)
            {
                mXScroll = caretX - getWidth() + 4;
            }
            else if (caretX - mXScroll < getFont()->getWidth(" "))
            {
                mXScroll = caretX - getFont()->getWidth(" ");
        
                if (mXScroll < 0)
                {
                    mXScroll = 0;
                }
            }
        }
    }

    void TextField::setCaretPosition(unsigned int position)
    {
        if (position > mText.size())
        {
            mCaretPosition = mText.size();
        }
        else
        {    
            mCaretPosition = position;
        }

        fixScroll();    
    }

    unsigned int TextField::getCaretPosition() const
    {
        return mCaretPosition;    
    }

    const std::string& TextField::getText() const
    {
        return mText;    
    }
  
    void TextField::fontChanged()
    {
        fixScroll();
    }
}

#ifdef USE_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(_XLIB_H_)
#include <X11/Xlib.h>
#endif

/**
** Paste text from the clipboard
*/
static int GetClipboard(std::string &str)
{
#if defined(USE_WIN32) || defined(_XLIB_H_)
	int i;
	unsigned char *clipboard;
#ifdef USE_WIN32
	HGLOBAL handle;
#elif defined(_XLIB_H_)
	Display *display;
	Window window;
	Atom rettype;
	unsigned long nitem;
	unsigned long dummy;
	int retform;
	XEvent event;
#endif

#ifdef USE_WIN32
	if (!IsClipboardFormatAvailable(CF_TEXT) || !OpenClipboard(NULL)) {
		return -1;
	}
	handle = GetClipboardData(CF_TEXT);
	if (!handle) {
		CloseClipboard();
		return -1;
	}
	clipboard = (unsigned char *)GlobalLock(handle);
	if (!clipboard) {
		CloseClipboard();
		return -1;
	}
#elif defined(_XLIB_H_)
	if (!(display = XOpenDisplay(NULL))) {
		return -1;
	}

	// Creates a non maped temporary X window to hold the selection
	if (!(window = XCreateSimpleWindow(display,
			DefaultRootWindow(display), 0, 0, 1, 1, 0, 0, 0))) {
		XCloseDisplay(display);
		return -1;
	}

	XConvertSelection(display, XA_PRIMARY, XA_STRING, XA_STRING,
		window, CurrentTime);

	XNextEvent(display, &event);

	if (event.type != SelectionNotify ||
			event.xselection.property != XA_STRING) {
		return -1;
	}

	XGetWindowProperty(display, window, XA_STRING, 0, 1024, False,
		XA_STRING, &rettype, &retform, &nitem, &dummy, &clipboard);

	XDestroyWindow(display, window);
	XCloseDisplay(display);

	if (rettype != XA_STRING || retform != 8) {
		if (clipboard != NULL) {
			XFree(clipboard);
		}
		clipboard = NULL;
	}

	if (clipboard == NULL) {
		return -1;
	}
#endif
	// Only allow ascii characters
	for (i = 0; clipboard[i] != '\0'; ++i) {
		if (clipboard[i] < 32 || clipboard[i] > 126) {
			return -1;
		}
	}
	str = (char *)clipboard;
#ifdef USE_WIN32
	GlobalUnlock(handle);
	CloseClipboard();
#elif defined(_XLIB_H_)
	if (clipboard != NULL) {
		XFree(clipboard);
	}
#endif
	return 0;
#else
	return -1;
#endif
}

