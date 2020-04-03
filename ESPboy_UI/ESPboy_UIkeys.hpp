#ifndef __ESPBOY_UI_KEYS__
#define __ESPBOY_UI_KEYS__

#include <stddef.h>

namespace ESPboy_UI
{
	enum Key: unsigned short {
		BUTTON_NONE = 0,	//        -----------
		BUTTON_UP,			//        |    o    |
		BUTTON_DOWN,		//    LT [|   /|\   |] RT
		BUTTON_LEFT,		//        |   / \   |
		BUTTON_RIGHT,		//        |---------|
		BUTTON_A,			//        |  U    A |
		BUTTON_B,			//        | L D     |
		BUTTON_LT,			//        |  D   B  |
		BUTTON_RT,			//        |---------|
		BUTTON_DEL = 0x7F,
		BUTTON_OK = 0x0A,
	};

	enum KeyControl : unsigned short
	{
		KEYCTRL_NONE = 0,
		KEYCTRL_ALT = 1,
		KEYCTRL_CTRL = 2,
		KEYCTRL_SHIFT = 4,
		KEYCTRL_CMD = 8,
	};

	enum KeyOp 
	{
		KEYOP_NONE,
		KEYOP_UP,
		KEYOP_DOWN,
		KEYOP_PRESS,
	};

	constexpr uint max_queue = 24;
	extern bool CheckOtaButtons();

	class Context;

	struct KeyMessage
	{
		Key key;
		KeyOp op;
		KeyControl ctrl;
		bool repeat;
		KeyMessage(Key vkey = BUTTON_NONE, KeyOp vop = KEYOP_NONE, KeyControl vctrl = KEYCTRL_NONE)
		{
			key = vkey;
			op = vop;
			ctrl = vctrl;
			repeat = false;
		}
	};

	class Keyboard
	{
		ESPboy_UI::Context *m_context;
		void(*m_proc)(ESPboy_UI::Context *ctx, const KeyMessage msg);
		Keyboard *m_base;
	public:
		Keyboard(ESPboy_UI::Context *ctx, Keyboard *base = NULL);
		virtual ~Keyboard();
		void setKeyboardProc(void(*proc)(ESPboy_UI::Context *ctx, const KeyMessage msg) = NULL)
		{
			m_proc = proc;
		}

		void runMessages();

	protected:

		void* m_queue;

		void storeMessage(const KeyMessage msg);

		void sendMessage(const KeyMessage msg)
		{
			if (m_base) m_base->sendMessage(msg);
			if (m_proc) m_proc(m_context, msg);
		}

	private: // no copy
		Keyboard(const Keyboard&) = delete;
		Keyboard& operator=(const Keyboard&) = delete;

	};

	Keyboard* CreateESPBoyKeyboard(ESPboy_UI::Context* ctx);
}

#endif // !__ESPBOY_UI_KEYS__