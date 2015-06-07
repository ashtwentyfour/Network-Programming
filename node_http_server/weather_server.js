/*
 * weather_server.js
 * 
 * Serve static pages from the current directory.
 * For GET requests to /weather?q=..., forward to api.openweathermap.org.
 */

var express = require('express');
var http = require('http');

var app = express( );
app.use(express.static('.'));
var myCache = {}

app.get('/weather/:zip', function(req, res) {
	var zip = req.params.zip;
	res.set('Content-Type', 'application/json');

        /* if the cache does not have the zip code (cache miss) */

        if(!myCache[zip]) {
	  http.get("http://api.openweathermap.org/data/2.5/weather?q="+zip,
	       function(weather_res) {   // request to OpenWeatherMap 

	       var DataString = "";   // string used to accumulate weather data 
               weather_res.on('data' , function(data) {   // receive data and append it to the data string

		  DataString = DataString + data;
	       
               });
		   
               weather_res.on('end' , function() {  // after all the data has been received

                  myCache[zip] = DataString;      
                  console.log("data added to cache for zip code: " + zip);
			   
               });  
	       
               weather_res.pipe(res);  // send the data back to the client
		     
	    });
	  
	}

        /* for a cache hit return the cached data */

        else {  
          
	    res.send(myCache[zip]);
            console.log("cache data sent for zip code: " + zip);

	} 

        /* function which deletes data for a zip code every 1/2 an hour */

	setTimeout(function() {  

	    if(myCache[zip]) {
                 
                 delete myCache[zip];
		 console.log("deleted cache data for zip "+ zip +  
                     " due to timeout of 30 minutes");

            }
	   } , 1800000)  // timer set to 30 minutes in milliseconds

});



/* request to return the cache in JSON format */

app.get('/cache' , function(req, res) {   
    
    res.send(myCache);
    console.log("current cache sent");

});
    

/* request to delete the cache data for the specified code */

app.delete('/weather/:zip' , function(req , res) {
     
    var zip = req.params.zip;
    if(myCache[zip]) {

      delete myCache[zip];
      console.log("deleted cache data for zip code: " + zip);

    }
    
    else {  // if the zip code does not exist in the cache
      
      console.log("zip code not found - data not deleted");
    
    }

});



var server = app.listen(3000);
