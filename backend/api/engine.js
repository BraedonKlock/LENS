const express          = require('express');
const engineAuth       = require('../middleware/engineAuth');
const engineController = require('../controllers/engineController');

const router = express.Router();

router.use(engineAuth);

router.post('/incidents/request-upload', engineController.requestUpload);
router.post('/incidents',                engineController.createIncident);

module.exports = router;
