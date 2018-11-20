#7 Segment Multi display Lap Counter

This project was designed for [Team FAST](https://www.facebook.com/FAST-Speed-Team-459974044024534/ "Team FAST") inline speed skating lap counting.  At speed meets, lap counters typically face the finish line, which isn't visible to most spectators.  A multi-sided display was needed to be seen from anywhere in a skating rink.

The displays need to be big enough and bright enough to be seen from at least 50 feet away.

The following is a list of skills needed to complete this project:
- Basic building skills (To build the enclosure)
- Ability to make soldering connections from wires to a PCB.
- Basic computing skills (to install the included software on the control board).

This is a relatively inexpensive project, but be warned, the costs can begin to mount if you have to purchase tools, and electronic components not in the hardware list provided.  It is also easy to spend money on parts for the enclosure.

###Hardware
- [NeoPixel Lights](http://a.co/d/82vFAID)
The lights come in 5 meter rolls.  They can be cut between every light on the copper contacts.  For this project, they will be cut into 6 light strips, 42 of them.  252 lights will be used in the 6 digits.

- [Itsy Bitsy M0 Express](https://www.adafruit.com/product/3727)
   - This is the control board for the project.  It was selected for a couple of reasons.  First of all, the NeoPixel lights need a 5V control signal.  Most arduino boards only supply 3.3V logic pins.  Fairly involved level shifting is required to get the 5V logic from a 3.3V pin.  The ItsyBitsy allows us to connect directly to the LED strips.
   - Secondly, it is small, cheap, and doesn't consume very much power.  But it has enough memory to drive a significant number of LEDs.

- [IR Sensor](http://a.co/d/hmyJgJH)
The remote in this sensor won't be used.  We want the sensor itself, which is mounted on a small PCB, which helps make the wiring a bit easier.

- Remote Controls
You don't have to use remotes listed below, but if you choose different remotes, you will need to change the code to recognize the proper signals.
  - [Number Pad](http://a.co/d/e0qm87g)
  This remote is for operating the lap counter.  It allows the operator to set the lap count and decrement the counter.
  - [Color Pad](http://a.co/d/i5sjCYd)
  The color remote isn't necessary for the lap counter, but it allows the operator to change the color and brightness of the display.  This could be important to those who care about power consumption, like being plugged into a bettery.

- Various Electronics
  - [Power Supply](http://a.co/d/hHfs552)
  - [JST Connectors](http://a.co/d/g6rFbLm)
  - [Quarter-sized proto board](http://a.co/d/5XRlcM6)
  - [Capacitors](http://a.co/d/czybLVH)
  - Optional Resistor
  It is a good idea to put a resistor on the logic pin going to the LED lights.  The NeoPixels listed above have resistors built in, but it still isn't a bad idea to add one on your proto board.

###The Enclosure
The enclosure in fairly simple to build.  It is a 3-sided box.  If you want it to be supported by an inexpensive camera tripod, it needs to be lightweight.  I used 1/4" PVC board, but 1/4" or 1/8" plywoood would work, too.

Simply cut the panels to size and screw them together, as shown.  Mount the tripod to the bottom of the enclosure using screws.

###Wiring it Up
[Fritzing Diagram](images/lapcounter_fritzing.png)

###Installing the Software
####Install the Arduino IDE
####Load the Sketch
####Compile and Upload the Sketch

###Finishing the Displays
