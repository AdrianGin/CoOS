/*
 * Tests_Signal.cpp
 *
 *  Created on: 12/10/2019
 *      Author: AdrianDesk
 */

#include <CoOS_Signal.h>

namespace CoOS
{
SignalFlags Signal1;

void Tests_Signal() {
   if( Signal1.Get() == 0x01 ) {
      Signal1.Clear(0x01);
   }
}




//Call from ISR
void ISR_SignalTest()
{
   Signal1.Set(0x01);
}


}
