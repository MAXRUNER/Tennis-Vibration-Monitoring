#include <Adafruit_LittleFS.h>
#include <Adafruit_TinyUSB.h>
#include <InternalFileSystem.h>
#include <arduinoFFT.h>
#include <math.h>
#include <string.h>

using namespace Adafruit_LittleFS_Namespace;

const int PIEZO = A0;
const uint16_t N = 1024;
const uint32_t SAMPLE_RATE = 10000;
const uint16_t SESSION_SIZE = 150;
const uint8_t MAX_SESSIONS = 7;
const uint32_t COOLDOWN = 10UL * 60 * 60 * 1000;
const int TRIGGER = 8;

double realData[N], imagData[N];
ArduinoFFT<double> FFT(realData, imagData, N, SAMPLE_RATE);

struct State {
  uint32_t magic;
  uint8_t session, stage, cooling;
  uint16_t valid;
  uint32_t count;
  double previous, pending;
  uint32_t pendingTime;
} state;

const uint32_t MAGIC = 0x54454E4E;
const char *CSV = "/data.csv";
const char *STATE = "/state.bin";
bool storageOK = false;
bool armed = true;
double baseline = 0;
uint32_t lastHit = 0, quietSince = 0, cooldownStarted = 0;

void newState() {
  memset(&state, 0, sizeof(state));
  state.magic = MAGIC;
  state.session = 1;
}

bool saveState() {
  if (!storageOK)
    return false;
  File f(InternalFS);
  if (!f.open(STATE, FILE_O_WRITE))
    return false;
  bool ok = f.write((uint8_t *)&state, sizeof(state)) == sizeof(state);
  f.close();
  return ok;
}

void loadState() {
  newState();
  File f(InternalFS);
  if (!storageOK || !f.open(STATE, FILE_O_READ))
    return;
  State saved;
  bool ok = f.read((uint8_t *)&saved, sizeof(saved)) == sizeof(saved);
  f.close();
  if (ok && saved.magic == MAGIC && saved.session >= 1 &&
      saved.session <= MAX_SESSIONS + 1 && saved.valid <= SESSION_SIZE &&
      saved.stage <= 2)
    state = saved;
}

void prepareCSV() {
  if (!storageOK || InternalFS.exists(CSV))
    return;
  File f(InternalFS);
  if (!f.open(CSV, FILE_O_WRITE))
    return;
  f.println("session,count,frequency_hz,mishit_flag,uptime_ms");
  f.close();
}

bool logHit(double frequency, bool mishit, uint32_t time) {
  if (!storageOK)
    return false;
  File f(InternalFS);
  if (!f.open(CSV, FILE_O_WRITE))
    return false;
  f.seek(f.size());
  f.print(state.session);
  f.print(',');
  f.print(state.count);
  f.print(',');
  f.print(frequency, 2);
  f.print(',');
  f.print(mishit);
  f.print(',');
  f.println(time);
  f.close();
  return true;
}

double readFrequency() {
  uint32_t next = micros();
  for (uint16_t i = 0; i < N; i++) {
    while ((int32_t)(micros() - next) < 0) {
    }
    realData[i] = analogRead(PIEZO);
    imagData[i] = 0;
    next += 1000000UL / SAMPLE_RATE;
  }

  double mean = 0;
  for (uint16_t i = 0; i < N; i++) {
    mean += realData[i];
  }
  mean /= N;

  for (uint16_t i = 0; i < N; i++) {
    realData[i] -= mean;
  }

  FFT.windowing(FFTWindow::Hann, FFTDirection::Forward);
  FFT.compute(FFTDirection::Forward);
  FFT.complexToMagnitude();

  uint16_t peakBin = 1;
  double peakMagnitude = realData[1];

  for (uint16_t bin = 2; bin <= N / 2; bin++) {
    if (realData[bin] > peakMagnitude) {
      peakMagnitude = realData[bin];
      peakBin = bin;
    }
  }

  if (!isfinite(peakMagnitude)) {
    return NAN;
  }

  return (double)peakBin * SAMPLE_RATE / N;
}

