/*
* NixieClock
*
* Code to run a Nixie Clock designed by Colin Luoma
*
* Uses a GPS devices to get current time
*/

#include <TinyGPS++.h>
#include <SoftwareSerial.h>

// Global variables
TinyGPSPlus gps;
SoftwareSerial ss(0, 1);

uint8_t hour_ten = 0;
uint8_t hour_one = 0;
uint8_t minute_ten = 0;
uint8_t minute_one = 0;
uint8_t hour_offset = 0;

int debounce = 0;
unsigned long debounce_time = 0;

// Pin Definitions
int tac_switch_1 = 4;
int tac_switch_2 = 13;
int tac_switch_3 = 2;
int tac_switch_4 = 7;
int latchPin = 8;
int clockPin = 12;
int dataPin = 11;

void
setup()
{
    // Init serial stuff
    Serial.begin(9600);
    ss.begin(9600);

    // Setup pins
    pinMode(tac_switch_1, INPUT);
    pinMode(tac_switch_2, INPUT);
    pinMode(tac_switch_3, INPUT);
    pinMode(tac_switch_4, INPUT);
    pinMode(latchPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    pinMode(dataPin, OUTPUT);
}

uint8_t
reverse_digit(uint8_t digit)
// Because I'm an idiot and the nixie tube leads are mirrored on the PCB
// This fixes the output
{
    switch(digit)
    {
        case 0:
            return 1;
        case 1:
            return 0;
        case 2:
            return 9;
        case 3:
            return 8;
        case 4:
            return 7;
        case 5:
            return 6;
        case 6:
            return 5;
        case 7:
            return 4;
        case 8:
            return 3;
        case 9:
            return 2;
    }
}

void
get_time()
// Parses the current time from GPS data and stores digits for each nixie
{
    hour_ten = ((gps.time.hour() + hour_offset) % 24) / 10;
    hour_one = ((gps.time.hour() + hour_offset) % 24) % 10;
    minute_ten = gps.time.minute() / 10;
    minute_one = gps.time.minute() % 10;
}

void
write_digits(uint8_t dig1, uint8_t dig2, uint8_t dig3, uint8_t dig4)
// Displays the supplied numbers on the Nixie Tubes, dig1 is the leftmost tube
{
    // Latch shift registers
    digitalWrite(latchPin, LOW);
    // Shift out the bits
    shiftOut(dataPin, clockPin, MSBFIRST,
        (reverse_digit(dig4) << 4)+reverse_digit(dig3));
    shiftOut(dataPin, clockPin, MSBFIRST,
        (reverse_digit(dig2) << 4)+reverse_digit(dig1));
    // Un-latch
    digitalWrite(latchPin, HIGH);
}

void
burn_in()
// Sequentially goes through each digit on the nixie tubes for 30s
// This is to extend the life of the tubes
{
    for(int i = 9; i >= 0; i--)
    {
        write_digits(i, i, i, i);
        delay(30000);
    }
}

void
matrix_random()
// Displays random numbers at increasingly long intervals
{
    int delay_time = 100;
    while (delay_time <= 1000)
    {
        write_digits(rand()%10, rand()%10, rand()%10, rand()%10);
        delay(delay_time);
        delay_time += 50;
    }

    delay(5000);
}

void
loop()
{
    // Read data from GPS
    while (ss.available() > 0)
        gps.encode(ss.read());

    // Read tac switch 1, this switch increments the hour offset by 1
    if (digitalRead(tac_switch_1) == HIGH && debounce == 0)
    {
        hour_offset += 1;
        hour_offset %= 24;

        debounce = 1;
        debounce_time = millis();
    }

    // Read tac switch 2, this starts a 'burn-in' sequence for the nixie tubes
    if (digitalRead(tac_switch_2) == HIGH && debounce == 0)
    {
        burn_in();
    }

    // Read tac switch 3, this generates a matrix-style random 4-digit number
    if (digitalRead(tac_switch_3) == HIGH && debounce == 0)
    {
        matrix_random();
    }


    // Monitor debounce timings
    if ((millis() - debounce_time) > 500)
        debounce = 0;

    // Parse current time from GPS data
    get_time();

    // Send time to display on nixies
    write_digits(hour_ten, hour_one, minute_ten, minute_one);
}
