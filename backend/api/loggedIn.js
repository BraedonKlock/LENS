const express             = require('express');
const auth                = require('../middleware/auth');
const incidentController  = require('../controllers/incidentController');

const router = express.Router();

router.use(auth);

router.get('/incidents',              incidentController.getIncidents);
router.get('/incidents/:id/clip',     incidentController.getClipUrl);
router.patch('/incidents/:id/review', incidentController.markReviewed);

module.exports = router;
