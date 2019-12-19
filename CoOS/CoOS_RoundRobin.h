/*
 * CoOS_RoundRobin.h
 *
 *  Created on: 12/10/2019
 *      Author: AdrianDesk
 */

#ifndef SRC_COOS_COOS_ROUNDROBIN_H_
#define SRC_COOS_COOS_ROUNDROBIN_H_


#include "CoOS_Conf.h"
#include "CoOS_Thread.h"



namespace CoOS {


class RoundRobin {

public:

   static void AddThread(Thread* add);
   static void Init();
   static void PendSV_Handler();

   static void SwitchThreads();

private:
   static uint32_t count;

};

} /* namespace CoOS */

#endif /* SRC_COOS_COOS_ROUNDROBIN_H_ */
