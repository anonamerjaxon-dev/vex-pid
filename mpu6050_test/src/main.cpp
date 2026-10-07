#include <Arduino.h>
#include <Wire.h>
#include <NimBLEDevice.h>
#include "motion_detect.h"

// ── Pin Definitions ────────────────────────────────────────────
#define LED_PIN       8

// MPU6050 I²C pins.  Instead of hard-coding one wiring, the firmware
// PROBES the candidate orderings at boot and locks onto whichever one
// actually has an IMU on it.  That way SDA/SCL swapped in hardware can
// never silently break the build.
//
//   {4, 5} = SDA on GPIO4, SCL on GPIO5   (the "standard" wiring)
//   {5, 4} = SDA on GPIO5, SCL on GPIO4   (the "swapped" wiring)
//
// Both are tried on every boot; the winner is reported over serial and BLE.
static const int I2C_CANDIDATES[][2] = {
    {4, 5},
    {5, 4},
};
static const int I2C_CANDIDATE_COUNT = (int)(sizeof(I2C_CANDIDATES) / sizeof(I2C_CANDIDATES[0]));

static int i2cSdaPin = -1;   // set by MPU6050::begin() once a pin pair works
static int i2cSclPin = -1;

// ── BLE UUIDs (Nordic UART Service) ────────────────────────────
#define SERVICE_UUID  "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHAR_UUID_TX  "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// ── Timing ─────────────────────────────────────────────────────
#define AHRS_HZ       200
#define AHRS_US       (1000000 / AHRS_HZ)
#define BLE_HZ        33
#define BLE_US        (1000000 / BLE_HZ)

// ── BLE framing ────────────────────────────────────────────────
// A BLE notification can only carry (ATT MTU - 3) bytes.  The default
// ATT MTU is 23, i.e. a 20-byte payload — far too small for one CSV
// line.  We therefore
//   1. ask for a big MTU on connect, and
//   2. regardless of what we get, split every line into <= payload
//      sized notifications and terminate each line with '\n' so the
//      browser can reassemble.
// Without (2) the CSV can never arrive intact and the dashboard shows
// nothing at all.
#define BLE_ATT_HEADER_LEN 3
#define BLE_CHUNK_FLOOR    20        // payload when MTU stays at the 23 default
static volatile uint16_t bleChunkSize = BLE_CHUNK_FLOOR;
static char bleLineBuf[288];   // 16 base fields + 11 event fields

// ═══════════════════════════════════════════════════════════════
//  MPU6050 Register-Level Driver
// ═══════════════════════════════════════════════════════════════

// Register map
#define REG_WHO_AM_I  0x75
#define REG_PWR_MGMT1 0x6B
#define REG_SIGNAL_PATH_RESET 0x68
#define REG_CONFIG    0x1A
#define REG_GYRO_CFG  0x1B
#define REG_ACCEL_CFG 0x1C
#define REG_ACCEL_XH  0x3B
#define REG_TEMP_H    0x41

// Sensitivity (LSB per unit) — set by range config.
// These are IDENTICAL for the MPU-6050, MPU-6000, MPU-6500 and MPU-9250,
// so a clone die needs no change here.
static const float ACCEL_LSB = 2048.0f;    // ±16g
static const float GYRO_LSB  = 16.4f;      // ±2000°/s

// Temperature conversion differs between dies:
//   MPU-6050/6000: T = TEMP_OUT/340 + 36.53
//   MPU-6500/9250: T = TEMP_OUT/321 + 25.0
// Cheap "GY-521" boards very often carry a 6500 die, so getting this
// wrong reports a temperature that is ~11 °C too high. The correct pair
// is selected in setup() once WHO_AM_I has been read.
static float TEMP_OFFSET = 36.53f;
static float TEMP_SCALE  = 340.0f;

class MPU6050 {
public:
    MPU6050() : m_addr(0) {}

    // Scan diagnostics (populated by begin(), sent via BLE)
    uint8_t diagCount = 0;
    uint8_t diagAddr[8] = {0};
    uint8_t diagWho[8] = {0};     // raw WHO_AM_I byte
    uint8_t diagMatch[8] = {0};   // 0=none, 1=MPU-6050, 2=MPU-6500, 3=MPU-9250, 4=ICM-20602, 5=ICM-20948, 6=unknown-at-0x68

    uint8_t address() const { return m_addr; }
    uint8_t whoAmI()  const { return m_who; }   // WHO_AM_I of the chip actually in use

    const char* chipName() const {
        switch (m_who) {
            case 0x68: return "MPU-6050/6000";
            case 0x70: return "MPU-6500";
            case 0x71: return "MPU-9250";
            case 0x73: return "MPU-9255";
            case 0x12: return "ICM-20602";
            case 0xEA: return "ICM-20948";
            default:   return "unidentified clone";
        }
    }

