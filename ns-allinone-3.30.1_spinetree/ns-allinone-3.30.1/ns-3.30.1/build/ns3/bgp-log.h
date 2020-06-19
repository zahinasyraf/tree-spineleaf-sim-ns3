
#ifndef BGP_LOG_H
#define BGP_LOG_H
#include <libbgp/bgp-log-handler.h>
#include "ns3/simple-ref-count.h"

namespace ns3
{

/**
 * @brief The libbgp log forwarder class.
 * 
 */
class BgpLog : public libbgp::BgpLogHandler, public SimpleRefCount<BgpLog>
{
public:
    BgpLog(const char *owner);
    ~BgpLog();

    void SetOwner(const char *owner);

protected:
    void logImpl(const char *str);
    char *owner;
};

} // namespace ns3
#endif // BGP_LOG_H