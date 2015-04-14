/*
 * Test client for Network Programming Project 2
 *
 * Andrew Armenia
 * 2015-03-26
 */

var dgram = require('dgram');

// process command line arguments
var proxy = process.argv[2];
var myName = process.argv[3];
var theirName = process.argv[4];

if (!proxy || !myName) {
	console.log("usage: node p2_sample_client.js proxy name [dest]");
	process.exit(1);
}

var callSetupSocket = dgram.createSocket('udp6');

// send initial registration, then one every 10 seconds
sendRegister();
setInterval(sendRegister, 10000);

// if a user to call was given, try to initiate a call
if (theirName) {
	message = new Buffer("CALL FROM:"+myName+" TO:"+theirName);
	callSetupSocket.sendto(
		message, 0, message.length,
		34567, proxy
	);
}

// handle incoming packets
callSetupSocket.on('message', function(msg, rinfo) {
	var fields = msg.toString().split(" ");

	switch (fields[0]) {
		case "ACK_REGISTER":
			console.log("Received ACK_REGISTER");
			break;

		case "CALL":
			handleCall(msg);
			break;

		case "ACK_CALL":
			console.log("Call accepted!");
			break;

		case "CALL_FAILED":
			console.log("Call failed!");
			break;

		case "MEDIA_PORT":
			handleMedia(msg);
			break;

		default:
			console.log("Unknown packet received!");
                        console.log(msg.toString());
			break;
	}
});

/*
 * handleCall(msg)
 *
 * Acknowledge the call (or hang up on it)
 */
function handleCall(msg) {
	var str = msg.toString();
	var pattern = /^CALL FROM:([A-Za-z0-9]+) TO:([A-Za-z0-9]+)/;
	var match = pattern.exec(str);

	if (match) {
		var response;

		// only accept calls that are for us
		if (match[2] != myName) {
			console.log("we got a call for someone else!");
                        //console.log(msg);
		} else {
			// decide whether the call should succeed or fail
			if (Math.random( ) < 0.8) {
				response = "ACK_CALL ";
			} else {
				response = "CALL_FAILED ";
			}
			
			// construct and send the response
			response += "FROM:"+myName;
			response += " TO:"+match[1];

			var message = new Buffer(response);
			callSetupSocket.send(
				message, 0, message.length,
				34567, proxy
			);
		}
	} else {
		console.log("invalid CALL packet received!");
	}
}

/*
 * handleMedia(msg)
 *
 * Create a new socket to start sending and receiving media 
 * on the appropriate port.
 */
function handleMedia(msg) {
	var str = msg.toString();
	var pattern = /^MEDIA_PORT FROM:([A-Za-z0-9]+) TO:([A-Za-z0-9]+) (\d+)/;
	var match = pattern.exec(str);

	if (match) {
		var peerName = match[1];
		var port = parseInt(match[3]);
		if (match[2] == myName) {
			createMediaSocket(peerName, port);
                        console.log("from: ")
			console.log(peerName);
                        console.log("to: ");
                        console.log(myName);
		} else {
			console.log("got a MEDIA_PORT meant for someone else!");
		}
	} else {
		console.log("invalid MEDIA_PORT message");
	}
}

/*
 * createMediaSocket(port)
 *
 * Send and receive "streaming media" (dummy UDP packets) via the given
 * port on the server side.
 */
function createMediaSocket(peerName, port) {
	var mediaSocket = dgram.createSocket('udp6');

	/* send one packet per second */
	setInterval(function() {
		var message = new Buffer("Hi there, this is " + myName);
		mediaSocket.send(
			message, 0, message.length,
			port, proxy
		);
	}, 1000);

	/* print a message when a packet is received */
	mediaSocket.on('message', function(msg, rinfo) {
		console.log("media from " + peerName + ": "
			+ msg.toString());
	});
}

/*
 * sendRegister()
 * 
 * send a registration packet to the proxy
 */
function sendRegister() {
	var message = new Buffer("REGISTER " + myName);
	callSetupSocket.sendto(
		message, 0, message.length,
		34567, proxy
	);
}
