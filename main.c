#include "MS51_16K.H"

// Segment pins
sbit SEG_A = P1^3;
sbit SEG_B = P0^0;
sbit SEG_C = P1^5;
sbit SEG_D = P0^3;
sbit SEG_E = P0^1;
sbit SEG_F = P1^2;
sbit SEG_G = P1^7;
sbit SEG_DP = P0^4;

// Digit pins
sbit DIGIT1 = P1^4;
sbit DIGIT2 = P1^1;
sbit DIGIT3 = P1^0;

// Relay pin
sbit RELAY = P0^6; // High=OFF (NC), Low=ON (NO)

// Segment patterns (0–9, O, P, N, E, R)
const unsigned char patterns[15] = {
    0x3F, // 0
    0x06, // 1
    0x5B, // 2
    0x4F, // 3
    0x66, // 4
    0x6D, // 5
    0x7D, // 6
    0x07, // 7
    0x7F, // 8
    0x6F, // 9
    0x3F, // O
    0x73, // P
    0x37, // N
    0x79, // E
    0x50  // R
};

// Delay (~16MHz)
void delay_ms(unsigned char ms) {
    unsigned char i, j;
    for (i = 0; i < ms; i++)
        for (j = 0; j < 160; j++);
}

// Display character
void set_segments(char c) {
    unsigned char seg = 0;
    if (c >= '0' && c <= '9') seg = patterns[c - '0'];
    else if (c == 'O') seg = patterns[10];
    else if (c == 'P') seg = patterns[11];
    else if (c == 'N') seg = patterns[12];
    else if (c == 'E') seg = patterns[13];
    else if (c == 'R') seg = patterns[14];
    SEG_A = !(seg & 0x01); SEG_B = !(seg & 0x02);
    SEG_C = !(seg & 0x04); SEG_D = !(seg & 0x08);
    SEG_E = !(seg & 0x10); SEG_F = !(seg & 0x20);
    SEG_G = !(seg & 0x40); SEG_DP = 1;
}

void main(void) {
    unsigned char display[3] = {'0', '0', '0'};
    unsigned char entry[3] = {0, 0, 0};
    const unsigned char password[3] = {8, 9, 4}; // Hardcoded: 894
    unsigned char current_digit = 0;
    unsigned char btn_timer = 0;
    unsigned char last_btn = 0;
    bit unlock_status = 0;
    unsigned char status_timer = 0;

    // Configure ports
    P0M1 &= ~0x5B; P0M2 |= 0x5B;
    P1M1 &= ~0xBF; P1M2 |= 0xBF;
    P0M1 |= 0x80; P0M2 &= ~0x80;
    AINDIDS = 0x80;
    ADCCON0 = 0x02;
    ADCCON1 = 0x01;
    RELAY = 1;
    DIGIT1 = 0; DIGIT2 = 0; DIGIT3 = 0;

    while (1) {
        unsigned int adc_value;
        unsigned char btn = 0, i;

        // Read ADC
        ADCCON0 |= 0x40;
        while (!(ADCCON0 & 0x80));
        ADCCON0 &= ~0x80;
        adc_value = (unsigned int)((ADCRH << 2) | (ADCRL >> 6));

        // Detect buttons
        if (adc_value >= 133 && adc_value <= 194) btn = 1; // LEFT
        else if (adc_value >= 297 && adc_value <= 358) btn = 2; // MIDDLE
        else if (adc_value >= 481 && adc_value <= 542) btn = 3; // RIGHT

        // Button logic
        if (btn == last_btn) {
            btn_timer++;
            if (btn_timer >= 4 && btn) { // ~20ms debounce
                if (btn == 2 && btn_timer >= 140) { // ~700ms hold
                    // Check password
                    if (entry[0] == password[0] && entry[1] == password[1] && entry[2] == password[2]) {
                        unlock_status = 1;
                        RELAY = 0; // ON
                        display[0] = 'O'; display[1] = 'P'; display[2] = 'N';
                    } else {
                        unlock_status = 0;
                        display[0] = 'E'; display[1] = 'R'; display[2] = 'R';
                    }
                    status_timer = 200; // ~1s display
                    btn_timer = 0;
                }
            }
        } else {
            if (btn_timer >= 4 && btn_timer < 40 && last_btn) { // Click
                if (last_btn == 1) { // LEFT
                    if (entry[current_digit] > 0) entry[current_digit]--;
                } else if (last_btn == 3) { // RIGHT
                    if (entry[current_digit] < 9) entry[current_digit]++;
                } else if (last_btn == 2) { // MIDDLE
                    current_digit = (current_digit + 1) % 3;
                }
                display[current_digit] = '0' + entry[current_digit];
            }
            btn_timer = 0;
            last_btn = btn;
        }

        // Handle unlock status
        if (status_timer > 0) {
            status_timer--;
            if (status_timer == 0) {
                // Reset display
                display[0] = '0' + entry[0];
                display[1] = '0' + entry[1];
                display[2] = '0' + entry[2];
                if (unlock_status) {
                    RELAY = 1; // OFF after 5s total
                    unlock_status = 0;
                }
            }
        }

        // Multiplex display (~66Hz)
        for (i = 0; i < 5; i++) {
            set_segments(display[0]);
            DIGIT1 = 1; DIGIT2 = 0; DIGIT3 = 0;
            delay_ms(5);
            DIGIT1 = 0;
            set_segments(display[1]);
            DIGIT2 = 1; DIGIT1 = 0; DIGIT3 = 0;
            delay_ms(5);
            DIGIT2 = 0;
            set_segments(display[2]);
            DIGIT3 = 1; DIGIT1 = 0; DIGIT2 = 0;
            delay_ms(5);
            DIGIT3 = 0;
        }
    }
}