#pragma GCC optimize ("-Os")
#pragma GCC push_options

#include "User_Setup.h"
#define USER_SETUP_LOADED

#include <arduino.h>

// eSPI Library setup
#include <SPI.h>
#include <TFT_eSPI.h>

#include <FS.h>
#include <Wire.h>

#include <Adafruit_MCP4725.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecureBearSSL.h>
#include <Adafruit_MCP23017.h>


#include "ESPboyLogo.h"
#include <pgmspace.h>
#include <Ticker.h>

#include <ArduinoJson.h>

#include "ESPboy_UI/ESPBoy_UIObj.hpp"
#include "ESPboy_UI/ESPBoy_UIKeys.hpp"
#include "ESPboy_UI/ESPboy_UIWiFi.hpp"


//system
#define NLINEFILES            14 //no of files in menu
#define LEDquantity           1

//pins
#define LEDPIN    D4
#define SOUNDPIN  D3

extern void printFast(int x, int y, char* str, int16_t color, int16_t bg);

String ssid;
String pass;

constexpr str_const::HashType scanwindow_name = HASH16("ScanWindow");
constexpr str_const::HashType passwindow_name = HASH16("PassWindow");
constexpr str_const::HashType checkwindow_name = HASH16("CheckWindow");
constexpr str_const::HashType otalistwindow_name = HASH16("OtaWindow");

class TFT_Context : public ESPboy_UI::Context
{
	TFT_eSPI m_tft;

public:
	TFT_Context()
	{
	}

	void initialize()
	{
		m_tft.init();
		m_tft.setRotation(0);
		m_tft.fillScreen(TFT_BLACK);

		m_tft.drawXBitmap(30, 24, ESPboyLogo, 68, 64, TFT_YELLOW);
    printFast (38, 105, "App Store", TFT_YELLOW, TFT_BLACK); 
	}


	uint_fast16_t getCharWidth() { return 6; };
	uint_fast16_t getCharHeght() { return 8; };

	void setTextDatum(uint_fast8_t datum)
	{
		m_tft.setTextDatum(datum);
	}


	void drawText(String& str, uint_fast16_t x, uint_fast16_t y, ESPboy_UI::Color color, ESPboy_UI::Color bgcolor)
	{
		if (color == -1) color = TFT_YELLOW;
		if (bgcolor == -1) bgcolor = TFT_BLACK;
		printFast (x+1, y+4, (char *)str.c_str(), color, bgcolor);
	}

 
	void drawText(char const* str, uint_fast16_t x, uint_fast16_t y, ESPboy_UI::Color color, ESPboy_UI::Color bgcolor)
	{
		if (color == -1) color = TFT_YELLOW;
		if (bgcolor == -1) bgcolor = TFT_BLACK;
		printFast (x+1, y+4, (char *)str, color, bgcolor);
	}

 
	void drawPixel(uint_fast16_t x, uint_fast16_t y, ESPboy_UI::Color& color)
	{
		if (color == -1) color = TFT_YELLOW;
		m_tft.drawPixel(x, y, color);
	}

 
	void drawLine(uint_fast16_t x0, uint_fast16_t y0, uint_fast16_t x1, uint_fast16_t y1, ESPboy_UI::Color color)
	{
		if (color == -1) color = TFT_YELLOW;
		m_tft.drawLine(x0, y0, x1, y1, color);
	}

 
	void drawHLine(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, ESPboy_UI::Color color)
	{
		if (color == -1) color = TFT_YELLOW;
		m_tft.drawFastHLine(x, y, w, color);
	}

 
	void drawVLine(uint_fast16_t x, uint_fast16_t y, uint_fast16_t h, ESPboy_UI::Color color)
	{
		if (color == -1) color = TFT_YELLOW;
		m_tft.drawFastVLine(x, y, h, color);
	}


	virtual void drawRect(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, ESPboy_UI::Color color)
	{
		if (color == -1) color = TFT_YELLOW;
		m_tft.drawRect(x, y, w, h, color);
	};


	virtual void clearScreen()
	{
		m_tft.fillScreen(TFT_BLACK);
	}

