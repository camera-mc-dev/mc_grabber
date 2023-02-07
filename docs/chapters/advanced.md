There are several "advanced" concepts relevant to using the grabber, these include:


  1) Timing signals and Synchronisation with Qualisys
  2) Fake cameras (for debugging and development of the grabber)
  3) Image sender
  4) Framegrabber board order
  5) Dual cable connection to the cameras
  6) Framegrabber driver and SiSo runtime
  7) Hardware debugging


## Timing signals, and Qualisys Synchronisation

The 3 framegrabbers are configured such that the top grabber is responsible for generating a time signal that the other grabbers are set up to respond to. Stemmer provided some technical documentation about this with delivery of the system. That document can be found in the `mc_grabber/docs/stemmer/` directory.

When the grabbers are initialised, `sisoRec` will make use of two configuration files `MS_Generator_Master_ME.mcf`, and `MS_Generator_Slave_ME.mcf` to give the grabbers the right configuration. It is _assumed_ that the "first" grabber reported by the system is the top grabber and thus the "master". If this is not the case, see the section regarding board-order later.

The DMA 0 (i.e. the first port), on the "master" grabber, is configured to generate a regular pulse. All cameras are set to listen to that pulse and to trigger image capture on the pulse. Thus, the pulse signal controls the framerate.

As well as being sent to all cameras, the timing pulse is sent to the "Opto-trigger" board (positioned between GPU and top grabber as of Feb 2023). An external cable (custom cable, should be near Grabbie in the ABS lab) can connect to Port A of this OptiTrigger board, and export the time signal.

The custom cable provided a BNC connector, which will allow this signal to be input into the Qualisys hardware. Qualisys then has configuration options to trigger its cameras based on the input signal.

While this signal can pretty much guarantee a Qualisys data frame being synchronised against the camera frames, the are complications with regards to stopping and starting Qualisys recording.

Obviously, `sisoRec` is constantly running the cameras, and waiting for the operator to "bang the space bar" to signal the end of a trial and that recording should be dumped to disk. To enable that "stop" to also trigger Qualisys to stop, a second signal is sent from the top grabber, out through the Optitrigger.

As of Feb-2023, `sisoRec` is configured to keep that second output "high" while images are being captured, and to then set it "low" when a trial is complete. Qualisys can be set to stop a recording when the output changes to low. You will however have to have alrady started recording the trial in Qualisys manually.

In setting the signal to be "high" normally, and "low" when saving / at the end of a trial, it is possible to set up Qualisys to use the hand help push switch to start a trial.

Complications occur in that it can't be guaranteed that Qualisys will stop on exactly the same frame as `sisoRec`. This can lead to time alignment issues between the two systems' recordings.

To this end, there is also an Arduino in a box. The Arduino has a simple program which flashes a pair of LEDs and sends that same flash signal out in a format which can be input into Qualisys as an analog signal.

The `mc_reconstruction` repository provides the `timeAlign` tool to detect the flashing LEDs in the image, and to match those flashes to the signal on an analog channel of a `.c3d` motion capture file, thus resolving the time alignment problem.

## Fake cameras / grabbers

To enable development of `sisoRec` away from the actual capture system, code was created to allow the use of `fake grabbers`. Instead of having a physical camera, an on-disk video is treated as if it was a camera. This has no practical value except as a debugging and development tool.

To use it, start `sisoRec` as:

```bash
  $> /opt/mc_bin/mc_grabber/sisoRec 1 <video file>
```
for 4 cameras using the same video file, or

```bash
  $> /opt/mc_bin/mc_grabber/sisoRec 1 <video file 0> <video file 1> ... <video file n>
```

for `n` cameras using `n` video files.

## Image sender

The image sender option in `sisoRec` should still be enabled, but there is not currently a tool to receive and display the image. Making such a tool would be quite simple using the `ImageReceiver` class and basic renderer in `mc_core`.


## Frame grabber board order

In the event that the top grabber is for some reason not the "first" grabber reported by the system, you can adjust the board order using the tools provided with the `SiSo` API / Runtime.

First, navigate to `/opt/software/framegrabbers/SISO-Runtime5.7.0/bin/impl`

Then, start the SiSo generic service: `./gs start`

Then, start the `microDiagnostics` tool `./microDiagnostics`

NOTE: You may want to use two terminals. One one terminal, start the service using `./gs run` and on the other the `microDiagnostics` tool. This way you can see status updates from the generic service printed to the command line.

You will find a tool in the `microDiagnostics` menus that allows you to change the board order.

NOTE: The changed board order is saved a configuration _local to the user_ in their home directory `~/SiliconSoftware/Runtime/board_order.cfg` and as such must be configured for every user. Fortunately, you can just copy the config for user to user once you get it right.

## Dual cable connection to each camera

If `1920x1080` at 200 fps is just not enough for you, then you can connect 2 CoaXPress cables to each camera. This allows for a much higher data rate between camera and computer, and should allow (iirc)  211 fps at `2560x2048`, or possibly, 300-400 fps at `1920x1080`.

If you want to do this, it will take a bit of effort. Not only will you need to create new configuration files for the grabbers, you will also need to "flash" different a different firmware applet onto the camera. This can be done with the `microDiagnostics` tool. You will probably need to use the `microDisplayX` tool to refine the configuration and export a configuration file.

We have, once, experimented with flashing the app and trying to set up the configuration, but only out of playful curiosity - got scared about breaking things and put it back to normal.

`sisoRec` has _never_ been tested with this camera configuration.

You would be able to run 2 cameras per grabber (in theory) with this configuration if you do need the speed / resolution.

## Framegrabber driver and SiSo runtime

The framegrabber driver (module) is called `menable`, and it needs to be recompiled every time there is an update to the Linux kernel.

When you can't find any cameras or grabbers on the system, the first thing to do is check that the module is installed on the system.

```bash
 $> lsmod | grep menable
```

If it is not there then the system has not been booted with the "correct" kernel version.

As of Feb-2023 you need the kernel version 5.4.

To recompile and install the module simply do:

```bash
cd /opt/software/framegrabbers/menable/
make clean
make
sudo make install
sudo modprobe menable
```

If the kernel is too new you _will_ get compilation errors because the kernel API has changed slightly and the module's source code is now too old.

It _is_ possible to hack the module's source code to get a successful compile and kernel install.

However, if you really need to be using a newer kernel, the _right_ solution is to use a newer version of the driver.

Just before abandoning the framegrabbers used by Grabbie, Basler (who presumably bought out SiliconSoftware) released an updated runtime and driver. You can find a download of that under `/opt/software/framegrabbers/basler/`

There may be API changes and configuration changes that affect `sisoRec` in as-yet unknown ways, so, as yet, it has not been deemed timely to update the driver etc.

*NOTE*: If you have having problems with the grabbers, it can be very useful to familiarise yourself with the various SiSo runtime tools, like `microDisplayX`, `microDiagnostics` etc. `microDisplayX` is particularly useful for showing the current temperatures of the grabbers. You may also find it advantageous to see what the system log reports wrt. the menable module: `$> dmesg | grep menable`
