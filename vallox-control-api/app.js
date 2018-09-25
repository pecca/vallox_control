var express = require('express');
var path = require('path');
var cookieParser = require('cookie-parser');
var bodyParser = require('body-parser');

var vallox_control = require('./routes/vallox_control');

var app = express();
var port = process.env.PORT || 9000;

app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: false }));
app.use(cookieParser())

app.use('/api/vallox', vallox_control);
app.listen(port);

module.exports = app;