	ESPboy_UI::Color getColor()
	{
		return TFT_YELLOW;
	}
	ESPboy_UI::Color getBgColor()
	{
		return TFT_BLACK;
	}

};

void draw_loading(ESPboy_UI::Context& context, bool reset = false)
{
	static bool firstdraw = true;
	static uint8_t index = 0;
	int32_t pos, next_pos;

	constexpr auto x = 29, y = 96, w = 69, h = 14, cnt = 5, padding = 3;

	if (reset)
	{
		firstdraw = true;
		index = 0;
	}

	if (firstdraw)
	{
		context.fillRect(x + 1, y + 1, w - 2, h - 2, TFT_BLACK);
		context.drawRect(x, y, w, h, TFT_YELLOW);
		firstdraw = false;
	}

	pos = ((index * (w - padding - 1)) / cnt);
	index++;
	next_pos = ((index * (w - padding - 1)) / cnt);

	context.fillRect(x + padding + pos, y + padding, next_pos - pos - padding + 1, h - padding * 2, TFT_YELLOW);
}


struct WiFiListStruct { int32_t index, rssi; };

int compareWiFiList2(const void* t1, const void* t2)
{
	const WiFiListStruct* cmp1 = (const WiFiListStruct*)t1;
	const WiFiListStruct* cmp2 = (const WiFiListStruct*)t2;

	if (cmp1->rssi > cmp2->rssi)
		return -1;
	if (cmp1->rssi < cmp2->rssi)
		return 1;
	return 0;
}

class ScanList : public ESPboy_UI::List
{
	WiFiListStruct* m_arr;
	bool no_networks;
public:
	ScanList(ESPboy_UI::Context* ctx, uint_fast16_t w, uint_fast16_t h) :List(ctx, w, h) { m_arr = NULL; no_networks = true; };
	~ScanList()
	{
		if (m_arr)
		{
			delete[] m_arr;
			m_arr = NULL;
		}
	}
	void sendKey(uint_fast16_t key);
};

class ScanListWindow : public ESPboy_UI::Window
{
	ScanList m_list;
	ESPboy_UI::TextField m_caption;
	bool m_firstdraw;
public:
	ScanListWindow(ESPboy_UI::Context* ctx) : Window(ctx), m_list(ctx, 128, 112), m_caption(ctx, 128, 8)
	{
		m_firstdraw = true;
		m_caption.setText(F("Choose WiFi:"));
		m_list.sendKey(ESPboy_UI::BUTTON_B);

		m_context->addWindow(scanwindow_name, this);
	}
	void sendKey(uint_fast16_t key)
	{
		m_list.sendKey(key);
	}
	void draw(uint_fast16_t x, uint_fast16_t y, bool redraw = false)
	{
		if (m_firstdraw || redraw)
		{
			redraw = m_firstdraw;
			m_context->fillRect(0, 0, 128, 128, TFT_BLACK);
			m_firstdraw = false;
		}
		m_caption.draw(0, 0, redraw);
		m_list.draw(0, 12, redraw);
	}
};

class PasswordWindow : public ESPboy_UI::Window
{
	ESPboy_UI::TextField m_caption;
	ESPboy_UI::TextField m_passfield;
	ESPboy_UI::BorderedElement m_passfield_b;
	ESPboy_UI::ScreenKeyboard m_keyboard;
	ESPboy_UI::BorderedElement m_keyboard_b;

	String m_password;
	bool m_firstdraw;
public:
	PasswordWindow(ESPboy_UI::Context* ctx) : Window(ctx),
		m_caption(ctx, String(F("Enter Password")), 128, 8),
		m_passfield(ctx, 74, 10),
		m_passfield_b(ctx, &m_passfield),
		m_keyboard(ctx),
		m_keyboard_b(ctx, &m_keyboard)
	{
		m_firstdraw = true;

		m_context->addWindow(passwindow_name, this);
	}
	void draw(uint_fast16_t x, uint_fast16_t y, bool redraw = false)
	{
		if (m_firstdraw)
		{
			m_context->fillRect(0, 0, 128, 128, TFT_BLACK);
			m_firstdraw = false;
		}
		m_caption.draw(22, 27, redraw);
		m_passfield_b.draw(25, 27 + 12, redraw);
		m_keyboard_b.draw(2, 74, redraw);

	}
	void init();
	void sendKey(uint_fast16_t key);
	void updatePassfield()
	{
		String text;
		for (uint_fast16_t i = 0; i < m_password.length(); i++)
		{
			text += "*";
		}
		m_passfield.setText(text);
	}
};

