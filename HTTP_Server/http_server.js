// using the http and request modules
var http = require('http');
var request = require('request');
var stream = require('stream');

// port
const PORT = 3000; 


// request handler
function handleRequest(req, res) {
   
    /* URL = my_server -> http://localhost:3000 + /foo/bar, 
    for example URL = http://www.cs.rpi.edu + 
    //~moorthy/Courses/CSCI2300/  */

    var url = process.argv[2] + req.url;

    request.get(url).on('response' , function(response) {   // request

       // if the content-type is text/html 
       if((response.headers['content-type']).indexOf("text/html") > -1) {  
	 response.setEncoding("utf8");
         var str = ""
         response.pipe(filter(function(data) {
	     return capitalize(data);       // response stream modifying function 
	 })).pipe(res);
    
       }

       // otherwise pipe directly to client
       else response.pipe(res);   

    });

    
}



// filter function - all caps
function capitalize(data) {
    return data.toUpperCase();
}



// stream filter
function filter(callback) {

    var xfrmStream = new stream.Transform({decodeStrings: false});
    xfrmStream._transform = function(chunk, encoding, doneCallback) {
      xfrmStream.push(callback(chunk));
      doneCallback();
    }

    return xfrmStream;
}



// create a server
var server = http.createServer(handleRequest);



// server
server.listen(PORT);