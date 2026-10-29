# Tennis Vibration Monitoring
This repository contains the firmware used for the experiments described in the accompanying paper Longitudinal Analysis of Tennis String-bed Degradation Through Embedded Vibration Monitoring.

Arduino C++ Firmware for the Seeed Studio XIAO nRF52840 implementing an onboard FFT-based vibration analysis and impact classification for a longitudinal study of tennis string-bed degradation as described in the accompanying paper

## Longitudinal Analysis of Tennis String-bed Degradation Through Embedded Vibration Monitoring
Using a Seeed Studio XIAO nRF52840 and a TE Connectivity LDT0-028K piezoelectric sensor as the primary components, the system samples the piezoelectric sensor signal, performs an onboard Fast Fourier Transform (FFT), extracts the dominant post-impact frequency, and logs the result for later analysis. 

A Hann window is applied to reduce the spectral leakage caused by discontinuities at the boundaries of the sampled signal. During field testing, the system operates without requiring the Serial Monitor; serial output was used only during debugging and pre-field testing.

The system also uses a mishit identification algorithm. The classification thresholds were determined empirically through preliminary testing using an older string bed before the main longitudinal experiment. However, there is still the risk of wrongly identifying mishits because the algorithm could make errors in the classification.


### Features
- Piezoelectric impact detection
- 10 kHz vibration sampling
- 2048-point FFT
- Hann windowing to reduce spectral leakage
- Dominant frequency extraction using an FFT
- Mishit identification and classification
- Flash-based CSV logging
- Automatic session tracking
- Persistent state recovery after power loss
  
<img width="627" height="807" alt="image" src="https://github.com/user-attachments/assets/99c31995-eb99-4f14-b0e7-e012c2c26113" />
<img width="480" height="388" alt="image" src="https://github.com/user-attachments/assets/71ee0e71-e531-4ac4-b70a-b9013bb93a94" />

### Hardware
- Seeed Studio XIAO nRF52840
- TE Connectivity LDT0-028K Piezoelectric Sensor
- 1× 10 kΩ resistor
- 2× 1 MΩ resistors
- 2× small-signal Schottky diodes for ADC input protection
- 1× Schottky diode for LiPo power isolation
- 3.7 V LiPo battery
- JST-PH 2-pin battery connector (2.0 mm pitch)
- 2× 1×7 2.54 mm female pin sockets for the removable XIAO nRF52840
- 2-pin connector for the piezoelectric sensor
- Custom PCB

<img width="818" height="729" alt="image" src="https://github.com/user-attachments/assets/31f9d240-71a5-4734-9768-91ece73de123" />

| Designator | Component | JLCPCB part # |
|---|---|---:|
| D2 | B140-13-F, SMA Schottky | `C15759` |
| D3, D4 | CDBF54-HF, SOD-323F Schottky | `C5612509` |
| J1, J2 | PM254V-11-07-H85, 1×7 female socket | `C2832270` |
| J4 | TSW-102-07-G-S, 1×2 male header | `C7402729` |
| J6 | B2B-PH-K-S(LF)(SN), JST-PH | `C131337` |
| R1 | 10 kΩ 0805, 0805W8F1002T5E | `C17414` |
| R3, R4 | 1 MΩ 0805, 0805W8F1004T5E | `C17514` |

### Ordered PCB Details
- 2-layer FR-4
- 0.8 mm thickness
- 1 oz copper
- Lead-free HASL
- Partial top-side assembly
- JLCPCB assembles D2, D3, D4, R1, R3 and R4
- J1, J2, J4 and J6 installed manually

### Software Libraries
> #include <Adafruit_TinyUSB.h>  
> #include <Adafruit_LittleFS.h>  
> #include <InternalFileSystem.h>  
> #include <arduinoFFT.h>
> #include <math.h>

### Processing Pipeline
Ball impact -> Piezoelectric Sensor -> ADC Sampling -> 2048 Samples at 10 kHz -> Hann window -> FFT -> Dominant Frequency Extraction -> Mishit Classification -> CSV Logging

### CSV Schema Documentation

<details>
<summary><b>Click to view CSV Column Definition</b></summary>

| Column Name | Data Type | Description |
| :--- | :--- | :--- |
| `session` | Integer | Session number (1 to 7). Advances after 150 valid impacts have been recorded for the current session. |
| `count` | Integer | Ball impact count for the session. Includes all detected impacts, including those classified as potential mishits. |
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
Power On -> Session 1 -> 150 valid impacts -> Cooldown Period -> Session 2 -> ... -> Session 7 -> Experiment Complete

### Mishit Detection
A retrospective high-frequency outlier filter was applied separately to each session. 

#### 1. Baseline Definition
The baseline $m_s$ is defined as the mode of the first 150 detected impacts:

$$m_s = \text{mode}(f_{s,1}, \dots, f_{s,150})$$

#### 2. Outlier Flagging
An impact is flagged if its dominant frequency exceeds this baseline by more than 90 Hz:

$$
M_{s,n} = 
\begin{cases} 
1, & f_{s,n} > m_s + 90\,\text{Hz} \\ 
0, & \text{otherwise} 
\end{cases}
$$

(Note: The 90 Hz threshold was selected after inspecting a dataset generated using an old set of Solinco Hyper-G strings at 54 &times; 54 lb tension on a Babolat Pure Aero 2019)


### Build
1. Install the Seeed XIAO nRF52840 board package
2. Install the required Arduino Libraries
3. Download this repo and open it (tennis_vibrational_monitoring.ino)
4. Compile and upload the firmware to the module

Note: Though I designed and wrote the firmware myself, some parts were edited with AI. The first use case was variable naming. I originally named variables things like i, x, y, z, a, b, and other random names that made sense at the time but eventually became impossible to keep track of. There were points where I couldn't even remember what some variables were for, so I used AI to generate a mapping and rename them into something a little more readable without changing the logic. The second use case was handling less common failure scenarios. Apart from obvious cases, such as the CSV file failing to open, I used AI to brainstorm additional edge cases and then implemented the appropriate error handling in the code. The biggest use case, however, was that it helped me understand that some of the more complex parts of the project could be implemented much more cleanly using lightweight C++ features.

And yes... I also used AI to name my git commits.

Questions, bug reports, and suggestions are welcome.  
Email: **rithwikmahanti258@gmail.com**
