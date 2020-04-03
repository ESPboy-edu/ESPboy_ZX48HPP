#ifndef __ESPBOY_UI_OBJ__
#define __ESPBOY_UI_OBJ__

#include "ESPBoy_UIKeys.hpp"
#include "strconst.hpp"

#include <map>

namespace ESPboy_UI
{
	class Window;

	typedef int32_t Color;

	constexpr uint_fast8_t line4to5[] PROGMEM = { 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 3, 3 };
	constexpr uint_fast8_t line5to4[] PROGMEM = { 0, 3, 7, 10, 0, 0, 0, 0, 0, 0, 0, 0 };

	class Context
	{
		std::map< str_const::HashType, Window*> m_windows;
		std::map< str_const::HashType, Window*>::iterator m_current;

	public:

		Context() : m_current(m_windows.end()) {}

		virtual uint_fast16_t getCharWidth() = 0;
		virtual uint_fast16_t getCharHeght() = 0;


		//  0 1 2
		//  3 4 5  <-- Text centre point (datum)
		//  6 7 8 
		virtual void setTextDatum(int_fast8_t datum) {};
		virtual void drawText(String &str, uint_fast16_t x, uint_fast16_t y, Color color = -1, Color bgcolor = -1)
		{
			drawText(str.c_str(), x, y, color, bgcolor);
		}
		virtual void drawText(char const *str, uint_fast16_t x, uint_fast16_t y, Color color = -1, Color bgcolor = -1) = 0;
		virtual void drawPixel(uint_fast16_t x, uint_fast16_t y, Color &color) = 0;
		virtual void drawLine(uint_fast16_t x0, uint_fast16_t y0, uint_fast16_t x1, uint_fast16_t y1, Color color) = 0;
		virtual void drawHLine(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, Color color) = 0;
		virtual void drawVLine(uint_fast16_t x, uint_fast16_t y, uint_fast16_t h, Color color) = 0;

		virtual Color getColor() = 0;
		virtual Color getBgColor() = 0;

		virtual void drawRect(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, Color color) 
		{
			drawHLine(x, y, w, color);			// -
			drawVLine(x, y, h, color);			// Ã
			drawHLine(x, y + h - 1, w, color);	// C
			drawVLine(x + w - 1, y, h, color);	// O
		}

		virtual void fillRect(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, Color color)
		{
			for (uint_fast16_t i = 0; i < h; i++)
			{
				drawHLine(x, y + i, w, color);
			}
		}

		void setCurrentWindow(str_const::HashType name);
		Window* getCurrentWindow() { if (m_current != m_windows.end()) return m_current->second; return NULL; }
		void addWindow(str_const::HashType name, Window *window) { m_windows[name] = window; }
		void removeWindow(str_const::HashType name) { m_windows.erase(name); }
	};

	struct Point
	{
		uint_fast16_t x, y;
		Point() :x(0), y(0) {};
	};
	struct Rect
	{
		uint_fast16_t x, y, w, h;
		Rect() :x(0), y(0), w(0), h(0) {};
	};

	class View
	{
	protected:
		Context *m_context;
	public:
		View(Context*ctx) : m_context(ctx) { }
		virtual ~View() {}
		const Rect& getRect() const { return m_window; }
		virtual void draw(uint_fast16_t x, uint_fast16_t y, bool redraw = false) = 0;
	protected:
		Rect m_window;
	};

	class Element : public View
	{
		bool m_active;
	public:
		bool isActive() const { return m_active; }
		void setActive(bool new_value) { m_active = new_value; }
		virtual void sendKey(uint_fast16_t key) {};
	protected:
		Element(Context*ctx) : View(ctx), m_active(false) {}
	};

	class Window : public Element
	{
	public:
		virtual void init() {};

		virtual bool needUpdate() { return false; }
	protected:
		Window(Context *ctx): Element(ctx) {}
	};

	void Context::setCurrentWindow(str_const::HashType name)
	{
		m_current = m_windows.find(name);
		if (m_current != m_windows.end())
		{
			(m_current->second)->init();
		}
	}


