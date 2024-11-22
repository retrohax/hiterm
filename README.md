# HITERM

vt100 terminal emulator for "dumb" terminals such as the Lear Siegler ADM-3A.  

Terminals that can at least clear the screen and position the cursor can actually perform pretty well, especially at 19200 baud.  

HITERM connects to hosts using the telnet protocol and then translates vt100 commands into something the terminal can work with.  

Some examples:  
1) Play Dope Wars on sdf.org.  
2) Play Inform/z-machine text adventure games.  
3) Connect to IRC using irssi.  
4) Any other ncurses app.  

Runs on ESP8266 devices such as NodeMCU which allows you to connect the terminal to wifi.  

Currently supports the Lear Siegler ADM-3A.  
I'd like to add the Hazeltine 1500 but I don't currently have one.  
