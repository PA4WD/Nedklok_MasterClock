
#define L9110S_1B 25
#define L9110S_1A 26

#define DS3231_SDA 21
#define DS3231_SCL 22

enum clockState_t
{
    RUNNING,
    STOPPED,
    ADDING,
    WAITING,
};

void initClock();
void tickSecond();
String getClockState();
void setClockState(clockState_t state);
void setClockState(clockState_t state, int seconds);
float getTemperature();