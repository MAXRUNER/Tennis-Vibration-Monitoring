# Tennis Vibration Monitoring
This repository contains the firmware used for the experiments described in the accompanying paper Longitudinal Analysis of Tennis String-bed Degradation Through Embedded Vibration Monitoring.

C++ Codebase for the Seeed Studio XIAO nRF52840 Sense implementing an onboard FFT-based vibration analysis and impact classification for a longitudinal study of tennis string-bed degradation as decribed in the accompanying paper

## Longitudinal Analysis of Tennis String-bed Degradation Through Embedded Vibration Monitoring
Using a Seeed Studio XIAO nRF52840 Sense and a TE Connectivity LDT0-028K Piezoelectric sensor as the primary components, the system samples the piezoelectric sensor signal, performs an onboard Fast Fourier Transform (FFT), extracts the dominant post-impact frequency, and logs the result for later analysis. 

A Hann Window is applied to reduce the spectral leakage caused by discontinuities at the boundaries of the sampled signal. During field testing, the system operates without requiring the Serial Monitor; serial output was used only during debugging and pre-field testing.

The system also uses a mishit identification algorithm. Originally, there were no shank classification methods, but after conducting a few tests by taking an old set of strings and conducting the trials on the racket, the algorithm was set. However, there is still the risk of wrongly identifying mishits because the algorithm could potentially make errors in the classification, 


### Features
- Piezoelectric impact detection
- 10 kHz vibration sampling
- 1024-point FFT
- Hann windowing to reduce spectral leakage
- Dominant frequency extraction using an FFT
- Mishit identification and classification
- Flash-based CSV logging
- Automatic session tracking
- OLED status display (Can be disabled or removed by removing `#define USE_OLED`)
- Persistent state recovery after power loss

### Hardware
- Seeed Studio XIAO nRF52840 Sense
- TE Connectivity LDT0-028K Piezoelectric sensor
- 1 MΩ Resistor
- 10 kΩ Resistor
- 3.3V Zener Diode
- 3.7V LiPo Battery
- SSD1306 OLED (optional, used during code development and pre-field testing)
- XIAO Expansion Board (Provided a 128x64 OLED, a JST-PH battery port, a buzzer and a button)

### Software Libraries
- Adafruit_TinyUSB.h
- Adafruit_LittleFS.h  
- InternalFileSystem.h  
- arduinoFFT.h  

> #include <Adafruit_TinyUSB.h>  
> #include <Adafruit_LittleFS.h>  
> #include <InternalFileSystem.h>  
> #include <arduinoFFT.h>  

### Processing Pipeline
Ball impact -> Piezoelectric Sensor -> ADC Sampling -> 1024 Samples at 10 kHz -> Hann Window -> FFT -> Major Peak Detection -> Mishit Classification -> CSV Logging

### CSV Schema Documentation

<details>
<summary><b>Click to view CSV Column Definition</b></summary>

| Column Name | Data Type | Description |
| :--- | :--- | :--- |
| `session` | Integer | Session number (1 to 7). Increases after 150 valid shots are hit. |
| `count` | Integer | Ball impact count for the session. Features a 10-hour cooldown after 150 valid shots. |
| `frequency_hz` | Float | Dominant post-impact frequency in Hertz. |
| `mishit_flag` | Binary | `0` = Valid impact, `1` = Mishit. |
| `uptime_ms` | Integer | Time elapsed in milliseconds since the microcontroller booted. |

#### Raw Format Example
```csv
session,count,frequency_hz,mishit_flag,uptime_ms
1,1,597.2,0,2014
```
</details>

### Session Workflow
Power On -> Session 1 -> 150 valid impacts -> 10 hour cooldown -> Session 2 -> ... -> Session 7 -> Experiment Complete

### Mishit Detection

$Δf_{\text{prev}} = | f_n - f_{n-1} |$  
$Δf_{\text{next}} = | f_n - f_{n+1} |$  
An impact was classified as a potential mishit if it satisfied both of the following conditions:  
$Δf_{\text{prev}} > 50 \text{ Hz}$  
$|Δf_{\text{next}} - Δf_{\text{prev}}| > 20 \text{ Hz}$

### Build
1. Install the Seeed XIAO nRF52840 board package
2. Install the required Arduino Libraries
3. Download this repo and open it
4. Compile and Upload the code to the module

Note: Though I designed the codebase myself, some parts of this repository are edited by AI. AI was mainly used to help resolve compilation issues, configure the library environment, and draft much of the optional OLED display text. This allowed me to save time by using it for the remaining parts of the code, rather than on a feature that was meant to be optional in the first place. However, the signal acquisition, FFT pipeline, the mishit logic and others were all designed without AI.


Questions, bug reports, and suggestions are welcome.  
Email: **rithwikmahanti258@gmail.com**
