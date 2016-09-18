/*****************************************************************************
* BreakOut for GBA.
* Author: Ryoga a.k.a. JDURANMASTER
* This module is a sample BreakOut-Clone.
*
******************************************************************************
*    - Testing GBA mode 4, GBA mode 3, double buffering, keyboard controlling.
*    - No sound Effects implemented yet.
*
******************************************************************************/  

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "font.h"

#include "gba.h"         //GBA register definitions.
#include "fade.h"        //background fades.
#include "keypad.h"      //keypad defines
#include "dispcnt.h"     //REG_DISPCNT register defines
#include "dma.h"         //dma defines.
#include "dma.c"         //dma copy function.
#include "util.h"         //utils defines.
#include "util.c"         //utils functions.

//gfx
#include "Rlogo.h"       //8-bit Ryoga Logo
#include "HKlogo.h"      //8-bit HK logo
#include "saludIntro.h"  //8-bit Health logo.
#include "tabmainlogo.h" //main screen of the game.

//some useful colors
#define BLACK 0x0000
#define WHITE 0xFFFF
#define BLUE 0xEE00
#define CYAN 0xFF00
#define GREEN 0x0EE0
#define RED 0x00FF
#define MAGENTA 0xF00F
#define BROWN 0x0D0D

//defines for the video system
//unsigned short* videoBuffer = (unsigned short*)0x6000000;
#define DISPLAY_CONTROLLER *(unsigned long*)0x4000000
#define VIDC_BASE_HALF	((volatile unsigned short int*) 0x04000000)	
#define VCOUNT		(VIDC_BASE_HALF[3])	

unsigned short* ScreenBuffer = (unsigned short*)0x6000000;
unsigned short* ScreenPal = (unsigned short*)0x5000000;

//pointer to the button interface
volatile unsigned int *BUTTONS = (volatile unsigned int *)0x04000130;

#define VIDEO_MODE_3 0x3
#define BACKGROUND2 0x400
#define SCREEN_W 240
#define SCREEN_H 160

//game constants
#define BALL_SIZE 3
#define PADDLE_WIDTH 30
#define PADDLE_HEIGHT 8
#define PADDLE_SPEED 4
#define BLOCK_WIDTH 20
#define BLOCK_HEIGHT 10

//global variables
int paddleX, paddleY;
int ballX = 120, ballY = 139;
int velX = 2, velY = -1;
int score = 0;
int state=0;

int blocks[10][6];

void setMode(int mode)
{
    DISPLAY_CONTROLLER = mode | BACKGROUND2;
}

//clear VideoBuffer with some unused VRAM
void ClearBuffer()
{
	REG_DM3SAD = 0x06010000;//Source Address - Some unused VRAM
	REG_DM3DAD = 0x06000000;//Destination Address - Front buffer
	REG_DM3CNT = DMA_ENABLE | DMA_SOURCE_FIXED | 19200;
}

inline void drawPixel(int x, int y, unsigned short color)
{
	videoBuffer[y * 240 + x] = color;
}

inline unsigned short getpixel(int x, int y)
{
    return videoBuffer[y * 240 + x];
}


int buttonPressed(int button)
{
    //see if UP button is pressed
    if (!((*BUTTONS) & button))
        return 1;
    else
        return 0;
}

//draw text using characters contained in font.h
void print(int left, int top, char *str, unsigned short color)
{
    int x, y, draw;
    int pos = 0;
    char letter;

    //look at all characters in this string
    while (*str)
    {
        //get current character ASCII code
        letter = (*str++) - 32;

        //draw the character
        for(y = 0; y < 8; y++)
            for(x = 0; x < 8; x++)
            {
                //grab a pixel from the font character
                draw = font[letter * 64 + y * 8 + x];

                //if pixel = 1, then draw it
                if (draw)
                    drawPixel(left + pos + x, top + y, color);
            }

        //jump over 8 pixels
        pos += 8;
    }
}

void drawbox(int left, int top, int right, int bottom, unsigned short color)
{
   int x = left, y;
   int length = (right - left) << 1;
   for (y = top; y < bottom; y++) {
       for (x = left; x < right; x++)
            drawPixel(x, y, color);
   }
}

//Wait until the start key is pressed
void WaitForStart()
{
	
	u8 t=0;//used for detecting a single press
	
	while (1)
	if( KEY_DOWN(KEYSTART) )
	{
		t++;
		if(t<2){
			return;
		}
	}
	else{
		t = 0;
	}
}

//Wait until the A key is pressed
void WaitForAButton()
{
	
	u8 t=0;//used for detecting a single press
	
	while (1)
	if( KEY_DOWN(KEYA) )
	{
		t++;
		if(t<2){
			return;
		}
	}
	else{
		t = 0;
	}
}

