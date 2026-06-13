const { S3Client, GetObjectCommand } = require('@aws-sdk/client-s3');
const { getSignedUrl }               = require('@aws-sdk/s3-request-presigner');
const pool                           = require('../db/connection');

const r2 = new S3Client({
  region:   'auto',
  endpoint: `https://${process.env.R2_ACCOUNT_ID}.r2.cloudflarestorage.com`,
  credentials: {
    accessKeyId:     process.env.R2_ACCESS_KEY_ID,
    secretAccessKey: process.env.R2_SECRET_ACCESS_KEY,
  },
});

exports.getIncidents = async (req, res) => {
  const { storeId } = req.user;

  try {
    const [rows] = await pool.query(
      `SELECT id, camera_id, timestamp, incident_type, reviewed, created_at
       FROM incidents
       WHERE store_id = ?
       ORDER BY timestamp DESC`,
      [storeId]
    );
    res.json(rows);
  } catch (err) {
    console.error('Get incidents error:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
};

exports.getClipUrl = async (req, res) => {
  const { storeId } = req.user;
  const { id }      = req.params;

  try {
    const [rows] = await pool.query(
      'SELECT id, clip_path FROM incidents WHERE id = ? AND store_id = ?',
      [id, storeId]
    );

    if (rows.length === 0) {
      return res.status(404).json({ error: 'Incident not found' });
    }

    const url = await getSignedUrl(
      r2,
      new GetObjectCommand({
        Bucket: process.env.R2_BUCKET_NAME,
        Key:    rows[0].clip_path,
      }),
      { expiresIn: parseInt(process.env.R2_CLIP_URL_TTL) || 300 }
    );

    res.json({ url });
  } catch (err) {
    console.error('Get clip URL error:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
};

exports.markReviewed = async (req, res) => {
  const { storeId } = req.user;
  const { id }      = req.params;

  try {
    const [result] = await pool.query(
      'UPDATE incidents SET reviewed = TRUE WHERE id = ? AND store_id = ?',
      [id, storeId]
    );

    if (result.affectedRows === 0) {
      return res.status(404).json({ error: 'Incident not found' });
    }

    res.json({ success: true });
  } catch (err) {
    console.error('Mark reviewed error:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
};
