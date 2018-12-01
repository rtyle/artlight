#ifndef TimeUpdate_h_
#define TimeUpdate_h_

#include <string>
#include <vector>

/// A TimeUpdate object configures and starts a polling SNTP client
/// that will run (in the LwIP task) for its lifetime.
/// Without notice (boo!), time will be set (jumped) using settimeofday.
/// As there is no consideration for how long it takes for the SNTP server's
/// response to reach us, it is best if the server is close (response is fast).
class TimeUpdate {
private:
    char const * const name;

public:
    TimeUpdate(
	char const * name_,
	std::vector<std::string> & timeUpdateServers);

    ~TimeUpdate();
};

#endif