	class TextField : public Element
	{
		String m_text;
		Color m_color;
		Color m_bgcolor;
	public:
		void setText(char const *new_text) { m_text = new_text; }
		void setText(const __FlashStringHelper* new_text) { m_text = new_text; }
		void setText(String const &new_text) { m_text = new_text; }
		TextField(Context *ctx, const String &text, uint_fast16_t w = 0, uint_fast16_t h = 0) : Element(ctx), m_text(text), m_color(-1), m_bgcolor(-1)
		{			
			if (w)
				m_window.w = w;
			else
				m_window.w = m_text.length() * ctx->getCharWidth() + 2;

			if (h)
				m_window.h = h;
			else 
				m_window.h = ctx->getCharHeght() + 2;

			m_color = m_context->getColor();
			m_bgcolor = m_context->getBgColor();
		}

		TextField(Context *ctx, uint_fast16_t w = 0, uint_fast16_t h = 0): Element(ctx)
		{
			if (w)
				m_window.w = w;
			else
				m_window.w = m_text.length() * ctx->getCharWidth() + 2;

			if (h)
				m_window.h = h;
			else
				m_window.h = ctx->getCharHeght() + 2;

			m_color = m_context->getColor();
			m_bgcolor = m_context->getBgColor();
		}

		void draw(uint_fast16_t x = 0, uint_fast16_t y = 0, bool redraw = false)
		{
			uint_fast16_t num_chars;
			bool cropped = false;

			if ((m_window.w - 2) / m_context->getCharWidth() < m_text.length())
			{
				num_chars = (m_window.w - 2 - 4) / m_context->getCharWidth(); // 2 - padding pixels, 4 - Ellipsis " . ."
				cropped = true;
			}
			else
			{
				num_chars = (m_window.w - 2) / m_context->getCharWidth(); // 2 - padding pixels
			}

			String str = m_text.substring(0, num_chars);

			Color color = isActive() ? m_bgcolor : m_color;
			Color bgcolor = isActive() ? m_color : m_bgcolor;

			m_context->fillRect(x + m_window.x, y + m_window.y, m_window.w, m_window.h, bgcolor);
			m_context->drawText(str, x + m_window.x + 1, y + m_window.y + 1, color, bgcolor);

			if (cropped)
			{
				uint_fast16_t ellipsex = x + m_window.x + num_chars * m_context->getCharWidth();
				uint_fast16_t ellipsey = y + m_window.y + m_context->getCharHeght() - 1;
				m_context->drawPixel(ellipsex + 1, ellipsey, color);
				m_context->drawPixel(ellipsex + 3, ellipsey, color);
			}
		}
	};

	class ScrollBar : public View
	{
		bool m_vertical;
		uint_fast16_t m_length;
		uint_fast16_t m_amount;
		uint_fast16_t m_pos;
	public:
		ScrollBar(Context *ctx, uint_fast16_t length, bool vertical = true) : View(ctx)
		{
			m_vertical = vertical;
			m_length = length;

			m_window.h = m_vertical ? m_length : 1;
			m_window.w = m_vertical ? 1 : m_length;
		}
		void setAmount(uint_fast16_t amount)
		{
			m_amount = amount;
		}
		void setPosition(uint_fast16_t pos)
		{
			m_pos = pos;
		}
		void draw(uint_fast16_t x = 0, uint_fast16_t y = 0, bool redraw = false)
		{
			uint_fast16_t sb_start, sb_len;

			if (m_amount)
			{
				if (m_pos + m_length >= m_amount)
				{
					m_pos = m_amount - m_length;
					sb_start = m_pos * m_length / m_amount;
					sb_len = m_length - sb_start;
				}
				else
				{
					sb_start = m_pos * m_length / m_amount;
					sb_len = m_length * m_length / m_amount;
				}

				if (sb_len == 0) sb_len = 1;

				m_context->fillRect(x + m_window.x, y + m_window.y, m_window.w, m_window.h, m_context->getBgColor());

				if (m_vertical)
					m_context->fillRect(x + m_window.x, y + m_window.y + sb_start, m_window.w, sb_len, m_context->getColor());
				else
					m_context->fillRect(x + m_window.x + sb_start, y + m_window.y, sb_len, m_window.h, m_context->getColor());
			}
		}
	};

constexpr int keybLayouts = 2;
constexpr int keybLines = 5;
constexpr int keybKeys = 12;

constexpr const char *onscreenKeyb [keybLayouts][keybLines][keybKeys] PROGMEM =
{
	{
		{"1","2","3","4","5","6","7","8","9","0","_","="}, 
		{"q","w","e","r","t","y","u","i","o","p","+","-"},
		{"a","s","d","f","g","h","j","k","l","?","*","/"},
		{"<",">","z","x","c","v","b","n","m",".",",",":"},
		{"ABC","Space","Del","OK"},
	},
	{
  		{"@","#","$","%","&","(",")","^","\\","~","_","="},
		{"Q","W","E","R","T","Y","U","I","O","P","+","-"},
		{"A","S","D","F","G","H","J","K","L","!","*","/"}, 
		{"[","]","Z","X","C","V","B","N","M",".",",",";"},
		{"abc","Space","Del","OK"},
	}
};

