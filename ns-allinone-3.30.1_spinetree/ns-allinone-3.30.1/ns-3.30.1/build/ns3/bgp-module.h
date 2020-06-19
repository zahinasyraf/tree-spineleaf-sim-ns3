
#ifdef NS3_MODULE_COMPILATION
# error "Do not include ns3 module aggregator headers from other modules; these are meant only for end user scripts."
#endif

#ifndef NS3_MODULE_BGP
    

// Module headers:
#include "bgp-log.h"
#include "bgp-ns3-clock.h"
#include "bgp-ns3-fsm.h"
#include "bgp-ns3-socket-in.h"
#include "bgp-ns3-socket-out.h"
#include "bgp-routing.h"
#include "bgp.h"
#endif
