//------------------------------------------------------------------------------------------------------------
//
// ODROID-C 16x2 LCD / LED / Button Test Application.
//
// taken from https://wiki.odroid.com/accessory/display/16x2_lcd_io_shield/c/start
// and adapted by steets@otech.nl
//
// Defined port number is wiringPi port number.
//
// Compile : gcc -g -o lcd lcd.c -lwiringPi -lwiringPiDev -lm -lpthread -lrt -lcrypt
//
// Run : sudo ./lcd
//
//------------------------------------------------------------------------------------------------------------
#include <argp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <unistd.h>
#include <string.h>
#include <time.h>

#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <wiringSerial.h>
#include <lcd.h>

#define max(a, b) (a > b ? a : b)

//------------------------------------------------------------------------------------------------------------
//
// LCD:
//
//------------------------------------------------------------------------------------------------------------
#define LCD_ROW 2             // 16 Char
#define LCD_COL 16            // 2 Line
#define LCD_BUS 4             // Interface 4 Bit mode
#define LCD_UPDATE_PERIOD 300 // 300ms

static int lcdHandle = 0;

#define PORT_LCD_RS 7
#define PORT_LCD_E 0
#define PORT_LCD_D4 2
#define PORT_LCD_D5 3
#define PORT_LCD_D6 1
#define PORT_LCD_D7 4

//------------------------------------------------------------------------------------------------------------
//
// Button:
//
//------------------------------------------------------------------------------------------------------------
#define PORT_BUTTON1 5
#define PORT_BUTTON2 6

//------------------------------------------------------------------------------------------------------------
//
// LED:
//
//------------------------------------------------------------------------------------------------------------
static int ledPos = 0;

const int ledPorts[] = {
    21,
    22,
    23,
    24,
    11,
    26,
    27,
};

#define MAX_LED_CNT sizeof(ledPorts) / sizeof(ledPorts[0])

//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------
//
// Collect arguments
//
//------------------------------------------------------------------------------------------------------------

const char *argp_program_version = "lcd v0.1.0";
static char doc[] = "write message to LCD over GPIO";
const char *argp_program_bug_address = "steets@otech.nl";
static char args_doc[] = "[STR1 [STR2]]";
static struct argp_option options[] = {
    {"leds", 'l', "0000000", 0, "State of the leds"},
    {0}};

typedef struct
{
    uint8_t size;
    unsigned char lines[LCD_ROW][LCD_COL];
    unsigned char leds[MAX_LED_CNT];
} Arguments;

void init_arguments(Arguments *args)
{
    args->size = 0;
    memset(args->lines, ' ', LCD_ROW * LCD_COL);
    memset(args->leds, '0', MAX_LED_CNT);
}

uint8_t add_line(Arguments *args, char *arg)
{
    if (args->size >= LCD_ROW)
    {
        return 0;
    }
    unsigned char *line = args->lines[args->size++];
    memset(line, ' ', LCD_COL);
    memcpy(line, arg, max(strlen(arg), LCD_ROW));

    return 1;
}

void set_leds(Arguments *args, char *leds)
{
    memset(args->leds, '0', MAX_LED_CNT);
    memcpy(args->leds, leds, max(strlen(leds), MAX_LED_CNT));
}

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{

    Arguments *args = (Arguments *)state->input;
    switch (key)
    {
    case 'l':
        set_leds(args, arg);
        break;
    case ARGP_KEY_ARG:
        if (!add_line(args, arg)) // Too many arguments.
            argp_usage(state);
        break;
    case ARGP_KEY_END:
        if (state->arg_num < 0) // Not enough arguments.
            argp_usage(state);
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc};

//------------------------------------------------------------------------------------------------------------
//
// LCD Update Function:
//
//------------------------------------------------------------------------------------------------------------

static void lcd_update(Arguments *args)
{
    for (uint8_t i = 0; i < LCD_ROW; i++)
    {
        lcdPosition(lcdHandle, 0, i);

        unsigned char *line = args->lines[i];
        for (uint8_t j = 0; j < LCD_COL; j++)
            lcdPutchar(lcdHandle, line[j]);
    }
}

//------------------------------------------------------------------------------------------------------------
//
// board data update
//
//------------------------------------------------------------------------------------------------------------
void ledUpdate(Arguments *args)
{
    int value;

    //  LED Control
    for (uint8_t i = 0; i < MAX_LED_CNT; i++)
    {
        value = args->leds[i] == '1' ? 1 : 0;
        digitalWrite(ledPorts[i], value);
    }
}

//------------------------------------------------------------------------------------------------------------
//
// system init
//
//------------------------------------------------------------------------------------------------------------
int system_init(void)
{
    int i;

    // LCD Init
    lcdHandle = lcdInit(LCD_ROW, LCD_COL, LCD_BUS,
                        PORT_LCD_RS, PORT_LCD_E,
                        PORT_LCD_D4, PORT_LCD_D5, PORT_LCD_D6, PORT_LCD_D7, 0, 0, 0, 0);

    if (lcdHandle < 0)
    {
        fprintf(stderr, "%s : lcdInit failed!\n", __func__);
        return -1;
    }

    // GPIO Init(LED Port ALL Output)
    for (i = 0; i < MAX_LED_CNT; i++)
    {
        pinMode(ledPorts[i], OUTPUT);
        pullUpDnControl(PORT_BUTTON1, PUD_OFF);
    }

    // Button Pull Up Enable.
    pinMode(PORT_BUTTON1, INPUT);
    pullUpDnControl(PORT_BUTTON1, PUD_UP);
    pinMode(PORT_BUTTON2, INPUT);
    pullUpDnControl(PORT_BUTTON2, PUD_UP);

    return 0;
}

//------------------------------------------------------------------------------------------------------------
//
// Start Program
//
//------------------------------------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    int timer = 0;

    Arguments args;
    init_arguments(&args);
    argp_parse(&argp, argc, argv, 0, 0, &args);

    wiringPiSetup();

    if (system_init() < 0)
    {
        fprintf(stderr, "%s: System Init failed\n", __func__);
        return -1;
    }

    // board update
    ledUpdate(&args);
    lcd_update(&args);

    return 0;
}

//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------
