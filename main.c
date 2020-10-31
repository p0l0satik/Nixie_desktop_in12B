#include<avr/io.h>
#include <util/delay.h>

#define delay 2.5
void TWIInit(void)
{
    //set SCL to 400kHz
    TWSR = 0x00;
    TWBR = 0x0C;
    //enable TWI
    TWCR = (1<<TWEN);
}
void TWIStart(void)
{
    TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
    while ((TWCR & (1<<TWINT)) == 0);
}
//send stop signal
void TWIStop(void)
{
    TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
    while ((TWCR & (1<<TWSTO)) == 0);
}
void TWISend(uint8_t u8data)
{
    TWDR = u8data;
    TWCR = (1<<TWINT)|(1<<TWEN);
    while ((TWCR & (1<<TWINT)) == 0);
}
uint8_t TWIReadACK(void)
{
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);
    while ((TWCR & (1<<TWINT)) == 0);
    return TWDR;
}
//read byte with NACK
uint8_t TWIReadNACK(void)
{
    TWCR = (1<<TWINT)|(1<<TWEN);
    while ((TWCR & (1<<TWINT)) == 0);
    return TWDR;
}
uint8_t TWIGetStatus(void)
{
    uint8_t status;
    //mask status
    status = TWSR & 0xF8;
    return status;
}

uint8_t digits[6] = {2, 2, 4, 1, 0, 0};
void upd_time(){
    TWIStart();
    TWISend(0b10100000);
    TWISend(2);
    TWIStart();
    TWISend(0b10100001);
    uint8_t sec = TWIReadACK();
    digits[5] = sec & 0x0F;
    digits[4] = (sec & 0xF0) >> 4;
    uint8_t min = TWIReadACK();
    digits[3] = min & 0x0F;
    digits[2] = (min & 0xF0) >> 4;
    uint8_t hr = TWIReadNACK();
    digits[1] = hr & 0x0F;
    digits[0] = (hr & 0xF0) >> 4 ;
    TWIStop();
}
void set_time(){
    TWIStart();
    TWISend(0b10100000);
    TWISend(2);
    TWISend((digits[4] << 4) + digits[5]);
    TWISend((digits[2] << 4) + digits[3]);
    TWISend((digits[0] << 4) + digits[1]);
    TWIStop();
}

enum digits{
    FIRST  = 0b00000001,
    SECOND = 0b00000010,
    THIRD  = 0b00000100,
    FORTH  = 0b00001000,
    FIFTH  = 0b00010000,
    SIXS   = 0b00100000,

    NINE   = 0b00000000,
    FOUR   = 0b00000001,
    TWO    = 0b00000010,
    SEVEN  = 0b00000011,
    ONE    = 0b00000100,
    THREE  = 0b00001000,
    EIGHT  = 0b00001001,
    FIVE   = 0b00001011,
    SIX    = 0b00001010,
    ZERO   = 0b00001100
};
uint8_t digits_convert[] = {0b00001100, 0b00000100, 0b00000010, 0b00001000, 0b00000001, 
                            0b00001011, 0b00001010, 0b00000011, 0b00001001, 0b00000000};
int main(void){
    DDRD =  0b11111111;
    DDRC =  0b11111111;
    TWIInit();
    set_time();

    while(1){
        PORTD = FIRST;
        PORTC = digits_convert[digits[0]];
        _delay_ms(delay);
        PORTD <<= 1;
        PORTC = digits_convert[digits[1]];
        _delay_ms(delay);
        PORTD <<= 1;
        PORTC = digits_convert[digits[2]];
        _delay_ms(delay);
        PORTD <<= 1;
        PORTC = digits_convert[digits[3]];
        _delay_ms(delay);
        PORTD <<= 1;
        PORTC = digits_convert[digits[4]];
        _delay_ms(delay);
        PORTD <<= 1;
        PORTC = digits_convert[digits[5]];
        _delay_ms(delay);
        upd_time();
    }
    return 0;
}