void startCooldown() {
  state.cooling = 1;
  state.stage = 0;
  cooldownStarted = millis();
  saveState();
}

void nextSession() {
  state.session++;
  state.valid = 0;
  state.count = 0;
  state.stage = 0;
  state.cooling = 0;
  saveState();
}

void processHit(double frequency, uint32_t time) {
  // The first impact has no previous impact, so just store it as valid
  if (state.stage == 0) {
    state.count++;

    if (!logHit(frequency, false, time)) {
      state.count--;
      return;
    }

    state.valid++;
    state.previous = frequency;
    state.stage = 1;

    if (state.valid >= SESSION_SIZE) {
      startCooldown();
    } else {
      saveState();
    }

    return;
  }

  // Store the 2nd impact until the 3rd impact is availabl
  if (state.stage == 1) {
    state.pending = frequency;
    state.pendingTime = time;
    state.stage = 2;
    saveState();
    return;
  }

  /* remember:
   * previous = f[n-1]
   * pending  = f[n]
   * frequency = f[n+1]
   */
  double deltaPrevious =
      fabs(state.pending -
           state.previous); // absolute freq diff from the previous impact
  double deltaNext = fabs(state.pending - frequency);

  bool mishit = deltaPrevious > 50.0 && fabs(deltaNext - deltaPrevious) > 20.0;

  // Record the pending impact, whether valid or a mishit
  state.count++;

  if (!logHit(state.pending, mishit, state.pendingTime)) {
    state.count--;
    state.stage = 0;
    state.previous = 0;
    state.pending = 0;
    state.pendingTime = 0;

    saveState();
    return;
  }

  if (!mishit) {
    state.valid++;
  }

  /*
   * Always advance previous, including when pending was a mishit.
   * The paper's f_[n-1] means the immediately preceding detected impact,
   * not the previous valid impact
   */
  state.previous = state.pending;

  /*
   * When 149 valid impacts have been recorded, the current impact should become
   * the final impact. Because it has no impact after it, the paper says that it
   * treats it as valid
   */
  if (state.valid == SESSION_SIZE - 1) {
    state.count++;

    if (!logHit(frequency, false, time)) {
      state.count--;

      state.stage = 0;
      state.previous = 0;
      state.pending = 0;
      state.pendingTime = 0;

      saveState();
      return;
    }

    state.valid++;
    state.previous = frequency;
    startCooldown();
    return;
  }

  // The current impact becomes the pending middle impact
  state.pending = frequency;
  state.pendingTime = time;
  state.stage = 2;
  saveState();
}

void setup() {
  analogReadResolution(10);
  storageOK = InternalFS.begin();
  loadState();
  prepareCSV();

  for (int i = 0; i < 256; i++) {
    baseline += analogRead(PIEZO);
    delayMicroseconds(200);
  }
  baseline /= 256;
  if (state.cooling)
    cooldownStarted = millis();
}

void loop() {
  if (state.session > MAX_SESSIONS)
    return;

  if (state.cooling) {
    if (millis() - cooldownStarted >= COOLDOWN)
      nextSession();
    delay(20);
    return;
  }

  int sample = analogRead(PIEZO);
  int difference = abs(sample - (int)round(baseline));

  if (!armed) {
    if (difference < TRIGGER / 2) {
      if (!quietSince)
        quietSince = millis();
      if (millis() - quietSince >= 80 && millis() - lastHit >= 500)
        armed = true;
    } else
      quietSince = 0;
    return;
  }

  if (difference < TRIGGER) {
    baseline = baseline * 0.999 + sample * 0.001;
    return;
  }

  armed = false;
  quietSince = 0;
  lastHit = millis();
  double frequency = readFrequency();
  if (isfinite(frequency))
    processHit(frequency, lastHit);
}
