# HITERM

HITERM is a telnet client for your Lear Siegler ADM-3A "dumb" terminal.  

It runs on ESP8266 devices (NodeMCU, etc.) to connect the terminal to wifi.  

Once connected to a host, HITERM converts DEC VT100 commands to primitive commands understood by the terminal.  

This allows your terminal to run a lot of programs intended to run on a VT100.  

Some examples:  
1) Dope Wars  
2) Inform/z-machine text adventure games  
3) irssi (IRC client)  
4) other ncurses programs  

Currently supports the Lear Siegler ADM-3A.  

TODO:  
Hazeltine 1500  


### QUICK START

Load hiterm.ino in the Arduino IDE  
Select your ESP8266 board in Boards Manager  
Compile and upload to your board  
Use a TTL to RS232 converter to connect the ESP8266 to your terminal  
Set terminal to 19200 baud  
Boot the ESP8266  

```
hiterm> wifi:SSID:PASSWORD  
```

Reset ESP8266  

```
hiterm> tcp:telehack.com:23
.rain

^c
.exit
```

