To complie:

g++ *.cpp -o run.exe -Wall

To Run:

./run.exe http://www.asquaredlabs.com:80/csci4220/ (or ant input link)


- The program splits the input arguement into a host, path and port number (if any) 
- The program implements the GET method of HTTP/1.1
- If a port number cannot be extracted from the input arguement port 80 is used by default
- The program assumes that the user will enter an input arguement that ends with a slash ('/')
- The program exits with an error if the header received does not contain "Content-Length: x"
- The program declares a character buffer of size = x+1 to receive the page's content in parts