class CheckWindow : public ESPboy_UI::Window
{
	ESPboy_UI::TextField m_caption;
	ESPboy_UI::TextField m_OK;
	ESPboy_UI::BorderedElement m_OK_b;

	ESPboy_UI::WiFiIcon m_wifiicon;

	String m_password;
	bool m_firstdraw;
	int m_frame;
	int m_status;
public:
	CheckWindow(ESPboy_UI::Context* ctx) : Window(ctx),
		m_caption(ctx, String(F("Message")), 128, 8),
		m_OK(ctx, String(F(" OK ")), 26, 10),
		m_OK_b(ctx, &m_OK),
		m_wifiicon(ctx)
	{
		m_firstdraw = true;
		m_frame = 0;
		m_status = WL_IDLE_STATUS;

		m_context->addWindow(checkwindow_name, this);
	}
	void draw(uint_fast16_t x, uint_fast16_t y, bool redraw = false)
	{
		if (m_firstdraw)
		{
			m_context->fillRect(0, 0, 128, 128, TFT_BLACK);
			m_firstdraw = false;
		}
		m_caption.draw(22, 27, redraw);

		if (m_status != WL_DISCONNECTED)
			m_OK_b.draw(50, 27 + 12, redraw);

		//m_wifiicon.draw(120, 0, redraw);
	}
	void sendKey(uint_fast16_t key);
	void init()
	{
		m_firstdraw = true;
		WiFi.mode(WIFI_STA);

		WiFi.begin(ssid, pass);
	}
	bool needUpdate()
	{
		String connecting;
		bool needupdate = false;

		int status = WiFi.status();

		if (status != m_status)
		{
			needupdate = true;
			m_status = status;
		}

		switch (status)
		{
		case WL_IDLE_STATUS:
			m_caption.setText(F("Idle"));
			break;
		case WL_NO_SSID_AVAIL:
			m_caption.setText(F("No SSID available"));
			break;
		case WL_SCAN_COMPLETED:
			m_caption.setText(F("Scan completed"));
			break;
		case WL_CONNECTED:
			m_caption.setText(F("Connected"));
			break;
		case WL_CONNECT_FAILED:
			m_caption.setText(F("Connect fail"));
			break;
		case WL_CONNECTION_LOST:
			m_caption.setText(F("Connection lost"));
			break;
		case WL_DISCONNECTED:
			connecting = F("Connecting");
			m_frame = (m_frame + 1) % 4;
			for (int i = 0; i < m_frame; i++) connecting += ".";
			m_caption.setText(connecting);
			delay(500);
			needupdate = true;
			break;
		default:
			break;
		};

		return needupdate;
	}
};

class OtaListWindow;

class OtaList : public ESPboy_UI::List
{
	JsonDocument* m_data;
	OtaListWindow* m_base;
public:
	OtaList(ESPboy_UI::Context* ctx, uint_fast16_t w, uint_fast16_t h, OtaListWindow* win) :List(ctx, w, h)
	{
		m_base = win;
		m_data = NULL;
	}
	~OtaList()
	{
	}
	void setDoc(JsonDocument* data)
	{
		m_data = data;
	}
	void sendKey(uint_fast16_t key);
};

class OtaListWindow : public ESPboy_UI::Window
{
	ESPboy_UI::TextField m_caption;
	ESPboy_UI::TextField m_desc;
	OtaList m_sketches;
	ESPboy_UI::WiFiIcon m_wifiicon;

	bool m_loaded;
	BearSSL::WiFiClientSecure m_wifi;
	HTTPClient m_http;

	String m_msg;

	DynamicJsonDocument m_doc;
	DeserializationError m_err;

public:
	String m_session_id;
	PGM_P m_host;
	PGM_P m_cert;

