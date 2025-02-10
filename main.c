#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>

#define RS_PIN (1 << PA2)
#define EN_PIN (1 << PD6)
#define LCD_PORT PORTC

#define MODEL_S 1
#define MODEL_3 2
#define MODEL_X 3
#define MODEL_Y 4

unsigned int percent = 0;
int current_power = 0;

void lcd_command(uint8_t cmd) {
	PORTA &= ~RS_PIN;
	PORTD |= EN_PIN;
	LCD_PORT = (LCD_PORT & 0x0F) | (cmd & 0xF0);
	_delay_us(1);
	PORTD &= ~EN_PIN;
	_delay_us(1);
	PORTD |= EN_PIN;
	LCD_PORT = (LCD_PORT & 0x0F) | (cmd << 4);
	_delay_us(1);
	PORTD &= ~EN_PIN;
	_delay_ms(2);
}

void lcd_data(uint8_t data) {
	PORTA |= RS_PIN;
	PORTD |= EN_PIN;
	LCD_PORT = (LCD_PORT & 0x0F) | (data & 0xF0);
	_delay_us(1);
	PORTD &= ~EN_PIN;
	_delay_us(1);
	PORTD |= EN_PIN;
	LCD_PORT = (LCD_PORT & 0x0F) | (data << 4);
	_delay_us(1);
	PORTD &= ~EN_PIN;
	_delay_ms(2);
}

void lcd_init() {
	DDRA = 4;
	DDRB = 255; 
	DDRC = 255;
	DDRD = 255;
	_delay_ms(20);
	lcd_command(0x33);
	lcd_command(0x32);
	lcd_command(0x28);
	lcd_command(0x0C);
	lcd_command(0x06);
	lcd_command(0x01);
	_delay_ms(2);
}

void lcd_clear() {
	lcd_command(0x01);
	_delay_ms(2);
}

void lcd_write_string(const char *str) {
	while (*str) {
		lcd_data(*str++);
	}
}

void display_main_screen() { 
	lcd_clear();
	lcd_write_string("Pick Model: ");
	_delay_ms(1);
	lcd_command(0b11000000);  
	_delay_ms(1);
	lcd_write_string("1:S 2:3 3:X 4:Y");
	_delay_ms(500);
}

void display_charging_status(int percent, int power) {
	lcd_clear();
	char buffer[16];
	snprintf(buffer, 16, "%3u kW %3u%%", power, percent);
	lcd_write_string(buffer);

	int led = (1 << percent / 12) - 1;
	PORTB = led;
}

int get_battery_capacity(int model) {
	switch (model) {
		case MODEL_S: return 100;
		case MODEL_3: return 75;
		case MODEL_X: return 90;
		case MODEL_Y: return 85;
		default: return 0;
	}
}

int get_charging_power(int model) {
	switch (model) {
		case MODEL_S: return 120;
		case MODEL_3: return 110;
		case MODEL_X: return 130;
		case MODEL_Y: return 100;
		default: return 0;
	}
}

void buzzer_end(){
	TCCR1A = 0b10100010;
	TCCR1B = 0b00011010;
	ICR1 = 4000;
	OCR1B = 2000;
	_delay_ms(1000);
	ICR1 = 0;
	OCR1B = 0;
}

void buzzer_start(){
	TCCR1A = 0b10100010;
	TCCR1B = 0b00011010;
	ICR1 = 4000;
	OCR1B = 2000;
	_delay_ms(400);
	ICR1 = 4000;
	OCR1B = 500;
	_delay_ms(400);
	ICR1 = 4000;
	OCR1B = 4000;
	_delay_ms(400);
	ICR1 = 0;
	OCR1B = 0;
	_delay_ms(200);
}



void charge_battery(int capacity, int max_power) {
	
	percent = ADC;
	percent = percent/9.8;
	
	buzzer_start();
	
	while (percent <= 100) {
		if (percent < 20) {
			current_power = max_power / 3; 
			} else if (percent >= 90) {
			current_power = max_power / 6; 
			} else if (percent > 80) {
			current_power = max_power / 3; 
			} else {
			current_power = max_power; 
		}
		
		display_charging_status(percent, current_power);

		if (percent < 20) {
			_delay_ms(600); 
			} else if (percent >= 90) {
			_delay_ms(1200); 
			} else if (percent > 80) {
			_delay_ms(600); 
			} else {
			_delay_ms(200); 
			}

		percent++;
		
		if (PINA & (1 << PA3)) {
			break;
		}
	}
	buzzer_end();
}

int main(void) {
	lcd_init();
	lcd_command(0b00101000);
	ADMUX = 0b01000001;
	ADCSRA = 0b11100111;

	while (1) {
		display_main_screen();
		int model = 0;
		
		if (PINA & (1 << PA7)) model = MODEL_S;
		if (PINA & (1 << PA6)) model = MODEL_3;
		if (PINA & (1 << PA5)) model = MODEL_X;
		if (PINA & (1 << PA4)) model = MODEL_Y;

		if (model) {
			int capacity = get_battery_capacity(model);
			int power = get_charging_power(model);
			charge_battery(capacity, power);
		}
	}
}
