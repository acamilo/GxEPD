/************************************************************************************
   class GxGDEW042T2 : Display class example for GxGDEW042T2 e-Paper from Dalian Good Display Co., Ltd.: www.good-display.com

   based on Demo Example from Good Display, now available on http://www.good-display.com/download_list/downloadcategoryid=34&isMode=false.html

   Author : J-M Zingg

   modified by :

   Version : 2.0

   Support: limited, provided as example, no claim to be fit for serious use

   connection to the e-Paper display is through DESTM32-S2 connection board, available from Good Display

   DESTM32-S2 pinout (top, component side view):
       |-------------------------------------------------
       |  VCC  |o o| VCC 5V, not needed
       |  GND  |o o| GND
       |  3.3  |o o| 3.3V
       |  nc   |o o| nc
       |  nc   |o o| nc
       |  nc   |o o| nc
       |  MOSI |o o| CLK=SCK
       | SS=DC |o o| D/C=RS    // Slave Select = Device Connect |o o| Data/Command = Register Select
       |  RST  |o o| BUSY
       |  nc   |o o| BS, connect to GND
       |-------------------------------------------------
*/

#include "GxGDEW042T2.h"

#if defined(__AVR)
#include <avr/pgmspace.h>
#endif

GxGDEW042T2::GxGDEW042T2(GxIO& io, uint8_t rst, uint8_t busy)
  : GxEPD(GxGDEW042T2_WIDTH, GxGDEW042T2_HEIGHT),
    IO(io), _rst(rst), _busy(busy),
    _current_page(-1), _using_partial_mode(false)
{
}

template <typename T> static inline void
swap(T& a, T& b)
{
  T t = a;
  a = b;
  b = t;
}

void GxGDEW042T2::drawPixel(int16_t x, int16_t y, uint16_t color)
{
  if ((x < 0) || (x >= width()) || (y < 0) || (y >= height())) return;

  // check rotation, move pixel around if necessary
  switch (getRotation())
  {
    case 1:
      swap(x, y);
      x = GxGDEW042T2_WIDTH - x - 1;
      break;
    case 2:
      x = GxGDEW042T2_WIDTH - x - 1;
      y = GxGDEW042T2_HEIGHT - y - 1;
      break;
    case 3:
      swap(x, y);
      y = GxGDEW042T2_HEIGHT - y - 1;
      break;
  }
  uint16_t i = x / 8 + y * GxGDEW042T2_WIDTH / 8;
  if (_current_page < 1)
  {
    if (i >= sizeof(_buffer)) return;
  }
  else
  {
    if (i < GxGDEW042T2_PAGE_SIZE * _current_page) return;
    if (i >= GxGDEW042T2_PAGE_SIZE * (_current_page + 1)) return;
    i -= GxGDEW042T2_PAGE_SIZE * _current_page;
  }

  if (!color)
    _buffer[i] = (_buffer[i] | (1 << (7 - x % 8)));
  else
    _buffer[i] = (_buffer[i] & (0xFF ^ (1 << (7 - x % 8))));
}

void GxGDEW042T2::init(void)
{
  IO.init();
  IO.setFrequency(4000000); // 4MHz : 250ns > 150ns min RD cycle
  digitalWrite(_rst, HIGH);
  pinMode(_rst, OUTPUT);
  pinMode(_busy, INPUT);
  fillScreen(GxEPD_WHITE);
  _current_page = -1;
  _using_partial_mode = false;
}

void GxGDEW042T2::fillScreen(uint16_t color)
{
  uint8_t data = (color == GxEPD_BLACK) ? 0xFF : 0x00;
  for (uint16_t x = 0; x < sizeof(_buffer); x++)
  {
    _buffer[x] = data;
  }
}

void GxGDEW042T2::update(void)
{
  if (_current_page != -1) return;
  _using_partial_mode = false;
  _wakeUp();
  IO.writeCommandTransaction(0x13);
  for (uint32_t i = 0; i < GxGDEW042T2_BUFFER_SIZE; i++)
  {
    uint8_t data = _buffer[i];
    IO.writeDataTransaction(~data);
  }
  IO.writeCommandTransaction(0x12);      //display refresh
  _waitWhileBusy("update");
  _sleep();
}

void  GxGDEW042T2::drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color)
{
  drawBitmap(bitmap, x, y, w, h, color);
}