	class ScreenKeyboard : public Element
	{
		uint_fast8_t m_layout;
		uint_fast8_t m_line;
		uint_fast8_t m_keyb;

		uint_fast8_t m_char;

	public:
		ScreenKeyboard(Context *ctx) : Element(ctx)
		{
			m_layout = 0;
			m_line = 0;
			m_keyb = 0;
			m_char = 0;
			m_window.w = 120;
			m_window.h = 50;
		}
		void draw(uint_fast16_t x = 0, uint_fast16_t y = 0, bool redraw = false) 
		{
			TextField button(m_context, 8, 10);
			TextField space(m_context, 32, 10);
			TextField command(m_context, 20, 10);

			for (uint_fast16_t i = 0; i < 4; i++)
			{
				for (uint_fast16_t k = 0; k < 12; k++)
				{
					button.setActive(i == m_line && k == m_keyb);
					button.setText(onscreenKeyb[m_layout][i][k]);

					button.draw(x + k * 10, y + i * 10, redraw);
				}
			}

			command.setActive(0 == m_keyb && m_line == 4);
			command.setText(onscreenKeyb[m_layout][4][0]);
			command.draw(x + 5, y + 4 * 10, redraw);

			space.setActive(1 == m_keyb && m_line == 4);
			space.setText(onscreenKeyb[m_layout][4][1]);
			space.draw(x + 33, y + 4 * 10, redraw);

			command.setActive(2 == m_keyb && m_line == 4);
			command.setText(onscreenKeyb[m_layout][4][2]);
			command.draw(x + 73, y + 4 * 10, redraw);

			command.setActive(3 == m_keyb && m_line == 4);
			command.setText(onscreenKeyb[m_layout][4][3]);
			command.draw(x + 101, y + 4 * 10, redraw);

		}
		void sendKey(uint_fast16_t key, Element* element)
		{
			switch (key)
			{
			case BUTTON_RT:
				m_layout ^= 1;
				break;
			case BUTTON_LT:
				element->sendKey(BUTTON_DEL);
				break;
			case BUTTON_UP:
				if (m_line == 4)
				{
					m_keyb = line5to4[m_keyb];
					m_line--;
				}
				else if (m_line) m_line--;
				break;

			case BUTTON_DOWN:
				if (m_line < 3) m_line++;
				else if (m_line == 3)
				{
					m_keyb = line4to5[m_keyb];
					m_line++;
				}
				break;

			case BUTTON_RIGHT:
				if (m_line == 4)
				{
					if (m_keyb < 3) m_keyb++;
					else m_keyb = 0;
				}
				else if (m_line < 4)
				{
					if (m_keyb < 11) m_keyb++;
					else m_keyb = 0;
				}
				break;

			case BUTTON_LEFT:
				if (m_line == 4)
				{
					if (m_keyb) m_keyb--;
					else m_keyb = 3;
				}
				else if (m_line < 4)
				{
					if (m_keyb) m_keyb--;
					else m_keyb = 11;
				}
				break;

			case BUTTON_A:
				// send key to window
				if (element)
				{
					if (m_line < 4)
					{
						element->sendKey(*onscreenKeyb[m_layout][m_line][m_keyb]);
					}
					else if(m_line == 4)
					{
						switch (m_keyb)
						{
						case 0:
							m_layout ^= 1;
							break;
						case 1:
							element->sendKey(' ');
							break;
						case 2:
							element->sendKey(BUTTON_DEL);
							break;
						case 3:
							element->sendKey(BUTTON_OK);
							break;
						}
					}
				}
				break;

			case BUTTON_B:
				m_layout ^= 1;
				break;

			}
		}
	};

