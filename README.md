# pico-gdo 


This repository contains C++ code intended for a Raspberry PI Pico W.  It's purpose is to replace the user interface in my repository called GDO.  GDO contains an Android app.  The app sent the operator button pushes to a Raspberry PI 0 that had the garage-door-opener (see that repository) software running on it.  That processor would signal a GPIO pin to close a relay that would then push the garage door button and open/close the door.  This implementation requires no phone but instead a Pico W.  It has no real human interface except that the human powers the processor on.  When powered on, the Pico sends "button presses" to the existing Raspberry PI 0 until it replies back that the command has been accepted.

Parts:
  - Raspberry PI Pico W
  - Computer capable of programming the Pico W

Software:
  1)  Install the Pico SDK on the development computer
  2)  git clone https://github.com/mbroihier/pico-gdo.git
  3)  cd pico-gdo
  4)  mkdir build
  5)  cd build
  6)  cmake ..
  7)  make
  8)  install gdo_client.uf2 to pico
  9)  pair the garage door opener Raspberry PI 0 to the Pico W
Once paired, whenever the Pico boots (is powered on) it will send button push requests to the garage door until a successful response is sent back.