void  GxGDEW042T2::drawBitmap(const uint8_t *bitmap, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color, bool mirror)
{
  if (mirror)
  {
    for (uint16_t x1 = x; x1 < x + w; x1++)
    {
      for (uint16_t y1 = y; y1 < y + h; y1++)
      {
        uint32_t i = (w - (x1 - x) - 1) / 8 + uint32_t(y1 - y) * uint32_t(w) / 8;
#if defined(__AVR)
        uint16_t pixelcolor = (pgm_read_byte(bitmap + i) & (0x01 << (x1 - x) % 8)) ? GxEPD_WHITE  : color;
#else
        uint16_t pixelcolor = (bitmap[i] & (0x01 << (x1 - x) % 8)) ? GxEPD_WHITE  : color;
#endif
        drawPixel(x1, y1, pixelcolor);
      }
    }
  }
  else
  {
    for (uint16_t x1 = x; x1 < x + w; x1++)
    {
      for (uint16_t y1 = y; y1 < y + h; y1++)
      {
        uint32_t i = (x1 - x) / 8 + uint32_t(y1 - y) * uint32_t(w) / 8;
#if defined(__AVR)
        uint16_t pixelcolor = (pgm_read_byte(bitmap + i) & (0x80 >> (x1 - x) % 8)) ? GxEPD_WHITE  : color;
#else
        uint16_t pixelcolor = (bitmap[i] & (0x80 >> (x1 - x) % 8)) ? GxEPD_WHITE  : color;
#endif
        drawPixel(x1, y1, pixelcolor);
      }
    }
  }
}

void GxGDEW042T2::drawBitmap(const uint8_t *bitmap, uint32_t size)
{
  drawBitmap(bitmap, size, false);
}

void GxGDEW042T2::drawBitmap(const uint8_t *bitmap, uint32_t size, bool using_partial_update)
{
  if (using_partial_update)
  {
    _using_partial_mode = true; // remember
    _wakeUp();
    // set full screen
    IO.writeCommandTransaction(0x91); // partial in
    _setPartialRamArea(0, 0, GxGDEW042T2_WIDTH - 1, GxGDEW042T2_HEIGHT - 1);
    for (uint32_t i = 0; i < GxGDEW042T2_BUFFER_SIZE; i++)
    {
#if defined(__AVR)
      IO.writeDataTransaction((i < size) ? pgm_read_byte(bitmap + i) : 0xFF);
#else
      IO.writeDataTransaction((i < size) ? bitmap[i] : 0xFF);
#endif
    }
    IO.writeCommandTransaction(0x12);      //display refresh
    _waitWhileBusy("drawBitmap");
    IO.writeCommandTransaction(0x92); // partial out
  }
  else
  {
    _using_partial_mode = false; // remember
    _wakeUp();
    IO.writeCommandTransaction(0x13);
    for (uint32_t i = 0; i < GxGDEW042T2_BUFFER_SIZE; i++)
    {
#if defined(__AVR)
      IO.writeDataTransaction((i < size) ? pgm_read_byte(bitmap + i) : 0xFF);
#else
      IO.writeDataTransaction((i < size) ? bitmap[i] : 0xFF);
#endif
    }
    IO.writeCommandTransaction(0x12);      //display refresh
    _waitWhileBusy("drawBitmap");
    _sleep();
  }
}

void GxGDEW042T2::eraseDisplay(bool using_partial_update)
{
  if (using_partial_update)
  {
    _using_partial_mode = true; // remember
    _wakeUp();
    // set full screen
    IO.writeCommandTransaction(0x91); // partial in
    _setPartialRamArea(0, 0, GxGDEW042T2_WIDTH - 1, GxGDEW042T2_HEIGHT - 1);
    for (uint32_t i = 0; i < GxGDEW042T2_BUFFER_SIZE; i++)
    {
      IO.writeDataTransaction(0xFF);
    }
    IO.writeCommandTransaction(0x12);      //display refresh
    _waitWhileBusy("eraseDisplay");
    IO.writeCommandTransaction(0x92); // partial out
  }
  else
  {
    _using_partial_mode = false; // remember
    _wakeUp();
    IO.writeCommandTransaction(0x13);
    for (uint32_t i = 0; i < GxGDEW042T2_BUFFER_SIZE; i++)
    {
      IO.writeDataTransaction(0xFF);
    }
    IO.writeCommandTransaction(0x12);      //display refresh
    _waitWhileBusy("eraseDisplay");
    _sleep();
  }
}

void GxGDEW042T2::updateWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool using_rotation)
{
  if (using_rotation)
  {
    switch (getRotation())
    {
      case 1:
        swap(x, y);
        swap(w, h);
        x = GxGDEW042T2_WIDTH - x - w - 1;
        break;
      case 2:
        x = GxGDEW042T2_WIDTH - x - w - 1;
        y = GxGDEW042T2_HEIGHT - y - h - 1;
        break;
      case 3:
        swap(x, y);
        swap(w, h);
        y = GxGDEW042T2_HEIGHT - y  - h - 1;
        break;
    }
  }
  //fillScreen(0x0);
  if (x >= GxGDEW042T2_WIDTH) return;
  if (y >= GxGDEW042T2_HEIGHT) return;
  // x &= 0xFFF8; // byte boundary, not here, use encompassing rectangle
  uint16_t xe = min(GxGDEW042T2_WIDTH, x + w) - 1;
  uint16_t ye = min(GxGDEW042T2_HEIGHT, y + h) - 1;
  // x &= 0xFFF8; // byte boundary, not needed here
  uint16_t xs_bx = x / 8;
  uint16_t xe_bx = (xe + 7) / 8;
  if (!_using_partial_mode) _wakeUp();
  _using_partial_mode = true;
  IO.writeCommandTransaction(0x91); // partial in
  _setPartialRamArea(x, y, xe, ye);
  IO.writeCommandTransaction(0x13);
  for (int16_t y1 = y; y1 <= ye; y1++)
  {
    for (int16_t x1 = xs_bx; x1 < xe_bx; x1++)
    {
      uint16_t idx = y1 * (GxGDEW042T2_WIDTH / 8) + x1;
      uint8_t data = (idx < sizeof(_buffer)) ? _buffer[idx] : 0x00;
      IO.writeDataTransaction(~data);
    }
  }
  IO.writeCommandTransaction(0x12);      //display refresh
  _waitWhileBusy("updateWindow");
  IO.writeCommandTransaction(0x92); // partial out
}

