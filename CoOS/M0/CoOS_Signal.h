/*
 * CoOS_Signal.h
 *
 *  Created on: 12/10/2019
 *      Author: AdrianDesk
 */

#ifndef SRC_COOS_COOS_SIGNAL_H_
#define SRC_COOS_COOS_SIGNAL_H_

#include "CoOS_Conf.h"
#include "core_cm0.h"

namespace CoOS {

class SignalFlags {

public:
   typedef uint32_t Flags;

   SignalFlags() : state(0) {}

   inline void Set(Flags mask) {
	   state |= mask;
   }

   inline void Clear(Flags mask) {
	   state &= ~mask;
   }

   inline Flags Get() {
      return state;
   }

   inline void Reset() {
      Clear(0xFFFFFFFF);
   }


private:
   Flags state;


};


} /* namespace CoOS */

#endif /* SRC_COOS_COOS_SIGNAL_H_ */