void printScore()
{
    //erase title/score line
    drawbox(0,0,SCREEN_W-1,10,BLACK);

    //display title
    print(SCREEN_W/2-32,1,"BREAKOUT", RED);

    //display score
    char s[5];
    sprintf(s,"%i",score);
    print(5,1,s,WHITE);
}

void updateBall()
{
    if (ballX < 1 || ballX > SCREEN_W - BALL_SIZE - 1)
    {
        velX *= -1;
    }

    if (ballY < 9)
    {
        velY *= -1;
    }

    if (ballY > SCREEN_H - BALL_SIZE - 1)
    {
        velY *= -1;
    }

    ballX += velX;
    ballY += velY;
}

void drawBall()
{
	drawbox(ballX, ballY, ballX + BALL_SIZE, ballY + BALL_SIZE, RED);
}

void eraseBall()
{
	drawbox(ballX, ballY, ballX + BALL_SIZE, ballY + BALL_SIZE, BLACK);
}

void drawBlock(int row, int col, int color)
{
    int startx,starty;

    startx = 10 + row * (BLOCK_WIDTH+2);
    starty = 20 + col * (BLOCK_HEIGHT+2);
    drawbox(startx, starty, startx + BLOCK_WIDTH, starty + BLOCK_HEIGHT, color);
}

void drawBlocks()
{
    int x,y;

	for (y = 0; y < 6; y++)  {
		for (x = 0; x < 10; x++)  {

			if (blocks[x][y] == 1 && 0 < x < 10 && y == 0) {
				drawBlock(x,y, MAGENTA);
           	}

           	if (blocks[x][y] == 1 && 10 < x < 20 && y == 1) {
               drawBlock(x,y, RED);
           	}
           
           	if (blocks[x][y] == 1 && 20 < x < 30 && y == 2) {
               	drawBlock(x,y, CYAN);
           	}
           
           	if (blocks[x][y] == 1 && 30 < x < 40 && y == 3) {
               	drawBlock(x,y, GREEN);
           	}
           
           	if (blocks[x][y] == 1 && 40 < x < 50 && y == 4) {
               	drawBlock(x,y, BLUE);
           	}
           
           	if (blocks[x][y] == 1 && 50 < x < 60 && y == 5) {
               	drawBlock(x,y, BROWN);
           	}
		}
	}
}

void drawPaddle()
{
	drawbox(paddleX, paddleY, paddleX + PADDLE_WIDTH, paddleY + PADDLE_HEIGHT, WHITE);
}

void erasePaddle()
{
	drawbox(paddleX, paddleY, paddleX + PADDLE_WIDTH, paddleY + PADDLE_HEIGHT, BLACK);
}

void updatePaddle()
{
    //check for LEFT button press
    if (buttonPressed(32))
    {
        if (paddleX > 1)
            paddleX -= PADDLE_SPEED;
    }

    //check for RIGHT button press
    if (buttonPressed(16))
    {
        if (paddleX < SCREEN_W - PADDLE_WIDTH - 2)
            paddleX += PADDLE_SPEED;
    }
}

int inside(int x,int y, int left, int top, int right, int bottom)
{
    return (x > left && x < right && y > top && y < bottom);
}

void testPaddleCollision()
{
    int x1 = paddleX;
    int y1 = paddleY;
    int x2 = paddleX + PADDLE_WIDTH;
    int y2 = paddleY + PADDLE_HEIGHT;
    int bx = ballX + BALL_SIZE/2;
    int by = ballY + BALL_SIZE/2;
    
    if (inside(bx,by,x1,y1,x2,y2)) {
        velY *= -1;
        ballY += velY;
    }
}

void testBlockCollisions()
{
    int x,y,x1,y1,x2,y2,bx,by;

    for (y = 0; y < 6; y++)  {
        for (x = 0; x < 10; x++)  {

            if (blocks[x][y] == 1) {
    
                x1 = 10 + x * (BLOCK_WIDTH+2);
                y1 = 20 + y * (BLOCK_HEIGHT+2);
                x2 = x1 + BLOCK_WIDTH;
                y2 = y1 + BLOCK_HEIGHT;
                bx = ballX + BALL_SIZE/2;
                by = ballY + BALL_SIZE/2;

                if (inside(bx, by, x1, y1, x2, y2)) {
                    score++;
                    drawBlock(x,y,BLACK);
                    blocks[x][y] = 0;
                    velY *= -1;
                    ballY += velY;
                }
            }

        }
    }
}

void waitRetrace()
{
	while (VCOUNT != 160);
	while (VCOUNT == 160);
}

void clearScreen() {
    drawbox(0,0,239,159,0x0000);
}

