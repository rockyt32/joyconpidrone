Nintendo Switch Joycon Controlled Drone
Control a drone using nintendo switch dual joycons!

Important
The code in joycon.cpp does not belong to me. It belongs to shinyquagsire23 and came from https://github.com/shinyquagsire23/HID-Joy-Con-Whispering. All I did was put the code into a "library" (seperate c and h files), since he had it all in main, and add versions of send command with timeouts.

What you will need
- A drone with a flight controlled that supports Multiwii Serial Communications (An FC with CleanFlight/BetaFlight will support this)
	- Note: This is not a drone building guide. There are plenty of good ones that can be found with a quick search.
	- When building your drone, make sure you have space to install your raspberry pi
- A raspberry pi (I used 3A+)
- A computer device that can establish a good bluetooth connection

Setting up the raspberry pi
- Download the joyconpidrone/dronepi/ folder on your pi
- Change the directory and run make

Setting up the ground station
Windows:
- Install Visual Studio
- Download the joyconpidrone/joycon_interface/windows folder with the visual studio project
- Install the hidapi from https://github.com/signal11/hidapi
- After building hidapi, copy the hidapi.dll and hidapi.lib file and put them in the main folder of the joycon_interface
- Build the joycon interface

Linux:
- Download the joyconpidrone/joycon_interface/linux
- Install the hidapi from https://github.com/signal11/hidapi and build it the manual way
- In the joycon interface makefile, you may need update COBJS and INCLUDES to reflect the location of when you installed hidapi
- Change directory to the joycon interface and run make

Why not a direct connection?
- Nintendo switch joycon bluetooth connection only have a range of about 10m, so there would be a lot of disconnects
- A better alternative to using wi-fi would be using radio for better range (this would require extra components and understanding of the SPI communication protocol)

Cleanfligt/Beta Flight setup instructions
- Check out a tutorial on how to use cleanflight/betaflight if you don't know how
- Make sure the flight controller is setup to use MSP for controls
- Make sure the the FC gets armed and goes to angle mode with aux1 and/or aux2 is below 1500 (the one you choose does not matter)
- Probably want to turn off failsafe more (I personally had problems with the FC accepting MSP commands when failsafe was on)


How to use:
- Optional (but recommended):
	- Check out main.cpp of the joycon interface. You can uncomment //#define INVERT_CONTRL if you want interveted vertical controls
	- Check out main.c of the dronepi:
		- There is a macro called POWER with a default value of 0.5. This represents 50% power. You can update this to a value from 0 to 1 depending on your drone/preference
		- There is a marco called THROTTLE_BASE with a default value of 1150. This the default/base throttle when the drone is armed. You can update this up or down depending on your drone
- Connect you right and left switch joycons via bluetooth to the device with the joycon interface
- Have both your raspberry pi and ground station connected to the same wifi network
- Find the ip adresss of your raspberry pi under this wifi connection
- Start the drone pi (must be in the drone pi folder) using the command "sudo ./drone_pi". It will now wait for the joycon interface to connect
- Run the joycon interface programe with the pi's IP address as command line argument
- The drone should be armed and ready to fly!
- To exit, press the home button on the joycons (also, with automatically exit upon disconnection)

Controls:
- Left joycon joystick up/down to go up/down
- Left joycon joystick left/right to go left/rigth
- Right joycon A button to go forward
- Right joycon home button to disarm and exit

Potential future updates:
- Add raspberry pi camera support so that a headset can be used
- More sophisticated controls: current setup does not support rolling, and commands are only 1 byte each. Expand to maybe five bytes (1 byte for throttle, 1 byte for yaw, 1 byte for pitch, 1 byte for roll, last byte for home button, aux, and error checking)
