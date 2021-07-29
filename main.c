#include<avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <util/atomic.h>



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


static volatile uint64_t time = 0;

ISR(TIMER0_COMPA_vect){
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        time += 1;
    }
}

//MODES
enum modes{
    NORMAL    = 0,
    SETTIME   = 1
};
uint8_t modes = 0;

//digit state machine init
uint8_t digits[6] = {1, 4, 1, 4, 0, 0};
enum digits{
    FIRST  = 0b00000001,
    SIXS   = 0b00100000
};
uint8_t digits_convert[] = {0b00001100, 0b00000100, 0b00000010, 0b00001000, 0b00000001, 
                            0b00001011, 0b00001010, 0b00000011, 0b00001001, 0b00000000};
uint8_t curr_digit;
uint64_t next_change;
uint8_t digit_light;
#define shine_time 3
void digite_state_m_init(){
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        next_change = time + shine_time;
    }
    digit_light = 0;
    curr_digit = 0;
}


void change_digit(){
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        if (next_change > time) return;
    }
    curr_digit = (curr_digit + 1) % 6;
    digit_light = (digit_light + 1) % 6;
    uint8_t temp_c = digits_convert[digits[curr_digit]];
    uint8_t temp_d = 1 << digit_light;
    PORTC = temp_c;
    PORTD = temp_d;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        next_change = time + shine_time;
    } 
}

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

//time loader state machine init
uint64_t next_update;
#define time_upd 1000

void update_time(){
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        if (next_update > time) return;
        upd_time();
        next_update = time + time_upd;
    } 
}

uint64_t next_button_update;
uint8_t buttons = 0;
#define time_but_upd 10
//PIN == 0 when button pushed
#define KEY_A                (1<<6) // red button,
uint8_t prev_A = 0, count_A = 0;
//PIN == 0 when button released
#define KEY_B                (1<<0) //green button, PIN is correct
uint8_t prev_B = 0, count_B = 0;

uint8_t set_pair = 2;
void upd_buttons(){
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        if (next_button_update > time) return;
        next_button_update = time + time_but_upd;
    }
    if ((PINB & KEY_B) == prev_B){
        if (prev_B){
            count_B++;
        }

        if (!prev_B && count_B > 0 && count_B < 30 && modes == NORMAL){
            set_pair = 2;
            modes = SETTIME;
        }

        if (count_B > 200 && modes == SETTIME){
            modes = NORMAL;
            set_time();
        }

        if (!prev_B && count_B > 0 && count_B < 30 && modes == SETTIME){
            uint8_t set_val = digits[set_pair * 2] * 10 + digits[set_pair * 2 + 1];
            set_val += 1;
            set_val %= 60;
            digits[set_pair * 2] = set_val / 10;
            digits[set_pair * 2 + 1] = set_val % 10;
        }

        if (!prev_B) {
            count_B = 0;
        }
    } else {
        prev_B = (PINB & KEY_B);
    }

    if ((PINB & KEY_A) == prev_A){
        if (!prev_A){
            count_A++;
        }

        if (prev_A && count_A > 0 && count_A < 30 && modes == SETTIME){
            uint8_t set_val = digits[set_pair * 2] * 10 + digits[set_pair * 2 + 1];
            if (set_val){
                set_val -= 1;
            } else {
                set_val = 59;
            }
            digits[set_pair * 2] = set_val / 10;
            digits[set_pair * 2 + 1] = set_val % 10;
        }
        if (count_A > 200 && modes == SETTIME){
            if (set_pair){
                set_pair -= 1;
            } else {
                set_pair = 2;
            }
        }

        if (prev_A) {
            count_A = 0;
        }
    } else {
        prev_A = (PINB & KEY_A);
    }
}


int main(void){
    DDRD =  0b11111111;
    DDRC =  0b11111111;
    //right button settings
    DDRB =  0;
    PORTB = 0b11111111;
    //
    // PORTC = digits_convert[0];
    // PORTD = 0b00000011;
    TWIInit();
    // set_time();

    //Timer init
    cli();
    TCCR0A |= 1<<WGM01;
    TCCR0B |= 1<<CS01; 
    TCNT0 = 0;
    OCR0A = 125; // 1/5 of a milisecond
    TIMSK0 |= 1 << OCIE0A; //allow interrupt
    sei();
    digite_state_m_init();
    while(1){
        upd_buttons();
        change_digit();
        if (modes == NORMAL){
            update_time();
        }
    }
    return 0;
}