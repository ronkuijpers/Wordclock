# Wordclock: Getting Started & Maintenance

Welcome to your Wordclock! This quick guide walks you through the first setup and ongoing care of your device.

## 1. Product Overview
- **Indoor use only**; supply 5V DC from the provided adapter.
- Words light up to show the time using 161 NeoPixels.

## 2. Box Contents
- Wordclock unit
- 5V power adapter
- Printed instructions (this sheet)

## 3. Initial Power-Up
1. Plug in the power adapter.
2. Wait for the clock to boot (about 10 seconds). The front should light briefly.

## 4. Connect to WiFi
1. On first power-up (or after a WiFi reset) the clock creates a temporary WiFi network named **`Wordclock_AP`** with the password printed on your device.
2. Connect with your phone or computer and a setup page opens automatically. If not, visit `http://192.168.4.1`.
3. Select your home WiFi, enter the password, and save. The clock will reboot and join your network.

## 5. Open the Dashboard
1. After joining WiFi, find the clock's IP address in your router or use `http://wordclock.local` in most browsers.
2. The dashboard lets you:
   - Toggle the clock on/off
   - View the log output
   - Adjust brightness and LED color
   - Start a fun LED sequence
   - Restart or reset WiFi
   - Check for firmware updates or upload a `.bin` file

## 6. Time Synchronization
The clock syncs time from the internet using NTP servers automatically. No user action is required.

## 7. Firmware Updates
- Each night at 02:00 the clock checks for new firmware and installs it automatically if found.
- You can also trigger an update manually from the dashboard using **Check for updates** or by uploading a firmware file.

## 8. Resetting WiFi / Restarting
- Use the dashboard buttons **Reset WiFi** or **Restart Wordclock** when moving the device or troubleshooting.
- After a WiFi reset the setup access point reappears.

## 9. Troubleshooting & Support
If the clock won't connect or shows the wrong time:
- Verify your WiFi credentials.
- Restart via the dashboard or by briefly disconnecting power.
- Check the log on the dashboard for error messages.
- For additional help visit the project repository.

**Firmware version:** 0.2
