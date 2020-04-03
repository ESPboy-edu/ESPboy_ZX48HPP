#include <Arduino.h>

#include "ESPboy_UI/ESPBoy_UIkeys.hpp"

#include <Ticker.h>
#include <functional>

#include <Adafruit_MCP23017.h>
#include <queue>

//buttons
#define LEFT_BUTTON_PRESSED(buttonspressed)   (buttonspressed & 1)
#define UP_BUTTON_PRESSED(buttonspressed)     (buttonspressed & 2)
#define DOWN_BUTTON_PRESSED(buttonspressed)   (buttonspressed & 4)
#define RIGHT_BUTTON_PRESSED(buttonspressed)  (buttonspressed & 8)
#define ACT_BUTTON_PRESSED(buttonspressed)    (buttonspressed & 16)
#define ESC_BUTTON_PRESSED(buttonspressed)    (buttonspressed & 32)
#define LFT_BUTTON_PRESSED(buttonspressed)    (buttonspressed & 64)
#define RGT_BUTTON_PRESSED(buttonspressed)    (buttonspressed & 128)

#define LEFT_BUTTON  0
#define UP_BUTTON    1
#define DOWN_BUTTON  2
#define RIGHT_BUTTON 3
#define ACT_BUTTON   4
#define ESC_BUTTON   5
#define LFT_BUTTON   6
#define RGT_BUTTON   7

#define csTFTMCP23017pin      8
#define MCP23017address       0 // actually it's 0x20 but in <Adafruit_MCP23017.h> lib there is (x|0x20) :)
#define MCP4725dacresolution  8
#define MCP4725address        0

extern volatile uint16_t buttons_data;

namespace ESPboy_UI
{
	const int_fast16_t TICK_MS = 25;
	const int_fast16_t BUTTON_COUNT = 8;
	const int_fast16_t BUTTON_DELAY_COUNT = 250 / TICK_MS;
	const int_fast16_t BUTTON_REPEAT_COUNT = 100 / TICK_MS;

	//std::queue<KeyMessage>

	bool CheckOtaButtons()
	{
		Adafruit_MCP23017	m_mcp;
		bool result;
		m_mcp.begin(MCP23017address);
		for (uint_fast16_t i = 0; i < 8; i++)
		{
			m_mcp.pinMode(i, INPUT);
			m_mcp.pullUp(i, HIGH);
		}

		delay(200);

		result = !!(~m_mcp.readGPIOAB() & 192);		
		return result;
	}

	Keyboard::Keyboard(ESPboy_UI::Context *ctx, Keyboard *base) :m_context(ctx), m_proc(NULL), m_base(base) { m_queue = new std::queue<KeyMessage>; }
	Keyboard::~Keyboard() { delete static_cast<std::queue<KeyMessage>*>(m_queue); }
	
	void Keyboard::runMessages()
	{
		std::queue<KeyMessage>* queue = static_cast<std::queue<KeyMessage>*>(m_queue);
		while (!queue->empty())
		{
			sendMessage(queue->front());
			queue->pop();
		}
	}

	void Keyboard::storeMessage(const KeyMessage msg)
	{
		std::queue<KeyMessage>* queue = static_cast<std::queue<KeyMessage>*>(m_queue);
		if (queue->size() < max_queue)
		{
			queue->push(msg);
		}
	}

	class ESPBoyKeyboard: public Keyboard
	{
		Ticker				m_ticker;
		int_fast16_t		m_state;
		int_fast16_t		m_lastpress[BUTTON_COUNT];

		Adafruit_MCP23017	m_mcp;
	
	public:
		ESPBoyKeyboard(ESPboy_UI::Context *ctx): Keyboard(ctx), m_state(0)
		{
			for (uint_fast16_t i = 0; i < BUTTON_COUNT; i++)
			{
				m_lastpress[i] = BUTTON_DELAY_COUNT;
			}

			m_mcp.begin(MCP23017address);
			for (uint_fast16_t i = 0; i < 8; i++)
			{
				m_mcp.pinMode(i, INPUT);
				m_mcp.pullUp(i, HIGH);
			}

			m_mcp.pinMode(csTFTMCP23017pin, OUTPUT);
			m_mcp.digitalWrite(csTFTMCP23017pin, LOW);

			m_ticker.attach_ms(TICK_MS, std::bind(&ESPBoyKeyboard::Tick, this));
		}

		~ESPBoyKeyboard()
		{
			m_ticker.detach();
		}

		uint_fast16_t readbuttons()
		{
			return ~m_mcp.readGPIOAB() & 255;
		}

		void Tick()
		{
			uint_fast16_t new_state = readbuttons();

			KeyMessage msg;

			uint_fast8_t unpressed = !new_state & (new_state ^ m_state);
			uint_fast8_t pressed = new_state & (new_state ^ m_state);
			uint_fast8_t waited = new_state & m_state;

			for (uint_fast16_t i = 0; i < BUTTON_COUNT; i++)
			{
				bool keyunpressed = (unpressed >> i) & 1;
				bool keypressed = (pressed >> i) & 1;
				bool keywait = (waited >> i) & 1;

				if ( keywait && m_lastpress[i] ) m_lastpress[i] -= 1;

				if ( keypressed ) m_lastpress[i] = BUTTON_DELAY_COUNT;

				switch (i)
				{
				case LEFT_BUTTON:
					msg.key = BUTTON_LEFT;
					break;
				case UP_BUTTON:
					msg.key = BUTTON_UP;
					break;
				case DOWN_BUTTON:
					msg.key = BUTTON_DOWN;
					break;
				case RIGHT_BUTTON:
					msg.key = BUTTON_RIGHT;
					break;
				case ACT_BUTTON:
					msg.key = BUTTON_A;
					break;
				case ESC_BUTTON:
					msg.key = BUTTON_B;
					break;
				case LFT_BUTTON:
					msg.key = BUTTON_LT;
					break;
				case RGT_BUTTON:
					msg.key = BUTTON_RT;
					break;
				}

				if (keyunpressed)
				{
					msg.op = KEYOP_UP;
					storeMessage(msg);
				}

/*				if ((keyunpressed && (m_lastpress[i] - BUTTON_REPEAT_COUNT) < 0)
					|| (m_lastpress[i] - BUTTON_REPEAT_COUNT) == 0)
				{
					msg.op = KEYOP_PRESS;
					storeMessage(msg);
				}*/

				if (keypressed)
				{
					msg.op = KEYOP_DOWN;
					storeMessage(msg);
					msg.op = KEYOP_PRESS;
					storeMessage(msg);
					m_lastpress[i] = BUTTON_DELAY_COUNT;
				}

				if (!m_lastpress[i]) 
				{ 
					msg.op = KEYOP_PRESS;
					storeMessage(msg);
					m_lastpress[i] = BUTTON_REPEAT_COUNT;
				}
			}

			m_state = new_state;
		}

	private: // no copy
  	ESPBoyKeyboard(const ESPBoyKeyboard&) = delete;
	  ESPBoyKeyboard& operator=(const ESPBoyKeyboard&) = delete;
	};

	Keyboard* CreateESPBoyKeyboard(ESPboy_UI::Context *ctx)
	{
		return new ESPBoyKeyboard(ctx);
	}
}
