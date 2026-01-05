# xLights Testing Guide for DDPico

Complete guide to test your DDPico LED controller with xLights.

## 🔧 Prerequisites

- ✅ Raspberry Pi Pico with updated firmware (with `[DDPico]` prefix)
- ✅ DDPico bridge running (`start_bridge.bat`)
- ✅ Web dashboard open at http://localhost:4000
- ✅ xLights installed
- ✅ LED strip connected to Pico GPIO 16

## 📋 Step-by-Step Setup

### 1. Start the DDPico Bridge

```bash
cd DDPico/ddp
start_bridge.bat
```

**Expected Output:**
```
[WEB] Dashboard available at http://localhost:4000
[SERIAL] Connected to COM6 @ 921600 baud
[UDP] Listening on 0.0.0.0:4048
[BRIDGE] DDP Serial Bridge started
```

### 2. Open Web Dashboard

- Browser should auto-open to http://localhost:4000
- Verify "Connected" status badge is green
- Check that Serial Port shows your COM port

### 3. Configure xLights Controller

#### A. Open xLights Setup

1. Launch xLights
2. Go to **Setup** tab
3. Click **Controllers** button (or press F5)

#### B. Add DDP Controller

1. Click **Add Ethernet** button
2. Select **DDP** from the protocol dropdown
3. Configure the controller:

   ```
   Protocol:        DDP
   IP Address:      127.0.0.1
   Port:            4048
   Description:     DDPico LED Controller
   Vendor:          Generic
   Model:           DDP
   ```

4. Click **OK** to save

#### C. Configure Output

1. In the Controllers list, find your new DDP controller
2. Set the following:
   ```
   Start Channel:   1
   Channels:        300    (100 LEDs × 3 colors = 300 channels)
   ```

3. Click **Visualizer** button to see the output mapping

### 4. Create a Simple Model

#### A. Add Model

1. Go to **Layout** tab
2. Click **Add** → **Matrix** (or any model type)
3. Configure:
   ```
   Name:            Test Strip
   String Type:     RGB Nodes
   Strings:         1
   Nodes/String:    100
   Start Channel:   1
   ```

4. Click **OK**

#### B. Position Model

- Drag the model in the layout view
- Resize as needed
- The model should now be linked to your DDP controller

### 5. Test with Built-in Effects

#### A. Open Sequencer

1. Go to **Sequencer** tab
2. Create a new sequence or open existing one

#### B. Add Test Effects

1. **Color Wash Test:**
   - Drag **Color Wash** effect onto your model
   - Set colors to Red, Green, Blue
   - Duration: 3 seconds
   - Play the sequence

2. **Morph Test:**
   - Drag **Morph** effect onto your model
   - Set start color: Red
   - Set end color: Blue
   - Duration: 2 seconds
   - Play the sequence

3. **Fire Test:**
   - Drag **Fire** effect onto your model
   - Adjust height and intensity
   - Play the sequence

### 6. Enable Output to Lights

**IMPORTANT:** Make sure Output to Lights is enabled!

1. In the Sequencer tab, look for the **Output to Lights** button (lightbulb icon)
2. Click it to enable (should turn green/highlighted)
3. Or press **F8** to toggle

### 7. Monitor in Web Dashboard

While playing effects in xLights, watch the web dashboard:

**Expected Behavior:**
- **Packets Sent** counter increasing rapidly
- **Data Sent** showing KB transferred
- **Throughput** showing KB/s (typically 2-10 KB/s)
- **Packet Rate Chart** showing activity
- **Console** showing DDP packet logs (if enabled)

**Example Console Output:**
```
[DDP] 139 bytes from 127.0.0.1:xxxxx
[STATS] RX: 0 pkts, 0 bytes | TX: 1234 pkts, 171626 bytes | Idle: 0.0s
```

### 8. Verify LED Output

Your LED strip should now display the effects from xLights!