    // Probe every candidate SDA/SCL ordering and use the first one that
    // yields a recognised IMU.
    bool begin() {
        for (int i = 0; i < I2C_CANDIDATE_COUNT; i++) {
            int sda = I2C_CANDIDATES[i][0];
            int scl = I2C_CANDIDATES[i][1];

            Serial.printf("[i2c] probing SDA=GPIO%d  SCL=GPIO%d ... ", sda, scl);
            if (beginOnPins(sda, scl)) {
                i2cSdaPin = sda;
                i2cSclPin = scl;
                Serial.printf("FOUND IMU at 0x%02X\n", m_addr);
                return true;
            }
            Serial.printf("nothing (%d device(s) on bus)\n", diagCount);
            Wire.end();           // release the pins before trying the next pair
            delay(20);
            diagCount = 0;
            m_addr = 0;
        }
        return false;
    }

    bool beginOnPins(int sda, int scl) {
        Wire.begin(sda, scl);
        delay(100);  // MPU datasheet: ~100ms from VDD stable to ready

        // ── Full I²C scan at 100 kHz ────────────────────
        // 100 kHz (not 400) because dupont jumpers + a 4 MB flash
        // breakout are not a controlled-impedance bus.
        Wire.setClock(100000);
        diagCount = 0;
        m_addr = 0;

        for (uint8_t addr = 1; addr < 127 && diagCount < 8; addr++) {
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() != 0) continue;  // no ACK

            // Device ACK'd — store address, read WHO_AM_I
            diagAddr[diagCount] = addr;

            Wire.beginTransmission(addr);
            Wire.write(REG_WHO_AM_I);
            Wire.endTransmission(false);
            Wire.requestFrom((uint16_t)addr, (uint8_t)1);
            uint8_t who = Wire.available() ? Wire.read() : 0x00;
            diagWho[diagCount] = who;

            // Identify chip by WHO_AM_I value
            // See: InvenSense/TDK register map, §3.1 "WHO_AM_I"
            switch (who) {
                case 0x68: diagMatch[diagCount] = 1; break;  // MPU-6050, MPU-6000
                case 0x70: diagMatch[diagCount] = 2; break;  // MPU-6500
                case 0x71: diagMatch[diagCount] = 3; break;  // MPU-9250
                case 0x73: diagMatch[diagCount] = 3; break;  // MPU-9255 (same register map as 9250)
                case 0x12: diagMatch[diagCount] = 4; break;  // ICM-20602
                case 0xEA: diagMatch[diagCount] = 5; break;  // ICM-20948
                default:   diagMatch[diagCount] = 0; break;  // unknown / not an IMU
            }

            if (diagMatch[diagCount] != 0 && m_addr == 0) {
                m_addr = addr;  // grab first recognized IMU
                m_who  = who;
            }

            // Clone tolerance: real GY-521 boards always live at 0x68/0x69,
            // but a few clone dies report a WHO_AM_I we do not know.  If the
            // address is right, assume it is usable and record the raw byte
            // so the serial log / dashboard can flag it as unidentified.
            if (diagMatch[diagCount] == 0 && (addr == 0x68 || addr == 0x69) && m_addr == 0) {
                diagMatch[diagCount] = 6;   // 6 = "at the right address, unknown WHO_AM_I"
                m_addr = addr;
                m_who  = who;
            }

            diagCount++;
        }

        // ── Configure detected chip ──────────────────────
        if (m_addr != 0) {
            return configure();
        }

        // ── Chip not in scan? Try wake on 0x68/0x69 ─────
        const uint8_t fallback[] = {0x68, 0x69};
        for (uint8_t i = 0; i < 2; i++) {
            Wire.beginTransmission(fallback[i]);
            Wire.write(REG_PWR_MGMT1);
            Wire.write(0x00);       // clear SLEEP bit
            Wire.endTransmission();
            delay(5);

            Wire.beginTransmission(fallback[i]);
            Wire.write(REG_WHO_AM_I);
            if (Wire.endTransmission(false) != 0) continue;
            Wire.requestFrom((uint16_t)fallback[i], (uint8_t)1);
            if (Wire.available()) {
                uint8_t who = Wire.read();
                if (who == 0x68 || who == 0x70 || who == 0x71 || who == 0x73 || who == 0x12 || who == 0xEA) {
                    m_addr = fallback[i];
                    m_who  = who;
                    return configure();
                }
            }
        }

