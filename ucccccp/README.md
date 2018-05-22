# ucccccp
Universal Cool Cheater Crypto Chit-Chat Protocol (meme)

This repo includes example code so you can include the lib to your cheat easily! **Make sure to look at example.cpp!**

```
 *  Usage:
 *  	To send message, call ucccccp::encrypt on text of the message and send the result to chat
 *  	When receiving a message, call ucccccp::validate, and if it succeeds, call ucccccp::decrypt and show the result
 *
 *		To include this to your project, add ucccccp.hpp and base64.h to your source and #include "ucccccp.hpp"
 *
 *		Feel free to suggest improvements! Use the github repo (https://github.com/nullifiedcat/ucccccp) to submit issues.
 *
 *  General structure of message:
 *  	!![version][data]
 *  		version: char from base64 alphabet
 *  		data: depends on version
 *
 * 	Versions:
 * 	A:
 * 		!!A[data][checksum]
 * 			data = base64(crappy_xorstring(input))
 *			checksum = crappy_checksum(input)
```