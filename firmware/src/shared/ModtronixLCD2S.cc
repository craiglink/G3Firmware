#include <string.h>
#include <util/delay.h>
#include "ModtronixLCD2S.hh"
#include "SPIBuffer.hh"


static SPIBuffer spiBuffer;

ModtronixLCD2S::ModtronixLCD2S(uint8_t addr)
{
  address = addr;
  charset = 255;
}

void ModtronixLCD2S::init()
{
    spiBuffer.init();
    _delay_us(50000); // give the LCD some time to power up
    clear();
    home();
}

void ModtronixLCD2S::write_command( uint8_t cmd )
{
    spiBuffer.write(0xF5);
    spiBuffer.write(cmd);
}

void ModtronixLCD2S::write_command( uint8_t cmd, uint8_t val )
{
    spiBuffer.write(0xF5);
    spiBuffer.write(cmd);
    if(cmd == 0xF5) { spiBuffer.write(cmd); }
}

void ModtronixLCD2S::write_command( uint8_t cmd, uint8_t* data, uint8_t len )
{
    write_command(cmd);
    while(len--)
    {
        uint8_t b = *data++;
        spiBuffer.write(b);
        if(b == 0xF5) { spiBuffer.write(b); }
    }
}

void ModtronixLCD2S::write_command( uint8_t cmd, PGM_P data, uint8_t len )
{
    write_command(cmd);
    while(len--)
    {
        uint8_t b = pgm_read_byte(data++);
        spiBuffer.write(b);
        if(b == 0xF5) { spiBuffer.write(b); }
    }
}

void ModtronixLCD2S::remember()
{
  write_command(0x8D);
}



void ModtronixLCD2S::config(uint8_t display, uint8_t contrast, uint8_t brightness, uint8_t keypadio, uint8_t keypadbuzz)
{
  uint8_t data[4];
  data[0] = display;
  data[1] = ((brightness & 0x3) << 6) | (contrast & 0x3f);
  data[2] = keypadio;
  data[3] = keypadbuzz;
  write_command(0xD0, data, 4);
}



void ModtronixLCD2S::set_base_address(uint8_t addr) {
  remember();
  write_command(0x91, addr);
}



void ModtronixLCD2S::write_decimal(int16_t val, int8_t wid, uint8_t decdigits) {
  char buf[10];
  char* ptr = buf;
  char* ptr2;
  bool force = false;
  if (val < 0) {
    val = -val;
    *ptr++ = '-';
  }
  if (decdigits == 5) {
    *ptr++ = '0';
    *ptr++ = '.';
    force = true;
  }
  if (force || val >= 10000) {
    *ptr++ = '0'+(val/10000);
    val = val % 10000;
    force = true;
  }
  if (decdigits == 4) {
    if (ptr == buf) {
      *ptr++ = '0';
    }
    *ptr++ = '.';
    force = true;
  }
  if (force || val >= 1000) {
    *ptr++ = '0'+(val/1000);
    val = val % 1000;
    force = true;
  }
  if (decdigits == 3) {
    if (ptr == buf) {
      *ptr++ = '0';
    }
    *ptr++ = '.';
    force = true;
  }
  if (force || val >= 100) {
    *ptr++ = '0'+(val/100);
    val = val % 100;
    force = true;
  }
  if (decdigits == 2) {
    if (ptr == buf) {
      *ptr++ = '0';
    }
    *ptr++ = '.';
    force = true;
  }
  if (force || val >= 10) {
    *ptr++ = '0'+(val/10);
    val = val % 10;
  }
  if (decdigits == 1) {
    if (ptr == buf) {
      *ptr++ = '0';
    }
    *ptr++ = '.';
  }
  *ptr++ = '0'+val;
  *ptr = '\0';
  if (ptr-buf < wid) {
    ptr2 = buf+wid;
    while (ptr >= buf) {
      *ptr2-- = *ptr--;
    }
    while (ptr2 >= buf) {
      *ptr2-- = ' ';
    } 
  }
  write_command(0x80, (uint8_t*)buf, strlen(buf));
}

void ModtronixLCD2S::writeInt(uint16_t value, uint8_t digits) {
    
    char buf[5];

	uint16_t currentDigit;
	switch (digits) {
	case 1:		currentDigit = 10;		break;
	case 2:		currentDigit = 100;		break;
	case 3:		currentDigit = 1000;	break;
	case 4:		currentDigit = 10000;	break;
	default: 	return;
	}

    uint8_t i;
	for (i = 0; i < digits; i++) {
		uint16_t nextDigit = currentDigit/10;
        buf[i] = (value%currentDigit)/nextDigit+'0';
		currentDigit = nextDigit;
	}

    write_command(0x80, (uint8_t*)buf, i);
}


void ModtronixLCD2S::write_string(const char* str) {
  write_command(0x80, (uint8_t*)str, strlen(str));
}



void ModtronixLCD2S::write_string_P(PGM_P pstr) {
  write_command(0x80, pstr, (uint8_t)strlen_P(pstr));
}



void ModtronixLCD2S::set_startup_line(uint8_t row, const char* str) {
  uint8_t buf[22];
  buf[0] = row;
  strcpy((char*)buf+1,str);
  write_command(0x90, buf, strlen(str)+1);
}