	OtaListWindow(ESPboy_UI::Context* ctx) : Window(ctx),
		m_caption(ctx, 128, 8),
		m_desc(ctx, 128, 8),
		m_sketches(ctx, 128, 116, this),
		m_wifiicon(ctx),
		m_doc(4000)
	{
		m_loaded = false;
		m_host = PSTR("https://store.espboy.com/ota");
		m_cert = PSTR("0D EB 59 64 FC D9 A8 E4 45 12 9A 01 9C 48 F2 8B 3A 6D FB 35");
		//m_host = F("https://192.168.1.32/ota");
		//m_cert = F("18 ae c3 a0 d8 69 68 63 f3 7e c3 fd c4 e3 d0 3d 3e 64 d0 8c");
		m_sketches.setLenght(1);
		m_sketches.setText(0, F("no sketches found"));

		m_wifi.setFingerprint(m_cert);

		m_context->addWindow(otalistwindow_name, this);
	}

	WiFiClient* getWiFiClient() { return &m_wifi; }

	int GetAPICommand(char const* host, String command, JsonDocument& doc, String session = String())
	{
		int result = 0;

		String t_host = host + command;
		//Serial.print(String("Host  ") + t_host);
		//Serial.println();

		delay(1);

		m_http.begin(m_wifi, t_host);
		m_http.addHeader(F("Cookie"), String(F("PHPSESSID=")) + session);
		//Serial.print(String("PHPSESSID=") + session);
		//Serial.println();
		result = m_http.GET();
		//Serial.printf("Result: %d\n", result);
		if (result != HTTP_CODE_OK)
		{
			m_msg = String(F("Error: ")) + m_http.errorToString(result) + String(F(" ")) + String(result);
			//Serial.print(m_msg);
			//Serial.println();
		}
		else
		{
			result = 0;
			if (m_http.getSize() > 4000) // exceed limits
			{
				m_msg = F("The server returned data that exceeds the limits");
				//Serial.print(m_msg);
				//Serial.println();
				result = 4000;
			}
			m_err = deserializeJson(doc, m_http.getStream());

			if (m_err.code() != DeserializationError::Ok)
			{
				m_msg = String(m_err.c_str()) + m_http.getSize();
				//Serial.print(m_msg);
				//Serial.println();
				result = m_err.code();
			}

			if (doc[F("status")] != 200)
			{
				//Serial.print("API found");
				//Serial.println();
				result = doc[F("status")];
			}
			//Serial.print("All ok\n");
		}

		m_http.end();
		return result;
	}

	bool needUpdate()
	{
		int err;

		if (!m_loaded)
		{
			err = GetAPICommand(m_host, F("/version"), m_doc);
			if (!err)
			{
				draw();
				err = GetAPICommand(m_host, F("/login?login=admin&pass=12345"), m_doc);
				if (!err)
				{
					draw();
					m_session_id = m_doc[F("data")][F("session")].as<const char*>();
					//Serial.print(String("sess:") + m_session_id);
					//Serial.println();

					err = GetAPICommand(m_host, F("/sketches"), m_doc, m_session_id);
					if (!err)
					{
						draw();

						JsonObject data = m_doc[F("data")].as<JsonObject>();

						m_sketches.setDoc(&m_doc);

						int arrlength = data.size();

						m_sketches.clear();
						m_sketches.setLenght(arrlength);

						int i = 0;

						for (JsonObject::iterator it = data.begin(); it != data.end(); ++it, ++i)
						{
							m_sketches.setText(i, it->value()[F("name")].as<const char*>());
						}

						m_loaded = true;

						m_caption.setText(F("Choose sketch:"));
					}
					else
					{
						m_msg = F("Sketches not found\n");
					}
				}
				else
				{
					m_msg = F("Not logged\n");
				}
			}
			else
			{
				m_msg = F("API not found\n");
			}
		}
		else
		{
			return false;
		}

		return true;
	}
	void draw(uint_fast16_t x = 0, uint_fast16_t y = 0, bool redraw = false)
	{
		if (m_loaded)
		{
			m_caption.draw(0, 0, redraw);
			m_sketches.draw(0, 12, redraw);
			m_wifiicon.draw(120, 0, redraw);
		}
		else
		{
			String msg, desc;

			m_caption.setText(msg);
			m_caption.draw(64 - msg.length() * 3, 55, redraw);

			m_desc.setText(desc);
			m_desc.draw(64 - desc.length() * 4, 65, redraw);

			m_wifiicon.draw(120, 0, redraw);
		}
	}
	void sendKey(uint_fast16_t key)
	{
		m_sketches.sendKey(key);
	}
};

