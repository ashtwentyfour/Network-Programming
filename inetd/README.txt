to complie program:
  
g++ *.cpp -o run.exe -Wall

to run:

./run.exe input.txt

- the program takes in one arguement (file with commands)
- open a new ternimal window or windows
- telnet to a port listed in the input.txt file 
   eg: telnet your_IP port_number (telnet 192.168.1.25 12345 or telnet 192.168.1.25 12346)
- the output should be displayed (based on the respective command for that port)  
- the program assumes that the ports are listed in ascending order: first_port, first_port + 1, first_port + 2
  ....... first_port + n












