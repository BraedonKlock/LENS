const pool = require('../db/connection');

exports.createIncident = async (req, res) => {
  const { storeId }                            = req;
  const { camera_id, timestamp, clip_path, incident_type } = req.body;

  if (!camera_id || !timestamp || !clip_path) {
    return res.status(400).json({ error: 'camera_id, timestamp, and clip_path are required' });
  }

  try {
    const [result] = await pool.query(
      `INSERT INTO incidents (id, store_id, camera_id, timestamp, clip_path, incident_type)
       VALUES (UUID(), ?, ?, ?, ?, ?)`,
      [storeId, camera_id, timestamp, clip_path, incident_type || null]
    );

    res.status(201).json({ success: true });
  } catch (err) {
    console.error('Create incident error:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
};
