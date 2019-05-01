/** \file shapemotion.c
 *  \brief This is a simple shape motion demo.
 *  This demo creates two layers containing shapes.
 *  One layer contains a rectangle and the other a circle.
 *  While the CPU is running the green LED is on, and
 *  when the screen does not need to be redrawn the CPU
 *  is turned off along with the green LED.
 */  
#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>

#define GREEN_LED BIT6
/*
AbRect bar = {abRectGetBounds, abRectCheck,
	      {screenWidth/2 - 10, 20}};
*/

AbRectOutline bar = {
  abRectOutlineGetBounds, abRectOutlineCheck, {screenWidth/2 - 10, 15}
};

AbRect rect10 = {abRectGetBounds, abRectCheck, {10,10}}; /**< 10x10 rectangle */
//AbRArrow rightArrow = {abRArrowGetBounds, abRArrowCheck, 30};

//my dumbass testing things
AbRect rect11 = {abRectGetBounds, abRectCheck, {10,10}};
AbRect rect12 = {abRectGetBounds, abRectCheck, {10,10}};
AbRect rect13 = {abRectGetBounds, abRectCheck, {10,10}};

AbRectOutline fieldOutline = {	/* playing field */
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2 - 10, screenHeight/2 - 10}
};

//Layer layer4 = {
//  (AbShape *)&rightArrow,
// {(screenWidth/2)+10, (screenHeight/2)+5}, /**< bit below & right of center */
//  {0,0}, {0,0},				    /* last & next pos */
//  COLOR_PINK,
// 0
//};
  

//Layer layer3 = {		/**< Layer with an orange circle */
  //  (AbShape *)&circle8,
  //  {(screenWidth/2)+10, (screenHeight/2)+5}, /**< bit below & right of center */
  //  {0,0}, {0,0},				    /* last & next pos */
  //  COLOR_VIOLET,
  //  &layer4,
//};

Layer fieldLayer = {		/* playing field as a layer */
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2},/**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_BLACK,
  0
};


Layer barLayer = {
  (AbShape *)&bar,
  {screenWidth/2, 120},
  {0,0}, {0,0},
  COLOR_RED,
  //  &fieldLayer,
  0
};



//Layer layer1 = {		/**< Layer with a red square */
//(AbShape *)&rect10,
//{screenWidth/2, screenHeight/2}, /**< center */
//{0,0}, {0,0},				    /* last & next pos */
//COLOR_RED,
//&fieldLayer,
//};

//more of my shit that might not work

Layer layer3 = {
  (AbShape *)&rect13,
  {(screenWidth/2)-33, (screenHeight/2 + 5)},
  {0,0}, {0,0},
  COLOR_GREEN,
  &barLayer,
};

Layer layer2 = {
  (AbShape *)&rect12,
  {(screenWidth/2)-11, (screenHeight/2 + 5)},
  {0,0}, {0,0},
  COLOR_YELLOW,
  &layer3,
};

Layer layer1 = {
  (AbShape *)&rect11,
  {(screenWidth/2)+11, (screenHeight/2) + 5},
  {0,0}, {0,0},
  COLOR_RED,
  &layer2,
};
//this is where my bullshit ends


Layer layer0 = {		/**< Layer with yellow square*/
  (AbShape *)&rect10,
  {(screenWidth/2)+33, (screenHeight/2)+5}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_BLUE,
  &layer1,
};

/** Moving Layer
 *  Linked list of layer references
 *  Velocity represents one iteration of change (direction & magnitude)
 */
typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

/* initial value of {0,0} will be overwritten */
MovLayer ml3 = { &layer3, {0,4}, 0 };
MovLayer ml2 = { &layer2, {0,2}, &ml3 }; /**< not all layers move */
MovLayer ml1 = { &layer1, {0,5}, &ml2 }; 
MovLayer ml0 = { &layer0, {0,2}, &ml1 }; 

void movLayerDraw(MovLayer *movLayers, Layer *layers)
{
  int row, col;
  MovLayer *current;

  and_sr(~8);			/**< disable interrupts (GIE off) */
  for (current = movLayers; current; current = current->next) { /* for each moving layer */
    Layer *l = current->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);			/**< disable interrupts (GIE on) */


  for (current = movLayers; current; current = current->next) { /* for each moving layer */
    Region bounds;
    layerGetBounds(current->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], 
		bounds.botRight.axes[0], bounds.botRight.axes[1]);
    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
	Vec2 pixelPos = {col, row};
	u_int color = bgColor;
	Layer *probeLayer;
	for (probeLayer = layers; probeLayer; 
	     probeLayer = probeLayer->next) { /* probe all layers, in order */
	  if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
	    color = probeLayer->color;
	    break; 
	  } /* if probe check */
	} // for checking all layers at col, row
	lcd_writeColor(color); 
      } // for col
    } // for row
  } // for moving layer being updated
}	  



Region fence = {{0,0}, {SHORT_EDGE_PIXELS, LONG_EDGE_PIXELS}}; /**< Create a fence region */
//changing the stuff inside the first curly bois from 10, 30 to 0,0 does nothing

/** Advances a moving shape within a fence
 *  
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */
void mlAdvance(MovLayer *ml, Region *fence)
{
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    //    for (axis = 1; axis < 2; axis ++) {
    if (/*(shapeBoundary.topLeft.axes[1] < fence->topLeft.axes[1]) ||*/
	  (shapeBoundary.botRight.axes[1] > fence->botRight.axes[1]) ) {
	//int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	int velocity = ml->velocity.axes[1];
	newPos.axes[1] = -50;//commenting this out does nothing
	//newPos.axes[axis] += 
      }	/**< if outside of fence */
      //} /**< for axis */
    ml->layer->posNext = newPos;
  } /**< for ml */
}


u_int bgColor = COLOR_WHITE;     /**< The background color */
int redrawScreen = 1;           /**< Boolean for whether screen needs to be redrawn */

Region fieldFence;		/**< fence around playing field  */


/** Initializes everything, enables interrupts and green LED, 
 *  and handles the rendering for the screen
 */
void main()
{
  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */		
  P1OUT |= GREEN_LED;

  configureClocks();
  lcd_init();
  shapeInit();
  p2sw_init(1);

  shapeInit();

  layerInit(&layer0);
  layerDraw(&layer0);


  layerGetBounds(&fieldLayer, &fieldFence);


  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */


  for(;;) { 
    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
      or_sr(0x10);	      /**< CPU OFF */
    }
    P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
    redrawScreen = 0;
    movLayerDraw(&ml0, &layer0);
  }
}

/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler()
{
  static short count = 0;
  P1OUT |= GREEN_LED;		      /**< Green LED on when cpu on */
  count ++;
  if (count == 15) {
    mlAdvance(&ml0, &fieldFence);
    if (p2sw_read())
      redrawScreen = 1;
    count = 0;
  } 
  P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
}
