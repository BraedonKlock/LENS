const path = require('path'); // importing path for linux OS path compatability
const express = require('express'); // importing express
const loggedinController = require('../controllers/loggedin');

const router = express.Router(); // creating a mini express router

router.get("/incidents", loggedinController.getIncidents);

module.exports = router;
