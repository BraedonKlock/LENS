const express = require("express");
const http = requires("http");

const notLoggedInRoute = require("./api/notLoggedIn");
const loggedInRoute = require("./api/loggedIn");

const app = express();
const server = http.createServer(app);

const PORT = process.env.PORT() || 3000;

server.listen(PORT);
