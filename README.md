# Battery-and-Temperature-monitor
!2v Battery monitor
I made this to use with my EV that has a 12v battery for use with various equipmrnt in the car 
when it gets low various problems occur so I wanted to monitor when the battery was getting low
It uses the ESP32-C3 Super Mini a INA 219 and a DS18b20. Data is sent to my LoRa network every 15mins
it is powered by a pair of duracell AA batteries and it expected to last the shelf life of the cells
I use a P Channel MOSFET to control power to the ina219 and the ds18b20.
