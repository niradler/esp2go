# USB HID Safety Guide

## ⚠️ IMPORTANT: Firmware Flashing

USB HID mode can interfere with firmware uploads because the ESP32-S3 presents itself as a keyboard/mouse device instead of a serial port.

## 🛡️ Safety Mechanisms

We've implemented **THREE** ways to safely flash firmware:

### Method 1: Safe Mode (Button Hold) ⭐ RECOMMENDED

**Easiest and safest method!**

1. **Hold the BUTTON** on your device
2. **Power on or press RESET** while holding button
3. **Keep holding for 3 seconds**
4. LED will blink **green/purple** to confirm safe mode
5. USB HID is now **disabled** for this session
6. You can now **flash firmware** normally!

```
Boot Sequence:
[Power On] → [Hold Button 3s] → [Green/Purple Blink] → [Safe Mode Active]
```

### Method 2: Config File Disable

**Permanent disable until you re-enable it**

1. Open `src/config.h`
2. Change:
   ```cpp
   #define USB_HID_ENABLED false  // Was: true
   ```
3. Flash firmware
4. USB HID will be permanently disabled
5. Re-enable when needed

### Method 3: Hardware Boot Mode

**Works even if firmware is completely broken**

1. **Power off** device
2. **Connect GPIO0 to GND**
3. **Power on** device
4. Device enters **bootloader mode**
5. Flash firmware
6. Disconnect GPIO0, reset device

## 🔍 How to Know You're in Safe Mode

### Serial Output:

```
⚠️  SAFE MODE ACTIVATED!
⚠️  USB HID will be DISABLED for this session
✅ You can now flash firmware safely
```

### LED Feedback:

- **6 rapid blinks** (green/purple alternating)
- Confirms safe mode is active

### Behavior:

- USB HID device **will NOT appear** on computer
- Device shows as **serial port** only
- Perfect for flashing!

## 📝 Configuration Options

In `src/config.h`:

```cpp
// Enable/disable USB HID completely
#define USB_HID_ENABLED true   // true = enabled, false = disabled

// How long to hold button for safe mode (milliseconds)
#define USB_HID_BOOT_TIMEOUT 3000  // 3 seconds
```

## 🎯 Best Practices

### During Development:

1. **Always test safe mode** before deploying
2. Keep a spare device without USB HID for testing
3. Document the button hold procedure for users

### For Production:

1. Train users on safe mode procedure
2. Print a sticker with instructions
3. Consider adding a physical "SAFE MODE" button

### Emergency Recovery:

If you're completely locked out:

1. Use **Method 3** (Hardware Boot Mode)
2. Flash firmware with `USB_HID_ENABLED = false`
3. Fix the issue
4. Re-enable USB HID

## 🔧 Technical Details

### Why USB HID Can Lock You Out:

- ESP32-S3 USB can operate in different modes
- **Serial Mode**: For flashing firmware (default)
- **HID Mode**: For keyboard/mouse emulation
- Only **one mode active** at a time
- HID mode **prevents** serial communication

### Our Solution:

- Check button state **during boot**
- **Before** USB HID initialization
- If button held → Skip USB HID
- Device stays in **serial mode**
- Flashing works normally!

### Boot Sequence:

```
1. Power On
2. Serial.begin() - Always available
3. Check Button (3 second window)
4. IF button held:
   - Skip USB HID init
   - Stay in serial mode
   - Blink LED for confirmation
5. ELSE:
   - Initialize USB HID
   - Device becomes keyboard/mouse
6. Continue with WiFi, SD card, etc.
```

## 🚨 Troubleshooting

### Problem: Can't Flash Firmware

**Solution:** Hold button during boot for 3 seconds

### Problem: Safe mode not working

**Possible causes:**

1. Not holding button long enough (needs 3 seconds)
2. Button wiring issue
3. Wrong GPIO pin

**Solution:** Use Method 3 (Hardware Boot Mode)

### Problem: USB HID not working after safe mode

**This is NORMAL!** Safe mode disables USB HID for that session.

**Solution:**

- Reset device WITHOUT holding button
- USB HID will initialize normally

### Problem: LED not blinking in safe mode

**Check:**

1. LED is wired correctly (GPIO 35)
2. M5.BtnA is correct button GPIO
3. Serial output shows safe mode message

## 📊 Feature Matrix

| Method         | Ease of Use          | Recovery Speed   | Works When Locked |
| -------------- | -------------------- | ---------------- | ----------------- |
| Button Hold    | ⭐⭐⭐⭐⭐ Very Easy | Instant          | ✅ Yes            |
| Config Disable | ⭐⭐⭐ Moderate      | Requires Reflash | ❌ No             |
| Hardware Boot  | ⭐⭐ Advanced        | Slow             | ✅ Yes            |

## 💡 Tips

1. **Test safe mode** before shipping devices
2. **Document** the button hold procedure
3. **Label** the button clearly
4. **Set** `USB_HID_ENABLED = false` during initial development
5. **Enable** USB HID only when you need it
6. **Keep** a backup device without USB HID

## 🎓 User Education

Print this for users:

```
═══════════════════════════════════════
   ESP2GO - USB HID Safe Mode
═══════════════════════════════════════

To flash new firmware:

1. HOLD the button
2. POWER ON the device
3. KEEP HOLDING for 3 seconds
4. LED will BLINK green/purple
5. RELEASE button
6. Flash firmware normally!

═══════════════════════════════════════
```

## ✅ Safety Checklist

Before deploying USB HID to users:

- [ ] Tested safe mode activation
- [ ] Verified LED blink feedback
- [ ] Documented button hold procedure
- [ ] Tested emergency recovery (Method 3)
- [ ] Added warning in web interface
- [ ] Trained support team
- [ ] Created recovery guide
- [ ] Tested with different computers
- [ ] Verified serial output messages
- [ ] Set appropriate timeout value

## 🔐 Security Note

Safe mode can be triggered by anyone with physical access. This is by design - it's a **safety feature**, not a security feature.

If you need to prevent USB HID disable:

- Remove the button check code
- Accept the risk of flash lockout
- Provide alternative recovery method

**Recommendation:** Keep the safety mechanism. Physical access = full control anyway.