        return false;
    }

    bool readRaw(int16_t& ax, int16_t& ay, int16_t& az,
                 int16_t& gx, int16_t& gy, int16_t& gz,
                 int16_t& temp) {
        if (m_addr == 0) return false;
        Wire.beginTransmission(m_addr);
        Wire.write(REG_ACCEL_XH);
        if (Wire.endTransmission(false) != 0) return false;

        Wire.requestFrom((uint16_t)m_addr, (uint8_t)14);
        if (Wire.available() < 14) return false;

        uint8_t buf[14];
        for (int i = 0; i < 14; i++) buf[i] = Wire.read();

        ax   = (int16_t)((buf[0]  << 8) | buf[1]);
        ay   = (int16_t)((buf[2]  << 8) | buf[3]);
        az   = (int16_t)((buf[4]  << 8) | buf[5]);
        temp = (int16_t)((buf[6]  << 8) | buf[7]);
        gx   = (int16_t)((buf[8]  << 8) | buf[9]);
        gy   = (int16_t)((buf[10] << 8) | buf[11]);
        gz   = (int16_t)((buf[12] << 8) | buf[13]);

        return true;
    }

private:
    uint8_t m_addr;
    uint8_t m_who = 0;

    bool configure() {
        writeRegister(REG_PWR_MGMT1, 0x80);  // device reset
        delay(100);
        writeRegister(REG_PWR_MGMT1, 0x01);  // auto-select clock, wake
        delay(10);
        writeRegister(REG_SIGNAL_PATH_RESET, 0x07);
        delay(10);

        writeRegister(REG_CONFIG,     0x01);  // DLPF: 184Hz accel, 188Hz gyro
        writeRegister(REG_GYRO_CFG,   0x18);  // ±2000°/s
        writeRegister(REG_ACCEL_CFG,  0x18);  // ±16g

        return true;
    }

    uint8_t readRegister(uint8_t reg) {
        Wire.beginTransmission(m_addr);
        Wire.write(reg);
        if (Wire.endTransmission(false) != 0) return 0;
        Wire.requestFrom((uint16_t)m_addr, (uint8_t)1);
        return Wire.available() ? Wire.read() : 0;
    }

    void writeRegister(uint8_t reg, uint8_t val) {
        Wire.beginTransmission(m_addr);
        Wire.write(reg);
        Wire.write(val);
        Wire.endTransmission();
    }
};

// ═══════════════════════════════════════════════════════════════
//  Madgwick AHRS — NED frame (North-East-Down)
//
//  Reference: Madgwick, S.O.H. (2010) "An efficient orientation
//             filter for inertial and IMU arrays"
//
//  The filter fuses 3-axis accelerometer + 3-axis gyroscope into
//  a quaternion representing sensor orientation relative to Earth.
//  Magnetometer is omitted (gyro yaw will drift slowly).
// ═══════════════════════════════════════════════════════════════

class Madgwick {
public:
    Madgwick() : m_beta(0.05f) {}

    void setBeta(float beta) { m_beta = beta; }

    void reset() {
        m_q0 = 1.0f; m_q1 = 0.0f; m_q2 = 0.0f; m_q3 = 0.0f;
    }

    void update(float gx_dps, float gy_dps, float gz_dps,
                float ax_g,   float ay_g,   float az_g,
                float dt) {
        if (dt <= 0.0f || dt > 0.5f) return;

        float recipNorm;
        float s0, s1, s2, s3;
        float qDot1, qDot2, qDot3, qDot4;
        float _2q0, _2q1, _2q2, _2q3;
        float _4q0, _4q1, _4q2, _8q1, _8q2;
        float q0q0, q1q1, q2q2, q3q3;

        float gx = gx_dps * 0.0174533f;  // to rad/s
        float gy = gy_dps * 0.0174533f;
        float gz = gz_dps * 0.0174533f;

        // Normalize accelerometer
        recipNorm = 1.0f / sqrtf(ax_g * ax_g + ay_g * ay_g + az_g * az_g);
        float ax = ax_g * recipNorm;
        float ay = ay_g * recipNorm;
        float az = az_g * recipNorm;

        // Auxiliary variables
        _2q0 = 2.0f * m_q0; _2q1 = 2.0f * m_q1;
        _2q2 = 2.0f * m_q2; _2q3 = 2.0f * m_q3;
        _4q0 = 4.0f * m_q0; _4q1 = 4.0f * m_q1;
        _4q2 = 4.0f * m_q2; _8q1 = 8.0f * m_q1; _8q2 = 8.0f * m_q2;
        q0q0 = m_q0 * m_q0;
        q1q1 = m_q1 * m_q1;
        q2q2 = m_q2 * m_q2;
        q3q3 = m_q3 * m_q3;

        // Gradient descent — accelerometer
        s0 = _4q0 * q2q2 + _2q2 * ax + _4q0 * q1q1 - _2q1 * ay;
        s1 = _4q1 * q3q3 - _2q3 * ax + 4.0f * q0q0 * m_q1 - _2q0 * ay
           - _4q1 + _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * az;
        s2 = 4.0f * q0q0 * m_q2 + _2q0 * ax + _4q2 * q3q3 - _2q3 * ay
           - _4q2 + _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * az;
        s3 = 4.0f * q1q1 * m_q3 - _2q1 * ax + 4.0f * q2q2 * m_q3 - _2q2 * ay;

        recipNorm = 1.0f / sqrtf(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3);
        s0 *= recipNorm; s1 *= recipNorm;
        s2 *= recipNorm; s3 *= recipNorm;

        // Gyroscope integration
        qDot1 = 0.5f * (-m_q1 * gx - m_q2 * gy - m_q3 * gz);
        qDot2 = 0.5f * ( m_q0 * gx + m_q2 * gz - m_q3 * gy);
        qDot3 = 0.5f * ( m_q0 * gy - m_q1 * gz + m_q3 * gx);
        qDot4 = 0.5f * ( m_q0 * gz + m_q1 * gy - m_q2 * gx);

        // Fuse: gyro integration - beta * gradient
        qDot1 -= m_beta * s0;
        qDot2 -= m_beta * s1;
        qDot3 -= m_beta * s2;
        qDot4 -= m_beta * s3;

        // Integrate
        m_q0 += qDot1 * dt;
        m_q1 += qDot2 * dt;
        m_q2 += qDot3 * dt;
        m_q3 += qDot4 * dt;

        // Normalize quaternion
        recipNorm = 1.0f / sqrtf(m_q0 * m_q0 + m_q1 * m_q1
                               + m_q2 * m_q2 + m_q3 * m_q3);
        m_q0 *= recipNorm;
        m_q1 *= recipNorm;
        m_q2 *= recipNorm;
        m_q3 *= recipNorm;
    }