void ScanList::sendKey(uint_fast16_t key)
{
	if (key == ESPboy_UI::BUTTON_B)
	{
		int n = WiFi.scanNetworks(false, false);
		if (n == 0)
		{
			setLenght(1);
			setText(0, F("no networks found"));
			no_networks = true;
		}
		else if (n == -1)
		{
			setLenght(1);
			setText(0, F("scan running"));
			no_networks = true;
		}
		else if (n == -2)
		{
			setLenght(1);
			setText(0, F("scan failed"));
			no_networks = true;
		}
		else
		{
			setLenght(n);

			if (m_arr) delete[] m_arr;
			m_arr = new WiFiListStruct[n];

			for (int i = 0; i < n; ++i) { m_arr[i].index = i; m_arr[i].rssi = WiFi.RSSI(i); };

			qsort(m_arr, n, sizeof(m_arr[0]), (__compar_fn_t)compareWiFiList2);

			for (int i = 0; i < n; ++i) {
				int k = m_arr[i].index;
				String str = String(F("(")) + WiFi.RSSI(k) + String(F(") ")) + WiFi.SSID(k) + String(WiFi.encryptionType(k) == ENC_TYPE_NONE ? " " : "*");
				setText(i, str);
			}
			no_networks = false;
		}
	}
	if (key == ESPboy_UI::BUTTON_A)
	{
		if (this->length() && !no_networks)
		{
			ssid = WiFi.SSID(m_arr[getCurrentLine()].index);
			m_context->setCurrentWindow(passwindow_name);
		}
	}
	List::sendKey(key);
}

void PasswordWindow::sendKey(uint_fast16_t key)
{
	if (key == ESPboy_UI::BUTTON_B)
	{
		m_context->setCurrentWindow(scanwindow_name);
	}
	else if (key < 9)
	{
		m_keyboard.sendKey(key, this);
	}
	else if (key == ESPboy_UI::BUTTON_OK)
	{
		pass = m_password;
		m_context->setCurrentWindow(checkwindow_name);
	}
	else if (key == ESPboy_UI::BUTTON_DEL)
	{
		if (m_password.length() < 2)
		{
			m_password = "";
		}
		else
		{
			m_password = m_password.substring(0, m_password.length() - 1);
		}
		updatePassfield();
	}
	else
	{
		m_password += (char)key;
		updatePassfield();
	}
}

void PasswordWindow::init()
{
	if (WiFi.SSID() == ssid)
	{
		pass = WiFi.psk();
		m_context->setCurrentWindow(checkwindow_name);
	}
	else
	{
		m_firstdraw = true;
	}
}

void update_started() {
	////Serial.println("CALLBACK:  HTTP update process started");
}

void update_finished() {
	////Serial.println("CALLBACK:  HTTP update process finished");
	ESP.reset();
}

void update_progress(int cur, int total) {
	////Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
}

void update_error(int err) {
	////Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
	ESP.reset();
}


void OtaList::sendKey(uint_fast16_t key)
{
	if (key == ESPboy_UI::BUTTON_A)
	{
		int idnum = 0;

		JsonObject data = (*m_data)[F("data")].as<JsonObject>();
		for (JsonObject::iterator it = data.begin(); it != data.end(); ++it)
		{
			////Serial.print(it->value()["name"].as<const char*>());
			////Serial.print(" = ");
			////Serial.print(it->value()["id"].as<int>());
			////Serial.println();
			if (m_list[getCurrentLine()].equals(it->value()[F("name")].as<const char*>()))
			{
				idnum = it->value()[F("id")].as<int>();
				break;
			}
		}

		if (idnum)
		{
			int err = m_base->GetAPICommand(m_base->m_host, String(F("/sketches?id=")) + String(idnum), *m_data, m_base->m_session_id);

			if (!err)
			{
				ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);

				// Add optional callback notifiers
				ESPhttpUpdate.onStart(update_started);
				ESPhttpUpdate.onEnd(update_finished);
				ESPhttpUpdate.onProgress(update_progress);
				ESPhttpUpdate.onError(update_error);
				////Serial.print((*m_data)["data"]["link"].as<char*>());

				// t_httpUpdate_return ret = 
				ESPhttpUpdate.update(*m_base->getWiFiClient(), (*m_data)[F("data")][F("link")].as<char*>());
			}
			else
			{
				////Serial.print("error on sketches by id\n");
			}
		}
		else
		{
			////Serial.print("id not found\n");
		}
	}

	if (key == ESPboy_UI::BUTTON_B)
	{
		WiFi.mode(WIFI_STA);
		wifi_station_disconnect();
		m_context->setCurrentWindow(scanwindow_name);
	}

	List::sendKey(key);
}


