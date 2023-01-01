
#define L9110S_1B 25
#define L9110S_1A 26

enum clockState_t
{
    RUNNING,
    STOPPED,
    ADDING,
    WAITING,
};

void initClock();
void tickClock();
String getClockState();
void setClockState(clockState_t state);
void setClockState(clockState_t state, int seconds);