# HITERM

Before the DEC VT100, there were "dumb" terminals such as the Lear Siegler ADM-3A.  

The ADM-3A could at least clear its screen and position the cursor. That, we can work with.  

HITERM runs on ESP8266 devices (like NodeMCU) to connect the terminal to wifi. Then you can telnet to a host and HITERM will do its best to translate vt100 commands to something the terminal can work with.  

Some examples:  
1) Play Dope Wars on sdf.org.  
2) Play Inform/z-machine text adventure games.  
3) Connect to IRC using irssi.  

Currently supports the Lear Siegler ADM-3A.  

TODO:  
Hazeltine 1500  

## QUICK START

Load hiterm.ino in the Arduino IDE  
Select your ESP8266 board in Boards Manager  
Compile and upload to your board  
Use a TTL to RS232 converter to connect the ESP8266 to your terminal  
Set terminal to 19200 baud  
Boot the ESP8266  

hiterm> wifi:<SSID>:<PASSWORD>  

Reset ESP8266  

hiterm> tcp:telehack.com:23  
.rain  

Watch it rain.  

^c  
.exit  
