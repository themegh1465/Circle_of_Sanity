#ifndef TapeService_H  // <- This should be changed to your own guard on both
#define TapeService_H  //    of these lines


/*******************************************************************************
 * PUBLIC #INCLUDES                                                            *
 ******************************************************************************/

#include "ES_Configure.h"   // defines ES_Event, INIT_EVENT, ENTRY_EVENT, and EXIT_EVENT

/*******************************************************************************
 * PUBLIC #DEFINES                                                             *
 ******************************************************************************/


/*******************************************************************************
 * PUBLIC TYPEDEFS                                                             *
 ******************************************************************************/


/*******************************************************************************
 * PUBLIC FUNCTION PROTOTYPES                                                  *
 ******************************************************************************/

 

uint8_t InitTapeService(uint8_t Priority);


uint8_t PostTapeService(ES_Event ThisEvent);


ES_Event RunTapeService(ES_Event ThisEvent);

//return the last debounced tape sensor bit mask
uint8_t TapeService_GetBits(void);



#endif

