#include <msp430.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>

#define GREEN_LED BIT6

AbRect mainRect = {abRectGetBounds, abRectCheck, {3, 3}};
AbRect rect1 = {abRectGetBounds, abRectCheck, {3,10}};

AbRectOutline fieldOutline = {
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2, screenHeight/2} 
};



Layer frog = {
  (AbShape *)&mainRect,
  {(screenWidth-20), (screenHeight/2)}, 
  {0,0}, {0,0},	
  COLOR_BLUE,
  0
};
Layer fieldLayer = {	
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2},
  {0,0}, {0,0},			
  COLOR_WHITE,
  &frog,
};

Layer layerX = {		
  (AbShape *)&rect1,
  {(screenWidth/2), screenHeight/2}, 
  {0,0}, {0,0},			
  COLOR_WHITE,
  &fieldLayer,
};

Layer enemy = {
  (AbShape *)&rect1,
  {screenWidth/2, screenHeight/2}, 
  {0,0}, {0,0},		
  COLOR_RED,
  &fieldLayer,
};



typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

MovLayer mlfrog = { &frog, {0,0}, 0 }; 
MovLayer mlx = { &layerX, {0,0}, &mlfrog }; 
MovLayer ml0 = { &enemy, {0,3}, &mlx }; 


MovLayer moveLeft = { &frog, {-3,0}, &mlfrog }; 
MovLayer moveRight = { &frog, {3,0}, &mlfrog }; 
MovLayer moveUp = { &frog, {0,-3}, &mlfrog }; 
MovLayer moveDown = { &frog, {0,3}, &mlfrog }; 

void movLayerDraw(MovLayer *movLayers, Layer *layers)
{
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);		
  for (movLayer = movLayers; movLayer -> next; movLayer = movLayer->next) { 
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);		


  for (movLayer = movLayers; movLayer -> next; movLayer = movLayer->next) { 
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], 
		bounds.botRight.axes[0], bounds.botRight.axes[1]);
    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
	Vec2 pixelPos = {col, row};
	u_int color = bgColor;
	Layer *probeLayer;
	for (probeLayer = layers; probeLayer; 
	     probeLayer = probeLayer->next) { 
	  if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
	    color = probeLayer->color;
	    break; 
	  } 
	} 
	lcd_writeColor(color); 
      } 
    } 
  } 
}	  


/** Advances a moving shape within a fence
 *  
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */
void mlAdvance(MovLayer *ml, Region *fence, MovLayer *mlfrog) {
	u_char axis;
	Region shapeBoundary;
	Region frog;

	
		Vec2 newPos;
	Vec2 currPos;
	vec2Add(&currPos, &mlfrog->layer->posNext, &mlfrog->velocity);
	abShapeGetBounds(mlfrog->layer->abShape, &currPos, &frog);

	for (; ml -> next -> next; ml = ml->next) {
    
		vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    	abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);

		for (axis = 0; axis < 2; axis ++) {
			if (shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) {
				
				int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
				newPos.axes[axis] += (2*velocity);
			}	
			
			if (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis])  {
				
				int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
				newPos.axes[axis] += (2*velocity);
			}	
			
			if (shapeBoundary.botRight.axes[0] > frog.topLeft.axes[0]) {
				if (frog.botRight.axes[0] > shapeBoundary.topLeft.axes[0]) {
                    if (shapeBoundary.botRight.axes[1] > frog.topLeft.axes[1]) {
                        if (frog.botRight.axes[1] > shapeBoundary.topLeft.axes[1]) {
                
                            drawString5x7(screenWidth/2,screenHeight/2, "you lose", COLOR_RED, COLOR_WHITE);
                            //reminder to change colors on this since its imposible to see
                            WDTCTL = 0;
                        }
                    }
                }
            }
        }

    	ml->layer->posNext = newPos;
  	} 

  	
  	
  	//COLLISIONS
	vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
	abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
	if (shapeBoundary.topLeft.axes[1] < fence->topLeft.axes[0]) {

		newPos.axes[0] = screenWidth - 20;
		newPos.axes[1] = screenHeight/2;

	}
	
	if (shapeBoundary.botRight.axes[1] > fence->botRight.axes[1]) {

		newPos.axes[0] = screenWidth - 20;
		newPos.axes[1] = screenHeight/2;

	}
	
	if (shapeBoundary.botRight.axes[0] > fence->botRight.axes[1]) {

		newPos.axes[0] = screenWidth - 20;
		newPos.axes[1] = screenHeight/2;

	}	
	if (shapeBoundary.topLeft.axes[0] < fence->topLeft.axes[0]) {
		newPos.axes[0] = screenWidth - 20;
		newPos.axes[1] = screenHeight/2;
	}
    ml->layer->posNext = newPos;
}


u_int bgColor = COLOR_WHITE;   
int redrawScreen = 1;     

Region fieldFence;		
Region frogRegion;


void main()
{
  P1DIR |= GREEN_LED;			
  P1OUT |= GREEN_LED;

  configureClocks();
  lcd_init();
  shapeInit();
  p2sw_init(15); 

  shapeInit();

  layerInit(&enemy);
  layerDraw(&enemy);
  layerGetBounds(&fieldLayer, &fieldFence);
  
  layerGetBounds(&frog, &frogRegion);


  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */


  for(;;) { 
    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
      or_sr(0x10);	      /**< CPU OFF */
    }
    P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
    redrawScreen = 0;
    movLayerDraw(&ml0, &enemy);
  }



}


void wdt_c_handler()
{
	char buttonPressed = 15 - p2sw_read();
    
    
    
    
    //char buttonPressed = p2sw_read();
    //ask*************************************************************
    
  static short count = 0;
  P1OUT |= GREEN_LED;		      /**< Green LED on when cpu on */
  count ++;
  if (count == 50) {
      //chang back maybe***********************

	  switch (buttonPressed) {	
          
		case 1:
			mlAdvance(&moveLeft, &fieldFence, &mlfrog);
			movLayerDraw(&moveLeft, &frog);
			break;
		case 2:
			mlAdvance(&moveRight, &fieldFence, &mlfrog);
			movLayerDraw(&moveRight, &frog);
			break;
		case 4:
			mlAdvance(&moveUp, &fieldFence, &mlfrog);
			movLayerDraw(&moveUp, &frog);
			break;
		case 8:
			mlAdvance(&moveDown, &fieldFence, &mlfrog);
			movLayerDraw(&moveDown, &frog);
			break;
	  }
    mlAdvance(&ml0, &fieldFence, &mlfrog);
    if (p2sw_read())
      redrawScreen = 1;
    count = 0;
  } 
  P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
}
