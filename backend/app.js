require('dotenv').config();

const express    = require('express');
const http       = require('http');
const cors       = require('cors');

const authRoutes     = require('./api/auth');
const loggedInRoutes = require('./api/loggedIn');
const engineRoutes   = require('./api/engine');

const app    = express();
const server = http.createServer(app);

app.use(cors());
app.use(express.json());

app.use('/auth',   authRoutes);
app.use('/engine', engineRoutes);
app.use('/',       loggedInRoutes);

const PORT = process.env.PORT || 3000;
server.listen(PORT, () => {
  console.log(`LENS backend running on port ${PORT}`);
});