    void getQuaternion(float& q0, float& q1, float& q2, float& q3) const {
        q0 = m_q0; q1 = m_q1; q2 = m_q2; q3 = m_q3;
    }

private:
    float m_beta;
    float m_q0 = 1.0f, m_q1 = 0.0f, m_q2 = 0.0f, m_q3 = 0.0f;
};

// ═══════════════════════════════════════════════════════════════
//  Globals
// ═══════════════════════════════════════════════════════════════

MPU6050              mpu;
Madgwick             ahrs;

NimBLEServer*        bleServer = nullptr;
NimBLECharacteristic* bleTxChar = nullptr;
bool                 bleConnected = false;

// The I²C diagnostics are a snapshot, not a stream, so they must be
// re-sent whenever a new listener could be watching: at boot, on every
// BLE connect, and every DIAG_PERIOD_MS (which covers a browser or
// serial terminal that attaches after the board has already booted).
#define DIAG_PERIOD_MS 6000
static volatile bool diagPending = true;

// Gyro bias & accel offset (populated during calibration)
float gyroBiasX = 0.0f, gyroBiasY = 0.0f, gyroBiasZ = 0.0f;
float accelOffsetX = 0.0f, accelOffsetY = 0.0f, accelOffsetZ = 0.0f;

// Running gyro bias for temperature drift (slowly updated when stationary)
float gyroDriftX = 0.0f, gyroDriftY = 0.0f, gyroDriftZ = 0.0f;

// Latest sensor readings (updated at AHRS_HZ, read by BLE sender)
volatile float qw = 1.0f, qx = 0.0f, qy = 0.0f, qz = 0.0f;
volatile float accelX = 0.0f, accelY = 0.0f, accelZ = 0.0f;
volatile float gyroX = 0.0f, gyroY = 0.0f, gyroZ = 0.0f;
volatile float eulerRoll = 0.0f, eulerPitch = 0.0f, eulerYaw = 0.0f;
volatile float temperature = 0.0f;
volatile bool  sensorOk = false;
volatile unsigned int packetCount = 0;

// ── Motion pattern detection (runs at the full 200 Hz) ───────────────
// The stream only leaves the chip at 33 Hz, so a 5-25 ms impact transient
// is usually missed entirely by the browser.  Detection therefore happens
// here, on every raw sample, and only the RESULT is streamed.
static md::Detector motionDetector;
static md::Output   motionOut;

// ── BLE Server Callbacks ───────────────────────────────────────