**If LEDs are not updating:**
1. Check web dashboard shows packets being sent
2. Verify Pico is powered and connected
3. Check LED strip connections (GPIO 16, GND, 5V)
4. Verify NUM_LEDS in firmware matches your strip
5. Upload updated firmware with `[DDPico]` prefix to see debug

## 🐛 Troubleshooting

### Problem: No packets in dashboard

**Solution:**
- Verify xLights Output to Lights is enabled (F8)
- Check DDP controller IP is 127.0.0.1:4048
- Restart xLights and try again

### Problem: Packets sent but LEDs not updating

**Solution:**
1. Upload updated firmware to see Pico debug messages
2. Check web console for `[DDPico]` messages
3. Verify LED strip is connected to GPIO 16
4. Check NUM_LEDS setting in firmware matches your strip
5. Test with simple solid color effect first

### Problem: Connection errors in console

**Solution:**
- These are normal when browser refreshes
- The new version suppresses these errors
- Restart bridge if needed: `start_bridge.bat`

### Problem: Slow/choppy animation

**Solution:**
- Check throughput in dashboard (should be 2-10 KB/s)
- Reduce effect complexity in xLights
- Verify USB cable quality
- Check CPU usage on PC

## 📊 Performance Expectations

**Normal Operation:**
- Packet Rate: 20-60 packets/second
- Throughput: 2-10 KB/s
- Latency: <50ms
- Frame Rate: 20-40 FPS

**100 LEDs @ 30 FPS:**
- Data per frame: 300 bytes (100 LEDs × 3 colors)
- Packets per second: ~30
- Throughput: ~9 KB/s

## 🎨 Recommended Test Sequence

1. **Solid Colors** (3 sec each)
   - Red → Green → Blue → White

2. **Color Wash** (5 sec)
   - Smooth transitions

3. **Chase Effect** (5 sec)
   - Single pixel moving

4. **Fire Effect** (5 sec)
   - Animated flames

5. **Rainbow** (5 sec)
   - Full spectrum

## 💡 Tips for Best Results

1. **Start Simple:** Test with solid colors first
2. **Monitor Dashboard:** Keep web dashboard open to see real-time stats
3. **Check Logs:** Enable console filters to see specific message types
4. **Use Test Mode:** xLights has a test mode (Tools → Test)
5. **Save Settings:** Export xLights controller config for backup

## 📝 xLights Test Mode

Quick way to test without creating sequences:

1. Go to **Tools** → **Test**
2. Select your DDP controller
3. Choose test pattern:
   - **Chase:** Moving pixel
   - **Chase 3:** Three moving pixels
   - **Chase 4:** Four moving pixels
   - **Alternate:** Alternating colors
   - **Twinkle:** Random twinkling
   - **Shimmer:** Shimmering effect
   - **Background Only:** Solid color
   - **RGB Cycle:** Color cycling

4. Click **Start** to begin test
5. Watch LEDs and dashboard for activity

## 🔍 Debug Checklist

Before asking for help, verify:

- [ ] Bridge is running (check terminal)
- [ ] Web dashboard shows "Connected"
- [ ] xLights Output to Lights is enabled
- [ ] DDP controller configured correctly (127.0.0.1:4048)
- [ ] Packets are being sent (dashboard counter increasing)
- [ ] Pico is powered and connected
- [ ] LED strip is connected to correct GPIO pin
- [ ] Firmware is uploaded to Pico
- [ ] NUM_LEDS matches your strip length

## 📚 Additional Resources

- **xLights Manual:** https://manual.xlights.org/
- **DDP Protocol:** https://github.com/marmilicious/DDP_Protocol
- **Raspberry Pi Pico:** https://www.raspberrypi.com/documentation/microcontrollers/

## 🎉 Success Indicators

You'll know everything is working when:

✅ Web dashboard shows increasing packet counts
✅ Throughput displays active KB/s
✅ Chart shows packet rate activity
✅ LEDs respond to xLights effects in real-time
✅ No errors in web console
✅ Smooth animations with no flickering

Happy testing! 🚀
