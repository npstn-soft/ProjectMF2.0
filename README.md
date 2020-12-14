# ProjectMF 2.0

## About

ProjectMF 2.0 adds MF Signaling capabilities into Asterisk for professional or hobbyist use.

PMF2 is capable of:

Local Sender\
Local Register\
Custom Sender\
Custom Register\
Forward Audio Mute

The MF Detection that is apart of PMF2 has been know to be very reliable and since this solely uses the Asterisk Dialplan you do not need to recompile and mess with Asterisk to get this to work. All you need is the included mf.conf, confbridge.conf, examples & binaries in the right place! (Or you can compile it yourself from source to add more functionality!)

## Getting Started

First step move mf.conf & confbridge.conf in /etc/asterisk.

Move "detect" into /etc/asterisk as well. Run the command "make" in the directory to compile it and then run the following to set the correct permissions.

```bash
chmod a+x mf
chmod a+x mf2
```

Next you will need to add the following to the top of your extensions.conf.

```
#include mf.conf
```

Bam, You are done!


## Syntax

## Gosub(mfmain,s,1(0,0,0,5551212))
###### Register_OPT=Local, Sender_OPT=Local, FAM=Enable, SenderDigits= KP5551212ST


The first argument tells the system if you want to use a local MF register or a farend/remote register. The value 0 will force a local MF receiver. Any other value will pull whatever is in register_map with that value.

The second argument tells the system if you want to use a Local MF Digit Sender or if you have a piece of equipment that will send MF forward. The value 0 will bring in a local MF sender with the value of the last argument (Sender Digits)

The third argument tells the system if you want to Enable or Disable "Forward Audio Mute" AKA "FAM" this will prevent any audio from the caller/equipment going forward until MF signaling is done. If you are using a Custom Sender you must disable Forward Audio Mute to allow audio from you Sender in the forward direction. Otherwise it is recommended you leave this value enabled so noise on the line will not adversely effect signaling.

The fourth argument is the Sender Digits and it is the digits send in MF (Multi-Frequency) in the forward direction and this value will only be used if the second argument is set to use the local sender. 



## Examples

### Local MF Loopback
If you just want to have the fun of hearing MF tones when placing calls on your local asterisk system Not much is required at all besides the above basic install and the below sample config.


Extensions.conf

```
#include mf.conf

[main] ; By default this is the context used to deliver calls into the dialplan from PMF. This can be changed in mf.conf or changed to a variable.
exten => 3551000,1,Progress
same => n,PlayTones(1004)
same => n,Wait(5)
same => n,Hangup

exten => 3551001,1,Answer
same => n,Playback(hello-world)
same => n,Hangup


[office] ; SIP Phones

;Make all 355-XXXX calls go over a loopback MF trunk using
;Asterisk Local Register, Asterisk Local Sender &
;Forward Audio Mute Enable.

exten => _355XXXX,1,Gosub(mfmain,s,1(0,0,0,${EXTEN}))
same => n,Hangup
```

### Define Remote MF Receivers 

In mf.conf find register_map and add the entrypoint into the devices that are expecting MF. (*Note the device must answer immediately and must not be expecting some type of start signaling, If some other type of signal needs to be sent that must be added in the register_map section.

```
[register_map]
exten => 22,1,Dial(DAHDI/g5) ; #5 Crossbar MF Receiver
same => n,Hangup

exten => 23,1,Dial(DAHDI/g3) ; Cognitronics 688 ANA
same => n,Hangup

exten => 24,1,Dial(IAX2/npstn@scpa01.ddns.net/5312600) ; ProjectMF2 on an NPSTN Members machine expecting 7 digits 531-XXXX or 826-XXXX.
same => n,Hangup

```

Sample Asterisk config for remote receivers:

```
#include mf.conf


[office] ; SIP Phones

exten => _531XXXX,1,Gosub(mfmain,s,1(24,0,0,${EXTEN})) ; MF Trunk into Terras Switch on NPSTN & sending the 7 digit called number.
same => n,Hangup

exten => _826XXXX,1,Gosub(mfmain,s,1(24,0,0,${EXTEN})) ; MF Trunk into Terras Switch on NPSTN & sending the 7 digit called number.
same => n,Hangup

exten => _232XXXX,1,Gosub(mfmain,s,1(22,0,0,${EXTEN})) ; MF Trunk into a #5 Crossbar.
same => n,Hangup

exten => 311,1,Gosub(mfmain,s,1(23,0,0,${CALLERID(num)})) ; MF Trunk into an ANAC Device.
same => n,Hangup
```
 
If you need any help with config you may contact Dylan Cruz at admin@dc4.us



## Contributing
Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

Please make sure to update tests as appropriate.

Also make sure to check out NPSTN at https://npstn.us & Donations are also accepted via a PayPal link at that address! :)
## License
[GNU GPL 3.0](https://www.gnu.org/licenses/gpl-3.0.txt)
