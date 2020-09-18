# ProjectMF 2.0 (AstMF Rel)
```
Included is a pre-compiled version of the DSP needed for x86 and ARM. If you would like to modify it we 
have included the main source (See detect_source)

Installation: 

Include the "detect" folder in /etc/asterisk

Make sure all the files ("mf", "mf2") have the correct permissions:

chmod a+x mf
chmod a+x mf2

Open your extensions.conf and add the line:

#include mf.conf

Copy the "mf_user" "mf_bridge" contexts from the included confbridge.conf example into your 
confbridge.conf or use our copy verbatim.

You are all set! (For correct supervision see below)

AST_Sender = Asterisk Sender (This is the built-in sender using the [mfer] subroutine developed by 
Naveen Albert & Brian Clancy)

Cust_Sender = Enable this  if you want to use your own MF sender.

FAM_Enable = Forward Audio Mute (This will mute the audio going in the forward direction and is to 
be used with AST_Sender for better reliability if you have F
AM_ENABLE turned on using a Custom Sender the detector will not be able to hear the MF)

FAM_Disable = This disables the Forward Audio Mute function.

First Argument: 0-AST_Sender, 1-Cust_Sender
Second Argument: 0-FAM_Enable, 1-FAM_Disable
Third Argument: Number to be out pulsed (If using Asterisk Sender)

Gosub(mfmain,s,1(0,0,5551212)) ; AST_Sender, FAM_Enable, KP 555-1212 ST (loopback into [Main]
Gosub(mfmain,s,1(0,1,5551212)) ; AST_Sender, FAM_Disable, KP 555-1212 ST (loopback into [Main]

Gosub(mfmain,s,1(1,1)) ; Cust_Sender, FAM_Disable, Expecting MF String from Custom Sender

Calls are delivered into the [Main] context.

You will need to Patch your Asterisk system to allow answer supervision to pass through the MF trunk. 
Please look at the NPSTN Docs for information on how to do this. The related article is included below:

https://npstn.us/docs/

Non-Supervising Conference Bridges
Discovery & Implementation Credit: Dylan Cruz, 04-2020
Article was written by Naveen Albert for NPSTN

By default, the ConfBridge() application will supervise with no alternative, unlike the Playback() 
application which accepts a noanswer argument. 
Because ConfBridge is often the only way to get much functionality in Asterisk to work as desired, 
this is a significant limitation; 
however, modifying the source code can remove this behavior for better operation.

Patch Instructions:

Navigate to the location where Asterisk was compiled, e.g. /usr/src/asterisk-13.whatever 
— go to the apps directory.
Open app_confbridge.c for editing
Perform a find and replace operation, replacing ast_answer(chan); with //ast_answer(chan);.
Navigate to the root source folder, e.g. /usr/src/asterisk-13.whatever
Type make and then make install to recompile Asterisk.
Type service asterisk stop and then service asterisk start to restart Asterisk.
ConfBridge() will no longer automatically supervise a call. 
Once you've patched your Asterisk system, you must manually perform supervisory functions, as follows:

This will replicate the previous behavior, by manually answering and supervising the call.
exten => s,1,Answer()
    same => n,ConfBridge(mybridge)
    same => n,Hangup()
                
This will allow two-way audio without answering. Progress allows passing of audio without supervising. 
The main use case here is for backend mixing of audio channels, such as for intercept, signaling,
or operator services.

exten => s,1,Progress()
    same => n,ConfBridge(mybridge)
    same => n,Hangup()
                
This will not perform any supervisory functions at all! The caller may/will not hear anything! 
For most cases, you should not use this at all:
exten => s,1,ConfBridge(mybridge)
    same => n,Hangup()
                
You must either use Progress() or Answer(). Failing to do so may result in no audio being passed at all.

Take care to revisit all uses of ConfBridge in your dialplan code to ensure you're using 
Progress or Answer before any calls to ConfBridge.
```
