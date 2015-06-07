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

app.get('/weather/:zip', function(req, res) {
	var zip = req.params.zip;
	res.set('Content-Type', 'application/json');
	http.get(
		"http://api.openweathermap.org/data/2.5/weather?q="+zip,
		function(weather_res) {
			/* send all weather data to our client */
			weather_res.pipe(res);
		}
	);

});

var server = app.listen(3000);
