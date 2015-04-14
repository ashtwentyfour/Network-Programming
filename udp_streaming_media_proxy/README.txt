To compile:

g++ *.cpp -o run.exe -Wall


To run the server:

./run.exe


basic steps to initiate a call between 2 clients:

- open new windows
- to run client 1: node p2_sample_client.js localhost Ashwin
- to run client 2 (caller): node p2_sample_client.js localhost menona2 Ashwin

- the program sends call failed messages if the client who is being 
  contacted does not exist or if the client rejects the call
- if the call is ackonwledged, the program generates a new socket and sends
  the media port info to each client
- the new thread receives messages on this socket and forwards it to the peer
- the server prints out the registration and acknowledgement messages it receives








