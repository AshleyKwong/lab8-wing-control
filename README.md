# lab8-wing-control
Control code for the 30 cm wing in BLWT at the University of Southampton.
ak1u24@soton.ac.uk
May 2025

nanotonano.ino - this is the control code for the nano that is connected to the lab pc. works via espnow connection to a second nano.

nanotoMega.ino - control code for the nano in UART connection with the Mega. ( I know it's weird daisy chaining but I could not get the Nano to drive the 4 linear actuators reliably ... in the end was too weak)

megatonano_rev2.ino - where the motor control codes live. for changing any of the timings or movements,this is where you will edit. Has the following functions :
1)
2)
3)
