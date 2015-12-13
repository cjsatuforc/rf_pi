/*
    send.cpp by @n8henrie
    https://github.com/n8henrie/rf_pi


    - Modified from https://github.com/ninjablocks/433Utils/blob/master/RPi_utils/codesend.cpp
    - which was modified from https://github.com/ninjablocks/433Utils/blob/master/Arduino_sketches/Send_Demo/Send_Demo.ino
    - which was modified from https://github.com/r10r/rcswitch-pi/blob/master/send.cpp

    ## Usage:

    - Set `PIN`, `PULSELENGTH`, and `BITLENGTH` if different than the defaults
    - `./send 12345` where 12345 is your *decimal* RF code (not binary or Tri-State)

    ## Important changes:

    - Externalizes the `send` function to make it available to other languages as a
      shared library
    - Jessie branch uses wiringPiSetupGpio() and sets WIRINGPI_GPIOMEM to use
      /dev/gpiomem instead of wiringPiSetup() to avoid need for root privileges
    - Wheezy branch uses wiringPiSetupSys() (and depends on pre-exported GPIO pins)
      instead of wiringPiSetup() to avoid need for root privileges
        - Wheezy branch **requires** that gpio pin have been exported by wiringPi2
          gpio function beforehand due to use of wiringPiSetupSys, e.g. `sudo -u
          $gpio_user /usr/local/bin/gpio export $pin out`
    - If run as sudo / root *or* set to have `setcap cap_sys_nice+ep`, both
      branches will set high priority scheduling for more reliable RF transmission,
      but still runs otherwise
    - See README.md for other details 
*/

#include "RCSwitch.h"
#include <stdlib.h>
#include <sched.h>
#include <string>
#include <cstring>

extern "C" int send(int num_args, int iterations, int switches[]) {

    // Have to use the BCM pin instead of wiringPi pin (e.g. 17 instead of 0)
    // if using wiringPiSetupGpio() or wiringPiSetupSys(). See:
    // http://wiringpi.com/pins/
    int PIN = 17;
    int PULSELENGTH = 190;
    int BITLENGTH = 24;

    // Set the env variable to use /dev/gpiomem for easier rootless access.
    // Will *not* overwrite the existing value, so if you know you don't want
    // this, either `export WIRINGPI_GPIOMEM=0` beforehand or comment the line
    // out.
    // http://wiringpi.com/wiringpi-update-to-2-29/
    setenv("WIRINGPI_GPIOMEM", "1", 0);

    if (wiringPiSetupGpio() == -1) return 1;
	RCSwitch mySwitch = RCSwitch();
	mySwitch.enableTransmit(PIN);
    mySwitch.setPulseLength(PULSELENGTH);

    // Make iteractions the outer loop and rotate through the switches inside
    // to try to maximize reliability; i.e. if you're tring to toggle 3 switches
    // with codes 1, 2, and 3, and iterations = 2, send `1 2 3 1 2 3` instead of 
    // `1 1 2 2 3 3`, which may have a higher risk of a brief timing glitch screwing
    // up a single code's transmission entirely.
    for (int i=0; i<iterations; i++) {
        for (int n=0; n<num_args; n++) {
           
            // Uncomment to print to stdout. Note: will print each one $iterations times
            // printf("sending code: %i\n", switches[n]);
            
            mySwitch.send(switches[n], BITLENGTH);
        }
    }

    return 0;
}

int main(int argc, char *argv[]) {
   
    // argv includes executable as argv[0], so we'll need to strip off argv[0]
    // and decreased argc by 1 to make sure send() can receive switches from
    // either the command line via main() or ctypes from python and expect
    // the same number of args
    int num_args = argc - 1;
    int args[num_args];
    for (int i=0; i<num_args; i++) {
        args[i] = std::stoi(argv[i+1]);
    }

    // Set scheduling if program is run directly from command line,
    // which is otherwise taken care of in Python
    //
    // This does the same thing as the `struct` stuff below. 20150208
    // (void)piHiPri (99) ;

    struct sched_param sched;
    std::memset (&sched, 0, sizeof(sched));
    sched.sched_priority = sched_get_priority_max (SCHED_RR);
    sched_setscheduler (0, SCHED_RR, &sched);
    
    send(num_args, 3, args);
    return 0;
}