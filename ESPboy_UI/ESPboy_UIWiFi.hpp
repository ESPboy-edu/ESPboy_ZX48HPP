#pragma once

#ifndef __ESPBOY_UIWIFI_OBJ__
#define __ESPBOY_UIWIFI_OBJ__

#include "ESPBoy_UIObj.hpp"
#include <ESP8266WiFi.h>

namespace ESPboy_UI
{

class WiFiIcon : public Element
{
	int m_lastbars;
public:
	WiFiIcon(Context *ctx): Element(ctx), m_lastbars(0) {}

	int rssi2bar(int32_t rssi)
	{
		if (!rssi) return 0;
		if (rssi > -50) return 4;
		if (rssi > -60) return 3;
		if (rssi > -70) return 2;
		return 1;
	}

	void draw(uint_fast16_t x = 0, uint_fast16_t y = 0, bool redraw = false)
	{
		int32_t rssivalue = 0;
		if (WiFi.status() == WL_CONNECTED)
		{
			rssivalue = WiFi.RSSI();
		}

		if (m_lastbars != rssi2bar(rssivalue) || redraw)
		{
			ESPboy_UI::Color color = m_context->getColor();

			m_lastbars = rssi2bar(rssivalue);
			m_context->fillRect(x, y, 8, 8, m_context->getBgColor());
			m_context->fillRect(x, y + 6, 2, 2, color);

			if (m_lastbars > 1)
			{
				m_context->drawHLine(x, y + 4, 3, color);
				m_context->drawVLine(x + 3, y + 5, 3, color);
			}
			if (m_lastbars > 2)
			{
				m_context->drawHLine(x, y + 2, 4, color);
				m_context->drawHLine(x + 4, y + 3, 1, color);
				m_context->drawVLine(x + 5, y + 4, 4, color);
			}
			if (m_lastbars > 3)
			{
				m_context->drawHLine(x, y, 5, color);
				m_context->drawHLine(x + 5, y + 1, 1, color);
				m_context->drawHLine(x + 6, y + 2, 1, color);
				m_context->drawVLine(x + 7, y + 3, 5, color);
			}

			if (m_lastbars == 0)
			{
				m_context->drawVLine(x + 4, y, 4, color);
				m_context->drawHLine(x + 4, y + 5, 1, color);
			}
		}
	}
};

}

#endif // !__ESPBOY_UIWIFI_OBJ__