class ServerCB : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* s, NimBLEConnInfo& info) override {
        bleConnected = true;
        // Re-send the I²C diagnostics now: they also go out at boot and
        // periodically, but a central that attaches late would otherwise
        // never see them.
        diagPending = true;
        // Size our notifications to whatever the peer actually granted.
        uint16_t mtu = info.getMTU();
        uint16_t payload = (mtu > BLE_ATT_HEADER_LEN) ? (mtu - BLE_ATT_HEADER_LEN) : BLE_CHUNK_FLOOR;
        if (payload > (uint16_t)(sizeof(bleLineBuf) - 2))
            payload = (uint16_t)(sizeof(bleLineBuf) - 2);
        bleChunkSize = payload;
        Serial.printf("[ble] connected: MTU=%u, notification payload=%u bytes\n",
                      mtu, payload);
    }
    void onDisconnect(NimBLEServer* s, NimBLEConnInfo& info, int r) override {
        bleConnected = false;
        Serial.println("[ble] disconnected — advertising again");
        bleServer->startAdvertising();
    }
};

// ── Calibration ────────────────────────────────────────────────

void calibrateSensors() {
    digitalWrite(LED_PIN, HIGH);

    int calSamples = 2000;

    // ── Stage 1: Gyro bias ─────────────────────────────────
    float sumGx = 0, sumGy = 0, sumGz = 0;
    int validG = 0;

    for (int i = 0; i < calSamples; i++) {
        int16_t rawAx, rawAy, rawAz, rawGx, rawGy, rawGz, rawTemp;
        if (!mpu.readRaw(rawAx, rawAy, rawAz, rawGx, rawGy, rawGz, rawTemp))
            continue;

        sumGx += rawGx;
        sumGy += rawGy;
        sumGz += rawGz;
        validG++;

        delayMicroseconds(800);
    }

    if (validG > 0) {
        float avgGx = sumGx / validG;
        float avgGy = sumGy / validG;
        float avgGz = sumGz / validG;

        // Reject outliers (> 3 LSB from mean — sensor must be still)
        sumGx = sumGy = sumGz = 0;
        int valid2 = 0;

        for (int i = 0; i < calSamples; i++) {
            int16_t rawAx, rawAy, rawAz, rawGx, rawGy, rawGz, rawTemp;
            if (!mpu.readRaw(rawAx, rawAy, rawAz, rawGx, rawGy, rawGz, rawTemp))
                continue;

            float dx = rawGx - avgGx, dy = rawGy - avgGy, dz = rawGz - avgGz;
            if (abs(dx) < 6.0f && abs(dy) < 6.0f && abs(dz) < 6.0f) {
                sumGx += rawGx;
                sumGy += rawGy;
                sumGz += rawGz;
                valid2++;
            }

            delayMicroseconds(800);
        }

        if (valid2 > 100) {
            gyroBiasX = sumGx / valid2 / GYRO_LSB;
            gyroBiasY = sumGy / valid2 / GYRO_LSB;
            gyroBiasZ = sumGz / valid2 / GYRO_LSB;
        } else {
            gyroBiasX = avgGx / GYRO_LSB;
            gyroBiasY = avgGy / GYRO_LSB;
            gyroBiasZ = avgGz / GYRO_LSB;
        }
    }

    // ── Stage 2: Accelerometer offset (flat calibration) ───
    //  Sensor must be FLAT (Z pointing up).
    //  Ideal: [0, 0, +1g].  Offset = measured - ideal.
    float sumAx = 0, sumAy = 0, sumAz = 0;
    int validA = 0;

    for (int i = 0; i < 800; i++) {
        int16_t rawAx, rawAy, rawAz, rawGx, rawGy, rawGz, rawTemp;
        if (!mpu.readRaw(rawAx, rawAy, rawAz, rawGx, rawGy, rawGz, rawTemp))
            continue;

        sumAx += rawAx;
        sumAy += rawAy;
        sumAz += rawAz;
        validA++;

        delayMicroseconds(1200);
    }

    if (validA > 0) {
        float avgAx = sumAx / validA;
        float avgAy = sumAy / validA;
        float avgAz = sumAz / validA;

        accelOffsetX = avgAx / ACCEL_LSB;         // ideal = 0
        accelOffsetY = avgAy / ACCEL_LSB;         // ideal = 0
        accelOffsetZ = avgAz / ACCEL_LSB - 1.0f;  // ideal = +1g, so offset = measured - 1
    }

    digitalWrite(LED_PIN, LOW);
}

// ═══════════════════════════════════════════════════════════════
//  Setup
// ═══════════════════════════════════════════════════════════════

