to compile:

g++ *.cpp -o run.exe -Wall -lssl -lcrypto -Wno-deprecated

to run:

for https:

./run.exe https://www.google.com/

for http:

./run.exe http://www.asquaredlabs.com/csci4220/


- the function which first parses the url has been modified for https arguments
- the program splits the input arguement into a host, path and port number (if any)
- The program implements the GET method of HTTP/1.1
- If a port number cannot be extracted from the input arguement port 80 is used by default for http and 403 for https
- the program assumes that the user will enter an input arguement that ends with a slash ('/')
- the program exits with an error if the header received does not contain "Content-Length: x"
- the program does not search for content length in the header for https since the sent request uses Connection: Close
- the program declares a character buffer of size = x+1 to receive the page's content in parts (for http)
- the program uses a polymorphic connection class which has 2 derived classes one for openssl and one for http 
- the respective objects of the aforementioned classes are created for openssl and http connections 
- each derived classes overrides the 'read_write()' function which serves as a wrapper around the specific receiving and writing 
  functionalities of the two connection types (virtual functions have been used)









