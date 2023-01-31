//------------------------------------------------------------------------------------------------------------
//
// CLI to control LCD and LEDs of 16x2 LCD IO shield
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
    {"clear", 'c', 0, 0, "Clear the LCD and LEDs"},
    {"leds", 'l', "FLAGS", 0, "Set the LEDs (0=off, 1=on, -=unchanged)"},
    {"row", 'r', "NR", 0, "LCD row to set"},
    {0}};

typedef struct
{
    uint8_t clear;
    int row;
    unsigned char *lines[LCD_ROW];
    unsigned char *leds;
} Arguments;

void init(Arguments *args)
{
    memset(args, 0, sizeof(Arguments));
    args->row = -1;
}

uint8_t get_size(Arguments *args)
{
    for (uint8_t r = 0; r < LCD_ROW; r++)
    {
        if (args->lines[r] == NULL)
        {
            return r;
        }
    }
    return LCD_ROW;
}

uint8_t add_line(Arguments *args, char *arg)
{
    uint8_t size = get_size(args);
    if (size >= LCD_ROW)
    {
        fprintf(stderr, "Too many lines (max %s)\n", LCD_ROW - 1);
        return 1;
    }
    else if ((args->row >= 0) && (size > 0))
    {
        fprintf(stderr, "Multiple texts for row %d ('%s')\n", args->row, arg);
        return 1;
    }
    args->lines[size] = arg;

    return 0;
}

uint8_t set_leds(Arguments *args, char *leds)
{
    int len = strlen(leds);
    if (len >= MAX_LED_CNT)
    {
        fprintf(stderr, "Too many LED values: %s = %d (max %d)\n", leds, len, MAX_LED_CNT);
        return 1;
    }
    args->leds = leds;
    return 0;
}

uint8_t set_row(Arguments *args, int row)
{
    if (row >= LCD_ROW)
    {
        fprintf(stderr, "Row index %d out of bounds (max %d)\n", row, LCD_ROW - 1);
        return 1;
    }
    if (get_size(args) > 1)
    {
        fprintf(stderr, "Multiple texts for row %d\n", row);
        return 1;
    }
    args->row = row;
    return 0;
}

uint8_t set_clear(Arguments *args)
{
    args->clear = 1;
    return 0;
}

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{

    Arguments *args = (Arguments *)state->input;
    uint8_t result = 0, size;

    switch (key)
    {
    case 'c':
        result = set_clear(args);
        break;
    case 'l':
        result = set_leds(args, arg);
        break;
    case 'r':
        result = set_row(args, atoi(arg));
        break;
    case ARGP_KEY_ARG:
        result = add_line(args, arg);
        break;
    case ARGP_KEY_END:
        size = get_size(args);
        if (!args->clear && (args->row < 0) && (size == 0))
        {
            fprintf(stderr, "Nothing to do. Please provide text and/or commands.\n\n");
            result = 1;
        }
        if ((args->row >= 0) && (size == 0))
        {
            fprintf(stderr, "Please provide a text for row.\n\n");
            result = 1;
        }
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }

    if (result)
    {
        argp_usage(state);
    }

    return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc};

//------------------------------------------------------------------------------------------------------------
//
// LCD Update Function:
//
//------------------------------------------------------------------------------------------------------------

void lcd_put_row(uint8_t row, unsigned char *line)
{
    static unsigned char full_line[LCD_COL];
    memset(full_line, ' ', LCD_COL);
    memcpy(full_line, line, strlen(line));

    lcdPosition(lcdHandle, 0, row);
    for (uint8_t j = 0; j < LCD_COL; j++)
    {
        lcdPutchar(lcdHandle, full_line[j]);
    }
}

static void lcd_update(Arguments *args)
{
    if (args->clear)
    {
        lcdClear(lcdHandle);
    }

    if (args->row >= 0)
    {
        lcd_put_row(args->row, args->lines[0]);
    }
    else
    {
        uint8_t l = 0;
        while (args->lines[l] != NULL)
        {
            lcd_put_row(l, args->lines[l]);
            l++;
        }
    }
}

//------------------------------------------------------------------------------------------------------------
//
// board data update
//
//------------------------------------------------------------------------------------------------------------
void led_update(Arguments *args)
{
    if (args->leds == NULL)
        return;

    int value;
    for (uint8_t l = 0; l < strlen(args->leds); l++)
    {
        if (args->leds[l] != '-')
        {
            value = args->leds[l] == '1' ? 1 : 0;
            digitalWrite(ledPorts[l], value);
        }
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
    init(&args);
    argp_parse(&argp, argc, argv, 0, 0, &args);

    wiringPiSetup();

    if (system_init() < 0)
    {
        fprintf(stderr, "%s: System Init failed\n", __func__);
        return -1;
    }

    // board update
    led_update(&args);
    lcd_update(&args);

    return 0;
}

//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------
