# MPU-6050 → ESP32-C3 live dashboard

Reads a 6-axis IMU at 200 Hz, fuses it with a Madgwick AHRS **on the ESP32**,
runs motion-pattern detection **on the ESP32** at the full 200 Hz, and streams
the result as CSV lines at 33 Hz. The browser just displays.

---

## Wiring (4 wires)

| MPU-6050 (GY-521) | ESP32-C3 Super Mini |
|---|---|
| VCC | **3V3** |
| GND | GND |
| SDA | **GPIO5** |
| SCL | **GPIO4** |
| AD0 | leave open (→ address 0x68) |
| INT | not connected |

**SDA is GPIO5 and SCL is GPIO4** — deliberately crossed relative to the
"obvious" order. The firmware probes **both** orders at boot, so if you wire it
the other way it will still be found; the boot log tells you which order won.

> ⚠️ **Power it from 3V3, never 5 V.** Most GY-521 breakouts tie the I²C
> pull-ups to VCC. The ESP32-C3 is not 5 V tolerant, so 5 V on VCC puts 5 V on
> SDA/SCL and can kill the pins.

> The sensor is confirmed working as an **MPU-6500 die** (WHO_AM_I = 0x70) at
> address 0x68 — a genuine InvenSense part on a GY-521 board. The firmware
> selects the matching temperature formula automatically; the MPU-6050 formula
> reads ~11 °C too high on this die.

---

## Running it

**1. Flash the board** (only needed once, or after editing `src/main.cpp`):

Double-click **`flash.command`**, or from a terminal:

```bash
cd mpu6050_test
pio run -t upload
```

`flash.command` waits for the ESP32 to appear instead of guessing the port,
which matters here: this Mac also has a Bluetooth speaker that shows up as a
serial device, and `pio` will happily pick it and fail.

> Make sure no serial monitor is holding the port, or the upload fails with
> `Could not open /dev/cu.usbmodem101 ... Resource temporarily unavailable`.
> Close PlatformIO's monitor / the dashboard tab first.

**2. Open the dashboard** — double-click **`start_dashboard.command`**.

It serves this folder on `http://localhost:8080` and opens Chrome. Leave the
terminal window open while you use it.

**3. Connect**, using either button:

- **Connect USB** *(recommended)* — pick the ESP32 port from the list. No
  Bluetooth stack, no pairing, no macOS permission prompts.
- **Connect Bluetooth** — pick `MPU6050-Test`. Untethered, and works on
  Android Chrome.

Either way the same CSV stream arrives, so the display is identical.

**4. Hold the sensor still and flat for ~5 seconds** — that is the gyro/accel
calibration. The dashboard shows zeros until it finishes.

---

## Reading the output

Each CSV line has 27 fields:

```
qw,qx,qy,qz,ax,ay,az,gx,gy,gz,tempC,sensorOk,count,roll,pitch,yaw,
aMag,jerk,evFlags,impactG,swayHz,swayRmsG,cntImpact,cntDrop,cntSway,cntShake,cntTap
```

- `qw..qz` — orientation quaternion (computed on the ESP32)
- `ax,ay,az` — accelerometer in **g** (gravity included: flat and still reads `az ≈ 1.0`)
- `gx,gy,gz` — gyroscope in **°/s**
- `tempC` — die temperature in °C
- `sensorOk` — 1 when the IMU initialised
- `count` — packet counter
- `roll,pitch,yaw` — Euler angles in degrees
- `aMag` — |acceleration| in g
- `jerk` — d|a|/dt in g/s (the sharpness of an impact)
- `evFlags` — bitfield, see below
- `impactG` — peak |a| of the most recent impact
- `swayHz`, `swayRmsG` — dominant sway frequency and amplitude
- `cntImpact,cntDrop,cntSway,cntShake,cntTap` — running event counters

The fields after `yaw` were **appended**, so an older dashboard that reads only
the first 16 still works, and this dashboard shows `--` on the pattern panel if
it is talking to older firmware.

The `Connection` card also reports the I²C scan: how many devices answered, the
`WHO_AM_I` bytes, and which chip was recognised.

---

## Detected patterns

Detection runs on the ESP32 at **200 Hz**, not in the browser — the board streams
at 33 Hz, so a hit that lasts 20 ms would be badly smeared or missed entirely if
it were detected in JavaScript.

`evFlags` mixes two kinds of bit:

| Bit | Meaning |
|---|---|
| `0x0001` | free-fall started this sample |
| `0x0002` | **drop** — free-fall followed by a landing |
| `0x0004` | **impact / hit** |
| `0x0008` | **tap** |
| `0x0010` | sway started this sample |
| `0x0020` | shake started this sample |
| `0x0040` | came to rest this sample |
| `0x0080` | started moving this sample |
| `0x0100` | free-fall **right now** |
| `0x0200` | at rest **right now** |
| `0x0400` | swaying **right now** |
| `0x0800` | shaking **right now** |

The transient bits (`0x0001`–`0x0080`) are true for a single sample. The live
bits (`0x0100`–`0x0800`) describe the current state. The dashboard holds the
transient pills lit for a moment (drop 3 s, impact 1.5 s, tap 0.9 s) so you can
actually see them.

