#ifndef DEFINES_H_
#define DEFINES_H_

#define VERSION "2.0.0"

#define DEBUG 1
#if DEBUG == 1

#define debug(x) Serial.print("[D] "); Serial.print(x)
#define debugln(x) Serial.print("[DEBUG]\t"); Serial.print(x); Serial.println()
#define debugf(fmt, ...) \
    Serial.print("[D] "); \
    Serial.printf(fmt, __VA_ARGS__)

#else
#define debug(x)
#define debugln(x)
#endif

#define info(x) Serial.print("[L]"); Serial.print(x)
#define infoln(x) Serial.print("[LOG]\t"); Serial.print(x); Serial.println()
#define infof(fmt, ...) \
    Serial.print("[L] "); \
    Serial.printf(fmt, __VA_ARGS__)

#define warn(x) Serial.print("[W] "); Serial.print(x)
#define warnln(x) Serial.print("[WARN]\t"); Serial.print(x); Serial.println()
#define warnf(fmt, ...) \
    Serial.print("[W] "); \
    Serial.printf(fmt, __VA_ARGS__)

#define error(x) Serial.print("[E] "); Serial.print(x)
#define errorln(x) Serial.print("[ERROR]\t"); Serial.print(x); Serial.println()
#define errorf(fmt, ...) \
    Serial.print("[E] "); \
    Serial.printf(fmt, __VA_ARGS__)

#define MAXBATERYVOLTAGE 4.00 // under load 4.2 -> 4.0
#define MINBATERYVOLTAGE 3.40
#define BATTERYSTABTIME \
    1000 // waiting time to stabilize battery voltage after diceState change
#define MOVINGTHRESHOLD \
    0.7                      // maximum acceleration magnitude to detect
                             // nonMoving

#define REGULATOR_PIN GPIO_NUM_18 // pin D9
#define BUTTON_PIN GPIO_NUM_14

#endif /* DEFINES_H_ */