void setup() {
    // ── USB serial diagnostics ─────────────────────────────────
    // The ESP32-C3 uses its native USB-Serial/JTAG, so this shows up on
    // /dev/cu.usbmodem* at 115200.  Everything important is mirrored here
    // so wiring can be debugged WITHOUT the browser.
    Serial.begin(115200);
    // USB CDC: never block if no host has opened the port yet, otherwise
    // setup() would stall here whenever the board is powered from a
    // charger or the monitor is not attached.
    Serial.setTxTimeoutMs(0);
    delay(400);              // let the USB CDC endpoint enumerate
    Serial.println();
    Serial.println("======================================================");
    Serial.println(" MPU6050 / ESP32-C3 test firmware");
    Serial.println("======================================================");

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    // ── Start BLE immediately (no dependencies) ────────────────
    {
        md::Config mc;
        mc.sampleHz = (float)AHRS_HZ;
        motionDetector.begin(mc);
        Serial.printf("[det] motion detection @ %d Hz: drop/impact/sway/shake/tap/still\n",
                      AHRS_HZ);
    }

    NimBLEDevice::init("MPU6050-Test");
    NimBLEDevice::setPower(ESP_PWR_LVL_P3);

    // Ask for a large ATT MTU so one CSV line fits in a single
    // notification.  Chrome typically grants ~185-517; if it refuses we
    // still work because every line is chunked (see emitLine).
    NimBLEDevice::setMTU(247);

    bleServer = NimBLEDevice::createServer();
    bleServer->setCallbacks(new ServerCB());

    NimBLEService* svc = bleServer->createService(SERVICE_UUID);
    bleTxChar = svc->createCharacteristic(CHAR_UUID_TX,
                   NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ);

    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    adv->setName("MPU6050-Test");
    adv->addServiceUUID(SERVICE_UUID);
    bleServer->startAdvertising();
    // BLE is now advertising — device WILL appear in scan
    Serial.println("[ble] advertising as \"MPU6050-Test\"");

    // ── MPU6050 init (optional — BLE already running) ──────────
    bool mpuOk = mpu.begin();

    if (mpuOk) {
        Serial.printf("[i2c] LOCKED to SDA=GPIO%d  SCL=GPIO%d  addr=0x%02X\n",
                      i2cSdaPin, i2cSclPin, mpu.address());
        Serial.printf("[i2c] chip = %s  (WHO_AM_I = 0x%02X)\n",
                      mpu.chipName(), mpu.whoAmI());

        Serial.print("[i2c] devices seen:");
        for (uint8_t i = 0; i < mpu.diagCount; i++) {
            Serial.printf(" 0x%02X(WHO=%02X,match=%d)", mpu.diagAddr[i],
                          mpu.diagWho[i], mpu.diagMatch[i]);
        }
        Serial.println();

        // Pick the temperature formula that matches the die we actually found.
        // The MPU-6500/9250 use /321 + 25 instead of /340 + 36.53; using the
        // 6050 constants on a 6500 die reads roughly 11 °C too high.
        if (mpu.whoAmI() == 0x70 || mpu.whoAmI() == 0x71 || mpu.whoAmI() == 0x73) {
            TEMP_SCALE  = 321.0f;
            TEMP_OFFSET = 25.0f;
            Serial.println("[i2c] using MPU-6500/9250 temperature formula");
        } else {
            Serial.println("[i2c] using MPU-6050 temperature formula");
        }

        Serial.println("[cal] HOLD THE SENSOR STILL AND FLAT for ~5 seconds...");
        calibrateSensors();
        ahrs.setBeta(0.08f);
        ahrs.reset();
        sensorOk = true;
        Serial.printf("[cal] gyro bias = %.4f, %.4f, %.4f deg/s\n",
                      gyroBiasX, gyroBiasY, gyroBiasZ);
        Serial.printf("[cal] accel off = %.4f, %.4f, %.4f g\n",
                      accelOffsetX, accelOffsetY, accelOffsetZ);
        Serial.println("[ok]  streaming over BLE (and this USB port) now");
        Serial.println("[fmt] CSV: qw,qx,qy,qz,ax,ay,az,gx,gy,gz,tempC,sensorOk,count,roll,pitch,yaw,aMag,jerk,flags,impactG,swayHz,swayRmsG,cntImpact,cntDrop,cntSway,cntShake,cntTap");
    } else {
        Serial.println();
        Serial.println("!! NO IMU FOUND ON ANY PIN PAIR.");
        Serial.println("!! Check: VCC=3V3, GND=GND, and that SDA/SCL are on");
        Serial.println("!! GPIO4/GPIO5 (either order works).");
        Serial.println("!! BLE still advertises so you can connect and see SCAN:0");
    }
    Serial.println("======================================================");

    digitalWrite(LED_PIN, LOW);
}

