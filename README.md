# Wiimote Emulator

Emulates a Bluetooth Wii controller in software.

![Raspberry Pi 3 running the emulator in Raspbian](rpi_ss.png)

### What does this fork do?
  - adds an input visualizer to the wmmitm tool
  - will add DTM (TAS) playback support in the future
  - TODO: possible to connect multiple wiimotes from 1 process? Note: continuous reporting mode needs to assume that different controllers use different reporting modes

### Features

  - Emulate the Wiimote's many features and extensions
  - Allows use of different input devices (keyboard etc.)

### Build/Install

The following dependencies/packages are required (if not already installed):

  - libdbus-1-dev
  - libglib2.0-dev
  - libsdl1.2-dev
  - libsdl2-dev
  - libsdl2-image-dev

Run the build script (in the project directory):

  > source ./build-custom.sh

For more information on the build script, see [this explainer](https://github.com/rnconrad/WiimoteEmulator/blob/master/CustomBuild.md).

### Using the Emulator

Stop any running Bluetooth service, e.g.:

  > sudo service bluetooth stop

Start the custom Bluetooth stack (e.g. from the project directory):

  > sudo ./bluez-4.101/dist/sbin/bluetoothd

Run the emulator (in the project directory):

  > ./wmemulator

With no arguments, the emulator will listen for incoming connections (similar to
syncing a real Wiimote). Pressing the sync button on a Wii should cause it to
connect.

You can also supply the address of a Wii to directly connect to it as long as
you have connected to it before (or you change your device's address to the
address of a trusted Wiimote).

  > ./wmemulator XX:XX:XX:XX:XX:XX

You will need to run the custom Bluetooth stack (as described above) whenever
using the emulator (it won't persist after e.g. a device restart). Also, the
custom stack generally won't be useful for anything besides Wiimote emulation.

To stop the custom stack and restore the original Bluetooth service, e.g.:

  > sudo killall bluetoothd

  > sudo service bluetooth start

### TAS Playback
To playback a sequence of inputs made with Dolphin Emulator:
1. Rename your desired TAS file to `tas.dtm` and put it in the same folder as `wmemulator`.
2. In `taskey.txt`, put the controller extension encryption key. If your TAS doesn't use an extension, use a key filled with 0s. Finding the key can be done with a lua script or RAM watch during playback in Dolphin Emulator.
3. Run `./wmemulator`, connect to your wii, and then press `Shift+T` to play it back.

### Using the Input Visualizer
As with the emulator, ensure that the custom Bluetooth stack is running. Then run

 > sudo ./wmmitm

To speedup the connecting process, you can provide the bluetooth address of the wiimote and wii you are connecting to:

 > sudo ./wmmitm -wm XX:XX:XX:XX:XX:XX -wii XX:XX:XX:XX:XX:XX

For games that use a continuous reporting mode, you can force the visualizer to send input report more frequently with the flag `-d <max forwarding delay>`. This will try to send data every d microseconds. If the delay is too small, it will send data much more often than the console can handle causing it to buffer your inputs significantly (which will feel like a large amount of input delay). The default value is 2500μs (2.5ms)
For example, to ensure that an input report is sent to the wii every 4ms, run it with:

 > sudo ./wmmitm -d 4000

Also, to see the data being sent between the wii and wiimote, use the debug flag:

 > sudo ./wmmitm -debug

To stop displaying a button from the visualizer, edit the layout in the `./config` folder and set both x and y to -1.
Then, click on the input visualizer, then press `CTRL+R` to refresh the graphics.
