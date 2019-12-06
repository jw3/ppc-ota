#include <ppc.h>
#include <otafwu.h>

#ifndef BROKER
#define BROKER "raspberrypi"
#endif

SerialLogHandler logHandler;
ppc::MQTTCloud Cloud(BROKER);

void setup() {
   EnableOTA(Cloud);
}

void doWork() {
}

void loop() {
   // control when its a good time to update
   doWork();
   doWork();
   doWork();
   doWork();
}