	class List : public Element
	{
		uint_fast16_t m_active;
		uint_fast16_t m_window_pos;

	protected:
		String *m_list;
		uint_fast16_t m_length;
	public:
		List(Context *ctx, uint_fast16_t w, uint_fast16_t h, uint_fast16_t length = 10) : Element(ctx)
		{
			m_window.w = w;
			m_window.h = h;
			m_length = length;

			m_window_pos = 0;
			m_active = 0;

			if (length)
				m_list = new String[length];
			else
				m_list = NULL;
		}

		~List()
		{
			if (m_list)
			{
				delete[] m_list;
				m_list = NULL;
			}
		}

		void setLenght(uint_fast16_t new_lenght, bool clear = false)
		{
			String *old_list = m_list;

			m_list = new String[new_lenght];

			uint_fast16_t min_lenght = new_lenght < m_length ? new_lenght : m_length;

			for (uint_fast16_t i = 0; i < min_lenght; i++)
			{
				m_list[i] = old_list[i];
			}

			delete[] old_list;
			m_length = new_lenght;
		}

		void setText(uint_fast16_t index, const String &str)
		{
			if (m_length > index )
			{
				m_list[index] = str;
			}
		}

		void setText(uint_fast16_t index, char const *str)
		{
			if (m_length > index)
			{
				m_list[index] = str;
			}
		}

		void setCurrentLine(uint_fast16_t line)
		{
			m_active = line;
		}

		uint_fast16_t getCurrentLine() const
		{
			return m_active;
		}

		void clear()
		{
			delete[] m_list;
			m_list = new String[m_length];
		}

		uint_fast16_t length() const
		{
			return m_length;
		}

		void draw(uint_fast16_t x = 0, uint_fast16_t y = 0, bool redraw = false)
		{
			uint_fast16_t recth = TextField(m_context).getRect().h;
			uint_fast16_t lines = m_window.h / recth;
			TextField m_text(m_context, m_window.w - (m_length > lines ? 2 : 0));

			if (m_window_pos + lines - 1 < m_active) m_window_pos = m_active - lines + 1;
			if (m_window_pos > m_active) m_window_pos = m_active;

			for (uint_fast16_t i = 0; i < lines; i++)
			{
				if (m_window_pos + i < m_length)
					m_text.setText(m_list[m_window_pos + i]);
				else
					m_text.setText("");

				m_text.setActive(m_active == m_window_pos + i);
				m_text.draw(x, y + i * recth, redraw );
			}

			if (m_length > lines)
			{
				ScrollBar m_scroll(m_context, lines*recth);
				m_scroll.setAmount(m_length*recth);
				m_scroll.setPosition(m_window_pos*recth);
				m_scroll.draw(x + m_window.w - 1, y, redraw);
			}
		}

		void sendKey(uint_fast16_t key)
		{
			if (key == BUTTON_UP && m_active)
				m_active--;

			if (key == BUTTON_DOWN && m_active < m_length+1)
				m_active++;

			if (!m_length) m_active = 0;
			else
			{
				if (m_active >= m_length) m_active = m_length - 1;
			}
		}
	};

	class BorderedElement : public Element
	{
		Element* m_element;
	public:
		BorderedElement(Context *ctx, Element*element) :Element(ctx), m_element(element){}

		virtual void draw(uint_fast16_t x, uint_fast16_t y, bool redraw = false) 
		{
			if (m_element)
			{
				m_context->drawRect(x, y, m_element->getRect().w + 4, m_element->getRect().h + 4, m_context->getColor());
				m_context->drawRect(x + 1, y + 1, m_element->getRect().w + 2, m_element->getRect().h + 2, m_context->getBgColor());
				m_element->draw(x + 2, y + 2, redraw);
			}
		}
		virtual void sendKey(uint_fast16_t key) 
		{
			if (m_element)
			{
				m_element->sendKey(key);
			}
		};
	};
}

#endif // !__ESPBOY_UI_OBJ__