// ═══════════════════════════════════════════════════════════════
//  Line writer — newline framing, sent to BOTH BLE and USB serial
// ═══════════════════════════════════════════════════════════════
//
// Emits `s` followed by '\n'.
//
// USB serial: written verbatim, so the Web Serial dashboard (or any
// terminal) sees exactly the same CSV stream.
//
// BLE: a notification can only carry (ATT MTU - 3) bytes.  If the line
// does not fit in one notification it is split across several at the
// negotiated payload size.  The browser reassembles on '\n', so a
// partial line is never mistaken for a complete one.
static void emitLine(const char* s, int len) {
    if (len <= 0) return;

    int n = len;
    if (n > (int)sizeof(bleLineBuf) - 2) n = (int)sizeof(bleLineBuf) - 2;
    memcpy(bleLineBuf, s, n);
    bleLineBuf[n++] = '\n';

    // ── USB serial mirror (always) ─────────────────────────────
    Serial.write((const uint8_t*)bleLineBuf, (size_t)n);

    // ── BLE notifications (only when a central is subscribed) ──
    if (!bleConnected || bleTxChar == nullptr) return;

    uint16_t chunk = bleChunkSize;
    if (chunk < BLE_CHUNK_FLOOR) chunk = BLE_CHUNK_FLOOR;

    for (int off = 0; off < n; off += chunk) {
        int take = n - off;
        if (take > (int)chunk) take = (int)chunk;

        bleTxChar->setValue((const uint8_t*)(bleLineBuf + off), (size_t)take);
        bleTxChar->notify();

        // Only pace ourselves when the line genuinely needs multiple
        // notifications; a single-notification line costs no delay.
        if (off + take < n) delay(3);
    }
}

// Pack the motion state into the single `flags` CSV field so the field
// count stays put:
//   bits 0-7  (0x00FF) transient events for THIS sample (EV_* in motion_detect.h)
//   bit  8    (0x0100) free-fall is happening right now
//   bit  9    (0x0200) at rest right now
//   bit 10    (0x0400) swaying right now
//   bit 11    (0x0800) shaking right now
// The transient bits drive the "just happened" flash; the live bits drive
// the steady YES/NO pill, which must not flicker off between samples.
static inline unsigned motionFlags() {
    unsigned f = (unsigned)motionOut.events;
    if (motionOut.freefall) f |= 0x0100u;
    if (motionOut.still)    f |= 0x0200u;
    if (motionOut.sway)     f |= 0x0400u;
    if (motionOut.shake)    f |= 0x0800u;
    return f;
}

// ═══════════════════════════════════════════════════════════════
//  Loop
// ═══════════════════════════════════════════════════════════════