void CheckWindow::sendKey(uint_fast16_t key)
{
	if (key == ESPboy_UI::BUTTON_A)
	{
		if (WiFi.status() == WL_CONNECTED)
		{
			m_context->setCurrentWindow(otalistwindow_name);
		}
		else
		{
			WiFi.mode(WIFI_STA);
			wifi_station_disconnect();
			if (WiFi.status() == WL_CONNECT_FAILED)
			{
				m_context->setCurrentWindow(passwindow_name);
			}
			else
			{
				m_context->setCurrentWindow(scanwindow_name);
			}
		}
	}
}

ESPboy_UI::Keyboard* keyb;
volatile bool keyneedupdate = false;

void KeyboardProc(ESPboy_UI::Context* ctx, ESPboy_UI::KeyMessage msg)
{
	ESPboy_UI::Window* window = ctx->getCurrentWindow();

	if (window && msg.op == ESPboy_UI::KEYOP_PRESS)
	{
		window->sendKey(msg.key);
		keyneedupdate = true;
	}
}

void ota_loop(ESPboy_UI::Context& ctx)
{
	static bool needupdate = true, winneedupdate, mustredraw = true;
	static ESPboy_UI::Window* prev;

	if (needupdate)
	{
		needupdate = false;
		mustredraw = false;
		keyneedupdate = false;
		ctx.getCurrentWindow()->draw(0, 12, mustredraw);
	}

	keyb->runMessages();

	winneedupdate = ctx.getCurrentWindow()->needUpdate();

	needupdate = keyneedupdate || winneedupdate;

	if (prev != ctx.getCurrentWindow())
	{
		prev = ctx.getCurrentWindow();
		mustredraw = true;
	}

}

class OTACore;
OTACore* otacore = NULL;

class OTACore
{
	TFT_Context context;
	ScanListWindow scanwindow;
	PasswordWindow passwindow;
	CheckWindow checkwindow;
	OtaListWindow otalistwindow;

public:
	OTACore() :
		scanwindow(&context),
		passwindow(&context),
		checkwindow(&context),
		otalistwindow(&context)
	{
		initOta();
	}

	static bool needOta()
	{
		if (ESPboy_UI::CheckOtaButtons())
		{
			otacore = new OTACore;
			return true;
		}
		return false;
	}

	static bool OtaLoop() { if (!otacore) return false; ota_loop(otacore->context); return true; }
protected:
	void initOta()
	{
		keyb = ESPboy_UI::CreateESPBoyKeyboard(&context);
		keyb->setKeyboardProc(KeyboardProc);

		context.initialize();
		//draw_loading(context);

		WiFi.setAutoConnect(false);
		WiFi.mode(WIFI_STA);

		//draw_loading(context);
		wifi_station_disconnect();
		//draw_loading(context);
		delay(100);
		//draw_loading(context);

		if (WiFi.SSID().length())
		{
			ssid = WiFi.SSID();
			pass = WiFi.psk();
			context.setCurrentWindow(checkwindow_name);
		}
		else
		{
			context.setCurrentWindow(scanwindow_name);
			scanwindow.sendKey(ESPboy_UI::BUTTON_B);
		}
		//draw_loading(context);

    delay(2000);
		context.clearScreen();
	}
};

extern "C" {

	int OTASetup()
	{
		return OTACore::needOta();
	}

	int OTALoop()
	{
		return OTACore::OtaLoop();
	}

}