void GxGDEW042T2::_setPartialRamArea(uint16_t x, uint16_t y, uint16_t xe, uint16_t ye)
{
  x &= 0xFFF8; // byte boundary
  xe = (xe - 1) | 0x0007; // byte boundary - 1
  IO.writeCommandTransaction(0x90); // partial window
  IO.writeDataTransaction(x / 256);
  IO.writeDataTransaction(x % 256);
  IO.writeDataTransaction(xe / 256);
  IO.writeDataTransaction(xe % 256);
  IO.writeDataTransaction(y / 256);
  IO.writeDataTransaction(y % 256);
  IO.writeDataTransaction(ye / 256);
  IO.writeDataTransaction(ye % 256);
  IO.writeDataTransaction(0x01); // don't see any difference
  //IO.writeDataTransaction(0x00); // don't see any difference
}

void GxGDEW042T2::_waitWhileBusy(const char* comment)
{
  unsigned long start = micros();
  while (1)
  { //=0 BUSY
    if (digitalRead(_busy) == 1) break;
    delay(1);
  }
  if (comment)
  {
    //    unsigned long elapsed = micros() - start;
    //    Serial.print(comment);
    //    Serial.print(" : ");
    //    Serial.println(elapsed);
  }
}

void GxGDEW042T2::_wakeUp(void)
{
  digitalWrite(_rst, 0);
  delay(10);
  digitalWrite(_rst, 1);
  delay(10);
  IO.writeCommandTransaction(0x06); // boost
  IO.writeDataTransaction(0x17);
  IO.writeDataTransaction(0x17);
  IO.writeDataTransaction(0x17);
  IO.writeCommandTransaction(0x04);
  _waitWhileBusy("Power On");
  IO.writeCommandTransaction(0x00);
  IO.writeDataTransaction(0x1f); // LUT from OTP Pixel with B/W.
}

void GxGDEW042T2::_sleep(void)
{
  IO.writeCommandTransaction(0X50); // border floating
  IO.writeDataTransaction(0x17);
  IO.writeCommandTransaction(0X02); // power off
  _waitWhileBusy("Power Off");
  IO.writeCommandTransaction(0X07); // deep sleep
  IO.writeDataTransaction(0xA5);
}

void GxGDEW042T2::drawPaged(void (*drawCallback)(void))
{
  _wakeUp();
  IO.writeCommandTransaction(0x13);
  for (_current_page = 0; _current_page < GxGDEW042T2_PAGES; _current_page++)
  {
    fillScreen(0xFF);
    drawCallback();
    for (int16_t y1 = 0; y1 < GxGDEW042T2_PAGE_HEIGHT; y1++)
    {
      for (int16_t x1 = 0; x1 < GxGDEW042T2_WIDTH / 8; x1++)
      {
        uint16_t idx = y1 * (GxGDEW042T2_WIDTH / 8) + x1;
        uint8_t data = (idx < sizeof(_buffer)) ? _buffer[idx] : 0x00;
        IO.writeDataTransaction(~data);
      }
    }
  }
  _current_page = -1;
  IO.writeCommandTransaction(0x12);      //display refresh
  _waitWhileBusy("drawPaged");
  _sleep();
}

void GxGDEW042T2::drawCornerTest()
{
  _wakeUp();
  IO.writeCommandTransaction(0x13);
  for (uint32_t y = 0; y < GxGDEW042T2_HEIGHT; y++)
  {
    for (uint32_t x = 0; x < GxGDEW042T2_WIDTH / 8; x++)
    {
      uint8_t data = 0xFF;
      if ((x < 1) && (y < 8)) data = 0x00;
      if ((x > GxGDEW042T2_WIDTH / 8 - 3) && (y < 16)) data = 0x00;
      if ((x > GxGDEW042T2_WIDTH / 8 - 4) && (y > GxGDEW042T2_HEIGHT - 25)) data = 0x00;
      if ((x < 4) && (y > GxGDEW042T2_HEIGHT - 33)) data = 0x00;
      IO.writeDataTransaction(data);
    }
  }
  IO.writeCommandTransaction(0x12);      //display refresh
  _waitWhileBusy("drawCornerTest");
  _sleep();
}