How each one is decided:

- **Free fall** — |a| drops below 0.45 g and stays there for ≥ 60 ms.
- **Drop** — the free fall ends with a spike above 2.5 g within 1.5 s. A free
  fall that never lands (a gentle toss) is classified as free fall only.
- **Impact / hit** — |a| above 2.2 g **and** jerk above 250 g/s, then a 180 ms
  refractory period so one collision is not counted several times.
- **Tap** — a short bump: rises above 1.5 g and falls back below 1.25 g within
  60 ms.
- **Sway** — the horizontal part of the acceleration, band-passed 0.25–2.5 Hz,
  sustained above 0.02 g RMS with several zero crossings.
- **Shake** — the same idea in the 2.5–15 Hz band, needing above 0.20 g RMS.
- **At rest** — |a| within 0.02 g of 1 g and every gyro axis under 1.5 °/s for
  400 ms.

The thresholds live in `md::Config` in `src/motion_detect.h`, each with a comment
explaining the physical reasoning, so they are all yours to tune.

### Testing the detectors without hardware

`src/motion_detect.h` has no Arduino dependencies, so the exact same code runs
on your Mac:

```bash
cd mpu6050_test
c++ -std=c++17 -O2 -o /tmp/test_motion test/test_motion.cpp && /tmp/test_motion
```

It plays ten synthetic scenarios (still, drop, hit, ringing impact, sway, shake,
slow tilt, a long toss, a tap, and sway on a tilted sensor) and checks what came
out. It should print `38 passed, 0 failed`.

---

## Why there is no position display

There is deliberately **no displacement or position readout**, and adding one
back would not work.

Position requires integrating acceleration **twice**. A consumer accelerometer
has a bias on the order of 10 mg, and that bias integrates to `b·t²/2` of
phantom distance: 10 mg is 0.098 m/s², which is **1.8 m of fake travel after
only 6 seconds**, and 29 m after a minute. It is not a tuning problem — it is
what the arithmetic does, and no filter removes it, because real slow motion
and sensor bias occupy the same frequency band.

What *is* solidly measurable from this sensor, and what the dashboard shows:

- **orientation** — the Madgwick quaternion, accurate in pitch and roll
  (yaw drifts without a magnetometer, which is normal);
- **tilt** and which way up the board is;
- **rotation rate** per axis;
- **acceleration** per axis and its magnitude;
- **jerk** — the derivative of |a|, which is what makes a hit feel sharp;
- **vibration** content and its frequency, via band-passed RMS and zero
  crossings;
- **events** — the detected-pattern panel below.

For real position you need an external reference: optical flow, UWB, GPS, or a
magnetometer-equipped 9-axis part for yaw. That is a hardware change, not a
software one.

### Watching the raw stream

The firmware mirrors the exact same CSV over USB serial, so you can debug
without the dashboard:

```bash
python3 - <<'EOF'
import serial, time
s = serial.Serial('/dev/cu.usbmodem101', 115200, timeout=0.2)
s.setDTR(False); s.setRTS(True); time.sleep(0.15); s.setRTS(False)
t0 = time.time()
while time.time() - t0 < 10:
    d = s.read(4096)
    if d: print(d.decode('utf-8', 'replace'), end='')
EOF
```

(`pio device monitor -b 115200` works too, but it needs a real terminal.)

---

## If something is wrong

| Symptom | Cause / fix |
|---|---|
| `I²C Scan: NO devices on bus!` | Wiring or power. The firmware already tried GPIO4/GPIO5 both ways, so a swap is not it. Check 3V3 and GND. |
| Connected, but the display stays at zero | Sensor not answering — see the I²C note in the Connection card. |
| Upload fails, port busy | Something is holding the port (serial monitor or the dashboard's USB connection). Close it. |
| "Bluetooth not supported here" | Safari and Firefox have no Web Bluetooth. Use Chrome/Edge/Opera. |
| macOS never shows a Bluetooth permission prompt | System Settings → Privacy & Security → Bluetooth → enable **Google Chrome**. |
| Pattern panel shows `--` everywhere | The firmware is older than the dashboard. Re-flash: `pio run -t upload`. |
| `pio run -t upload` says `No serial data received` | It picked the wrong port (e.g. a Bluetooth speaker). Pass the port explicitly: `pio run -t upload --upload-port /dev/cu.usbmodem101`. |

---

## Files

| File | Purpose |
|---|---|
| `src/main.cpp` | Firmware: I²C probe, calibration, Madgwick AHRS, CSV streaming |
| `dashboard.html` | Browser UI — Web Bluetooth + Web Serial, Three.js 3D orientation, gyro chart, pattern panel |
| `vendor/three.min.js` | Three.js vendored locally, so the page needs no internet |
| `start_dashboard.command` | Double-click launcher (local server + Chrome) |
| `flash.command` | Double-click build + upload, waits for the right port |
| `src/motion_detect.h` | Pattern detection engine — portable C++, no Arduino deps |
| `test/test_motion.cpp` | Host test harness for the detector (10 scenarios, 38 checks) |
| `platformio.ini` | Board config and USB-CDC build flags |
