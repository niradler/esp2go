# Microphone Troubleshooting Guide

## Hardware: M5Stack AtomS3U with SPM1423 PDM Microphone

### Pin Configuration

- **PDM Clock**: GPIO 38
- **PDM Data**: GPIO 39

### Common Issues and Solutions

## Issue 1: Microphone Not Initializing

### Symptoms

- `M5.Mic.begin()` returns false
- Error message: "Failed to initialize microphone"

### Possible Causes

1. **M5.begin() not called first**

   - Solution: Ensure `M5.begin()` is called before `setupMicrophone()`

2. **Incorrect pin configuration in M5Unified**

   - The M5Stack AtomS3U has hardware-specific pin mappings
   - M5Unified should auto-detect pins, but verify in library

3. **I2S peripheral conflict**
   - PDM uses I2S internally
   - Check if other code is using I2S

### Debug Steps

```cpp
// Add this to setupMicrophone():
auto mic_cfg = M5.Mic.config();
Serial.printf("Pin CLK: %d\n", mic_cfg.pin_data_in);
Serial.printf("Pin DATA: %d\n", mic_cfg.pin_mck);
Serial.printf("I2S Port: %d\n", mic_cfg.i2s_port);
```

## Issue 2: Microphone Initialized but No Data

### Symptoms

- `M5.Mic.begin()` returns true
- `M5.Mic.isEnabled()` returns false
- `M5.Mic.record()` returns false or reads zeros

### Possible Causes

1. **Microphone not started**

   - M5Unified might require explicit start after begin()

2. **DMA buffer issues**

   - Buffer size mismatch
   - Insufficient DMA buffers

3. **Incorrect sample rate**
   - SPM1423 supports specific sample rates

### Solutions

#### Solution 1: Check isEnabled() status

```cpp
if (!M5.Mic.isEnabled()) {
    Serial.println("Microphone not enabled after begin()!");
    // Try different configuration
}
```

#### Solution 2: Adjust DMA configuration

```cpp
auto cfg = M5.Mic.config();
cfg.dma_buf_count = 16;  // Try more buffers
cfg.dma_buf_len = 256;   // Try different length
M5.Mic.config(cfg);
```

#### Solution 3: Try different sample rates

```cpp
// SPM1423 supports: 8000, 16000, 32000, 44100, 48000 Hz
cfg.sample_rate = 44100;  // Try higher rate
```

## Issue 3: Reading All Zeros

### Symptoms

- `M5.Mic.record()` returns true
- Buffer contains all zeros or very small values

### Possible Causes

1. **PDM to PCM conversion not working**
2. **Microphone hardware fault**
3. **Gain too low**

### Solutions

#### Check raw values

```cpp
bool success = M5.Mic.record(micBuffer, MIC_BUFFER_SIZE);
Serial.printf("First 10 samples: ");
for (int i = 0; i < 10; i++) {
    Serial.printf("%d ", micBuffer[i]);
}
Serial.println();
```

#### Try M5Unified's built-in test

```cpp
// In setup():
M5.Mic.begin();
delay(1000);

// In loop():
if (M5.Mic.isAvailable()) {
    auto data = M5.Mic.data();
    Serial.printf("Available samples: %d\n", data.size());
}
```

## Issue 4: High RMS but Low Audio Level

### Symptoms

- RMS value is high (e.g., 5000-10000)
- Calculated level is very low (0-5%)

### Cause

Incorrect scaling factor in normalization

### Solution

Adjust the scaling in `readMicrophoneLevel()`:

```cpp
// Current (may be wrong):
currentAudioLevel = min(100, max(0, (rms * 100) / 10000));

// Try different scaling:
currentAudioLevel = min(100, max(0, (rms * 100) / 1000));  // More sensitive
// or
currentAudioLevel = min(100, max(0, rms / 100));  // Even more sensitive
```

## Testing Procedure

### 1. Upload Dedicated Test Firmware

Use the `test/mic_test.cpp` standalone test:

```bash
# Copy test config
cp test/platformio_test.ini platformio.ini.backup
cp test/platformio_test.ini platformio.ini

# Build and upload
pio run -e mic_test --target upload

# Monitor output
pio device monitor
```

### 2. Expected Output

```
========================================
   M5Stack AtomS3U Microphone Test
========================================

✅ Microphone initialized successfully!
  - Is Enabled: YES
  - Sample Rate: 16000 Hz

✅ First successful read from microphone!

Sample #1 | RMS:  1523 | Level:  15% | Peak:  3421 | Range: [ -3421,   2847] | [███·················]
Sample #2 | RMS:  2104 | Level:  21% | Peak:  4982 | Range: [ -4982,   4234] | [████················]
```

### 3. Troubleshooting Based on Output

**If you see "FAILED to initialize microphone":**

- Check M5Unified library version (needs >= 0.2.0)
- Verify hardware connection
- Try power cycle

**If you see "Microphone is NOT enabled":**

- M5Unified version issue
- Try calling M5.Mic.begin() multiple times
- Check for I2S conflicts

**If you see "Failed to read microphone data":**

- Increase delay between reads
- Check DMA buffer configuration
- Try lower sample rate

**If you see all zeros in buffer:**

- Hardware fault
- Wrong pin configuration in M5Unified
- PDM to PCM conversion failed

## Advanced Debugging

### Check M5Unified Source Code

Look at the M5Unified library to see actual pin definitions:

```bash
# Find M5Unified library location
pio pkg show m5stack/M5Unified

# Check mic configuration
# Look in: .pio/libdeps/*/M5Unified/src/utility/Speaker_Class.cpp
```

### Use Logic Analyzer

If available, probe the PDM signals:

- **Clock (GPIO 38)**: Should see square wave at PDM clock rate
- **Data (GPIO 39)**: Should see PDM bitstream (high-frequency switching)

### Check Power Supply

- SPM1423 requires stable 3.3V
- Measure voltage on microphone VDD pin
- Check for noise on power rail

## M5Unified API Reference

```cpp
// Configuration
auto cfg = M5.Mic.config();
cfg.sample_rate = 16000;        // Sample rate in Hz
cfg.stereo = false;             // true=stereo, false=mono
cfg.dma_buf_count = 8;          // Number of DMA buffers
cfg.dma_buf_len = 128;          // Samples per DMA buffer
cfg.task_priority = 2;          // FreeRTOS task priority
cfg.task_pinned_core = 1;       // CPU core (0 or 1)
M5.Mic.config(cfg);

// Initialization
bool success = M5.Mic.begin();

// Status
bool enabled = M5.Mic.isEnabled();
bool available = M5.Mic.isAvailable();
size_t rate = M5.Mic.getSampleRate();

// Reading
bool success = M5.Mic.record(buffer, samples);
// or
auto data = M5.Mic.data();  // Returns array-like object
```

## Known Working Configuration

```cpp
auto cfg = M5.Mic.config();
cfg.sample_rate = 16000;
cfg.stereo = false;
cfg.dma_buf_count = 8;
cfg.dma_buf_len = 128;
cfg.task_priority = 2;
cfg.task_pinned_core = APP_CPU_NUM;
M5.Mic.config(cfg);

if (M5.Mic.begin()) {
    // Success!
}
```

## Additional Resources

- [M5Unified GitHub](https://github.com/m5stack/M5Unified)
- [M5Stack AtomS3U Schematic](https://docs.m5stack.com/en/core/AtomS3U)
- [SPM1423 Datasheet](https://www.knowles.com/docs/default-source/model-downloads/spm1423hm4h-b-datasheet-rev-g.pdf)

## Quick Commands Reference

```bash
# Upload main firmware
pio run --target upload
pio device monitor

# Upload test firmware
pio run -e mic_test --target upload
pio device monitor

# Clean build
pio run --target clean

# Check dependencies
pio pkg list
```