void ModtronixLCD2S::write_char(char ch) {
  write_command(0x80, ch);
}



void ModtronixLCD2S::backlight_on() {
  write_command(0x28);
}



void ModtronixLCD2S::backlight_off() {
  write_command(0x20);
}



void ModtronixLCD2S::backlight_brightness(uint8_t val) {
  write_command(0x81, val);
}



void ModtronixLCD2S::set_contrast(uint8_t val) {
  write_command(0x82, val);
}



void ModtronixLCD2S::dir_forward() {
  write_command(0x09);
}



void ModtronixLCD2S::dir_backwards() {
  write_command(0x01);
}



void ModtronixLCD2S::display_on() {
  write_command(0x1A);
}



void ModtronixLCD2S::display_off() {
  write_command(0x12);
}



void ModtronixLCD2S::cursor_underline_on() {
  write_command(0x19);
}



void ModtronixLCD2S::cursor_underline_off() {
  write_command(0x11);
}



void ModtronixLCD2S::cursor_block_on() {
  write_command(0x18);
}



void ModtronixLCD2S::cursor_block_off() {
  write_command(0x10);
}



void ModtronixLCD2S::shift_right() {
  write_command(0x85);
}



void ModtronixLCD2S::shift_left() {
  write_command(0x86);
}



void ModtronixLCD2S::shift_up() {
  write_command(0x87);
}



void ModtronixLCD2S::shift_down() {
  write_command(0x88);
}



void ModtronixLCD2S::home() {
  write_command(0x8B);
}



void ModtronixLCD2S::clear() {
  write_command(0x8C);
}



void ModtronixLCD2S::load_charset(uint8_t val) {
  if (val != charset) {
    write_command(0x8E, val);
    charset = val;
  }
}



void ModtronixLCD2S::set_position(uint8_t row, uint8_t col) {
  uint8_t data[2];
  data[0] = row;
  data[1] = col;
  write_command(0x8A, data, 2);
}


void ModtronixLCD2S::interrupt_open_collection() {
  write_command(0x2A);
}


void ModtronixLCD2S::interrupt_push_pull() {
  write_command(0x22);
}


void ModtronixLCD2S::out1_on() {
  write_command(0x38);
}



void ModtronixLCD2S::out1_off() {
  write_command(0x30);
}



void ModtronixLCD2S::out2_on() {
  write_command(0x39);
}



void ModtronixLCD2S::out2_off() {
  write_command(0x31);
}



void ModtronixLCD2S::gpio1_on() {
  write_command(0x48);
}



void ModtronixLCD2S::gpio1_off()  {
  write_command(0x40);
}



void ModtronixLCD2S::gpio2_on() {
  write_command(0x49);
}



void ModtronixLCD2S::gpio2_off() {
  write_command(0x41);
}



void ModtronixLCD2S::config_keypad_and_io(uint8_t val) {
  remember();
  write_command(0xE0, val);
}



void ModtronixLCD2S::config_gpio1(uint8_t val) {
  remember();
  write_command(0xE3, val);
}



void ModtronixLCD2S::config_gpio2(uint8_t val) {
  remember();
  write_command(0xE4, val);
}



void ModtronixLCD2S::config_gpio3(uint8_t val) {
  remember();
  write_command(0xE5, val);
}



void ModtronixLCD2S::set_max_backlight_brightness(uint8_t val) {
  remember();
  write_command(0xA3, val);
}



void ModtronixLCD2S::set_keypad_debounce_time(uint8_t val) {
  remember();
  write_command(0xE1, val);
}



void ModtronixLCD2S::set_keypad_repeat_delay(uint8_t val) {
  remember();
  write_command(0xA0, val);
}



void ModtronixLCD2S::set_keypad_repeat_rate(uint8_t val) {
  remember();
  write_command(0xA1, val);
}



void ModtronixLCD2S::set_keypad_buzzer_period(uint8_t val) {
  remember();
  write_command(0xA2, val);
}



void ModtronixLCD2S::write_large_number_string(const char* str) {
  load_charset(2);
  write_command(0x8F, (uint8_t*)str, strlen(str));
}



void ModtronixLCD2S::set_cursor_addr(uint8_t val) {
  write_command(0x89, val);
}



void ModtronixLCD2S::set_out12(uint8_t val1, uint8_t val2) {
  uint8_t val = (val1? 0x1 : 0) | (val2? 0x2 : 0);
  write_command(0xE2, val);
}



void ModtronixLCD2S::draw_vertical_bar_graph(uint8_t row, uint8_t col, uint8_t val) {
  uint8_t buf[3];
  buf[0] = row;
  buf[1] = col;
  buf[2] = val;
  load_charset(0);
  write_command(0x93, buf, 3);
}



void ModtronixLCD2S::draw_tall_bar_graph(uint8_t row, uint8_t col, uint8_t val) {
  uint8_t buf[3];
  buf[0] = row;
  buf[1] = col;
  buf[2] = val;
  load_charset(1);
  write_command(0x94, buf, 3);
}



void ModtronixLCD2S::define_custom_char(uint8_t adr, const uint8_t *data) {
  uint8_t buf[9];
  buf[0] = adr;
  memcpy(buf+1, data, 8);
  write_command(0x92, buf, 9);
}



// vim: set sw=2 autoindent nowrap expandtab: settings