// show the game intro.
void showGameIntro()
{
	int loop;
	
	EraseScreen();
	
	// logo - saludIntro
	for(loop=0;loop<256;loop++) {
      	ScreenPal[loop] = saludIntroPalette[loop];     
   	}

   	for(loop=0;loop<19200;loop++) {
      	ScreenBuffer[loop] = saludIntroData[loop];
   	}
	
    WaitForVblank();
	Flip();
	Sleep(2500);
	EraseScreen();
   	
	// logo Hammer Keyboard Studios.
	for(loop=0;loop<256;loop++) {
      	ScreenPal[loop] = HKlogoDataPalette[loop];     
   	}

   	for(loop=0;loop<19200;loop++) {
      	ScreenBuffer[loop] = HKlogoDatadata[loop];
   	}
	
   	WaitForVblank();
	Flip();
	Sleep(2000);
	EraseScreen();
   	
	// logo - Rlogo
	for(loop=0;loop<256;loop++) {
      	ScreenPal[loop] = RlogoPalette[loop];     
   	}

   	for(loop=0;loop<19200;loop++) {
      	ScreenBuffer[loop] = Rlogodata[loop];
   	}
	
   	WaitForVblank();
	Flip();
	Sleep(2000);
	EraseScreen();
	
	// logo - Main Screen
	for(loop=0;loop<256;loop++) {
      	ScreenPal[loop] = tabmainlogoPalette[loop];     
   	}

   	for(loop=0;loop<19200;loop++) {
      	ScreenBuffer[loop] = tabmainlogodata[loop];
   	}
	
   	WaitForVblank();
	Flip();
	Sleep(2000);
}

void selectGameMode()
{
	state = 0;
	
	drawbox(0, 0, 239, 159, BLACK);
		
	print(23-16,1,"BREAKOUT IS AN ARCADE GAME", WHITE);
	print(23-16,11,"DEV AND PUBLISHED BY ATARI.", WHITE);
	print(23-16,22,"ITWAS CONCEPTUALIZED BY NOLAN", WHITE);
	print(23-16,33,"BUSHELL AND STEVE BRISTOW", WHITE);
	print(23-16,44,"INFLUENCED BY THE 1972 ATARI", WHITE);
	print(23-16,55,"GAME PONG AND BUILT BY STEVE", WHITE);
	print(23-16,66,"WOZNIAK.", WHITE);
	
	print(23-16,77,"SELECT THE DIFFICULTY LEVEL.", MAGENTA);
	
	
	print(30-16,100,"*", WHITE);
	print(38-16,100,"LEVEL 1. HURT ME PLENTY ", GREEN);
	print(38-16,110,"LEVEL 2. ULTRA-VIOLENCE", RED);
	
	print(110-16,130,"2016", WHITE);
	print(25-16,140,"PROGRAMMED BY JDURANMASTER", WHITE);
	
	while((((KEYS) & KEYA))){
		if(!((KEYS) & KEYUP)){
			if(state == 0){
				print(30-16,110,"*", WHITE);
				print(30-16,100,"*", BLACK);
				state = 1;
			}
			else{
				print(30-16,100,"*", WHITE);
				print(30-16,110,"*", BLACK);
				state = 0;
			}	
		}
		if(!((KEYS) & KEYDOWN)){
			if(state == 1){
				print(30-16,100,"*", WHITE);
				print(30-16,110,"*", BLACK);
				state = 0;
			}
			else{
				print(30-16,110,"*", WHITE);
				print(30-16,100,"*", BLACK);
				state = 1;
			}	
		}
	}
}

int main(void)
{
	//Enable background 2 and set mode to MODE_4
	setMode(MODE_4 | OBJ_MAP_1D | BG2_ENABLE);
	
	showGameIntro();
	WaitForStart();
	EraseScreen();
	
	//clear buffer
	ClearBuffer();
	
	//Change video mode to MODE_3 with background 2 enabled
	setMode(MODE_3 | BG1_ENABLE);
	
	selectGameMode();
	
	//clear the screen
    drawbox(0, 0, 239, 159, BLACK);
    
	//print(23-16,1,"GOOD, LITTLEBOY.BUT YOU MUST", RED);
	//print(23-16,11,"BEAT THE ULTRAVIOLENCE MODE", RED);
	//print(23-16,22,"TO STAND A CHANCE.", RED);
	//while(1);
	
	//clear buffer
	ClearBuffer();
	
    //init video mode 3 (240x160)
    setMode(VIDEO_MODE_3 | OBJ_MAP_1D | BG2_ENABLE);

    //init the left paddle
    paddleX = 120;
    paddleY = SCREEN_H - PADDLE_HEIGHT - 5;
    
    //turn on the blocks so they are all visible
    int x,y;
    for (y = 0; y < 6; y++)
        for (x = 0; x < 10; x++)
            blocks[x][y] = 1;

    //draw all the blocks
    drawBlocks();

    //game loop
    while(1)
    {
        waitRetrace();

        erasePaddle();
        eraseBall();
        
        updatePaddle();
        testPaddleCollision();
        drawPaddle();
        
        updateBall();
        testBlockCollisions();
        drawBall();

        printScore();
    }

    return 0;
}

