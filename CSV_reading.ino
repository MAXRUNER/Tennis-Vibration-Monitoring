#include <Adafruit_LittleFS.h>
#include <Adafruit_TinyUSB.h>
#include <InternalFileSystem.h>

using namespace Adafruit_LittleFS_Namespace;

void setup() {
  Serial.begin(115200);

  while (!Serial) {
    delay(10);
  }

  if (!InternalFS.begin()) {
    Serial.println("ERROR: Could not mount InternalFS");
    return;
  }

  File f(InternalFS);

  if (!f.open("/data.csv", FILE_O_READ)) {
    Serial.println("ERROR: Could not open /data.csv");
    return;
  }

  Serial.println("-----BEGIN CSV-----");

  while (f.available()) {
    Serial.write(f.read());
  }

  Serial.println();
  Serial.println("-----END CSV-----");

  f.close();
}

void loop() {
}