void loop() {
    static unsigned long lastAhrsUs  = 0;
    static unsigned long lastBleUs   = 0;
    static unsigned long lastLedMs   = 0;
    unsigned long nowUs = micros();
    unsigned long nowMs = millis();

    // ── LED heartbeat ─────────────────────────────────────────
    if (nowMs - lastLedMs >= (bleConnected ? 200 : 600)) {
        lastLedMs = nowMs;
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    }

    // ── Read MPU6050 every AHRS tick (~200 Hz) ────────────────
    unsigned long elapsed = nowUs - lastAhrsUs;
    if (elapsed >= AHRS_US) {
        float dt = elapsed * 1e-6f;
        lastAhrsUs = nowUs;

        int16_t rawAx, rawAy, rawAz, rawGx, rawGy, rawGz, rawTemp;
        rawAx = rawAy = rawAz = rawGx = rawGy = rawGz = rawTemp = 0;
        bool ok = mpu.readRaw(rawAx, rawAy, rawAz, rawGx, rawGy, rawGz, rawTemp);
        sensorOk = ok;

        if (ok) {
            float ax = rawAx / ACCEL_LSB - accelOffsetX;
            float ay = rawAy / ACCEL_LSB - accelOffsetY;
            float az = rawAz / ACCEL_LSB - accelOffsetZ;
            float gx = rawGx / GYRO_LSB - gyroBiasX - gyroDriftX;
            float gy = rawGy / GYRO_LSB - gyroBiasY - gyroDriftY;
            float gz = rawGz / GYRO_LSB - gyroBiasZ - gyroDriftZ;
            float tempC = rawTemp / TEMP_SCALE + TEMP_OFFSET;

            ahrs.update(gx, gy, gz, ax, ay, az, dt);

            float qw_local, qx_local, qy_local, qz_local;
            ahrs.getQuaternion(qw_local, qx_local, qy_local, qz_local);
            qw = qw_local;
            qx = qx_local;
            qy = qy_local;
            qz = qz_local;
            accelX = ax;
            accelY = ay;
            accelZ = az;
            gyroX = gx;
            gyroY = gy;
            gyroZ = gz;
            temperature = tempC;

            // ── Euler angles from quaternion (ZYX Tait-Bryan) ──
            //  roll  = rotation about X (bank left/right)
            //  pitch = rotation about Y (nose up/down)
            //  yaw   = rotation about Z (heading)
            float sinr_cosp = 2.0f * (qw_local * qx_local + qy_local * qz_local);
            float cosr_cosp = 1.0f - 2.0f * (qx_local * qx_local + qy_local * qy_local);
            eulerRoll = atan2f(sinr_cosp, cosr_cosp) * 57.29578f;  // to degrees

            float sinp = 2.0f * (qw_local * qy_local - qz_local * qx_local);
            if (fabsf(sinp) >= 1.0f)
                eulerPitch = copysignf(90.0f, sinp);
            else
                eulerPitch = asinf(sinp) * 57.29578f;

            float siny_cosp = 2.0f * (qw_local * qz_local + qx_local * qy_local);
            float cosy_cosp = 1.0f - 2.0f * (qy_local * qy_local + qz_local * qz_local);
            eulerYaw = atan2f(siny_cosp, cosy_cosp) * 57.29578f;

            // ── Motion pattern detection (full 200 Hz) ────────
            {
                float av[3] = { ax, ay, az };
                float gv[3] = { gx, gy, gz };
                motionOut = motionDetector.update(av, gv, dt * 1000.0f);
            }

            // ── Stationary drift compensation ─────────────────
            //  When gyro magnitude is near zero (sensor still),
            //  slowly pull gyroDrift toward the residual rotation.
            float gyroMag = fabsf(gx) + fabsf(gy) + fabsf(gz);
            if (gyroMag < 1.0f) {  // < 1 deg/s total → nearly still
                gyroDriftX += gx * dt * 0.002f;   // ~2 second time constant
                gyroDriftY += gy * dt * 0.002f;
                gyroDriftZ += gz * dt * 0.002f;
            }
        }
    }

    // ── Emit one CSV line every tick (~33 Hz) ─────────────────
    // Deliberately NOT gated on bleConnected: the same stream is
    // mirrored to USB serial so the Web Serial dashboard works with no
    // Bluetooth at all.
    if (nowUs - lastBleUs >= BLE_US) {
        lastBleUs = nowUs;

        char buf[256];
        int len;

        // ── I²C diagnostics: (re)sent at boot, on BLE connect, and
        //    every DIAG_PERIOD_MS so a late listener still sees them. ──
        static uint8_t  diagPhase  = 0;
        static uint32_t lastDiagMs = 0;
        if (diagPending || (nowMs - lastDiagMs >= DIAG_PERIOD_MS)) {
            diagPending = false;
            lastDiagMs  = nowMs;
            diagPhase   = 0;
        }

        if (diagPhase < 3) {
            if (diagPhase == 0) {
                len = snprintf(buf, sizeof(buf),
                    "SCAN:%d", mpu.diagCount);
            } else if (diagPhase == 1 && mpu.diagCount > 0) {
                len = snprintf(buf, sizeof(buf),
                    "SCANWHO:%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X",
                    mpu.diagWho[0], mpu.diagWho[1],
                    mpu.diagWho[2], mpu.diagWho[3],
                    mpu.diagWho[4], mpu.diagWho[5],
                    mpu.diagWho[6], mpu.diagWho[7]);
            } else if (diagPhase == 2 && mpu.diagCount > 0) {
                len = snprintf(buf, sizeof(buf),
                    "SCANMATCH:%d,%d,%d,%d,%d,%d,%d,%d",
                    mpu.diagMatch[0], mpu.diagMatch[1],
                    mpu.diagMatch[2], mpu.diagMatch[3],
                    mpu.diagMatch[4], mpu.diagMatch[5],
                    mpu.diagMatch[6], mpu.diagMatch[7]);
            } else {
                len = 0;
            }
            diagPhase++;
        } else {
            // 16 original fields, then 11 appended fields.  Appending
            // keeps older consumers working: a parser that reads the first
            // 16 comma-separated values is unaffected by the tail.
            len = snprintf(buf, sizeof(buf),
                "%.4f,%.4f,%.4f,%.4f,%.3f,%.3f,%.3f,%.1f,%.1f,%.1f,%.1f,%d,%u,%.1f,%.1f,%.1f,"
                "%.3f,%.1f,%u,%.2f,%.2f,%.3f,%u,%u,%u,%u,%u",
                qw, qx, qy, qz,
                accelX, accelY, accelZ,
                gyroX, gyroY, gyroZ,
                temperature,
                sensorOk ? 1 : 0,
                packetCount,
                eulerRoll, eulerPitch, eulerYaw,
                motionOut.aMag,
                motionOut.jerk,
                motionFlags(),
                motionOut.impactG,
                motionOut.swayHz,
                motionOut.swayRmsG,
                motionOut.countImpact,
                motionOut.countDrop,
                motionOut.countSway,
                motionOut.countShake,
                motionOut.countTap);
        }

        if (len > 0 && len < (int)sizeof(bleLineBuf) - 1) {
            emitLine(buf, len);
            packetCount++;
        }
    }

    delayMicroseconds(200);
}