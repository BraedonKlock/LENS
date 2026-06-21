const https   = require('https');
const crypto  = require('crypto');
const { S3Client, PutObjectCommand } = require('@aws-sdk/client-s3');
const { getSignedUrl }               = require('@aws-sdk/s3-request-presigner');
const pool = require('../db/connection');

const r2 = new S3Client({
  region:   'auto',
  endpoint: `https://${process.env.R2_ACCOUNT_ID}.r2.cloudflarestorage.com`,
  credentials: {
    accessKeyId:     process.env.R2_ACCESS_KEY_ID,
    secretAccessKey: process.env.R2_SECRET_ACCESS_KEY,
  },
});

async function sendPushNotifications(tokens, title, body, data = {}) {
  const valid = tokens.filter(t => t.startsWith('ExponentPushToken'));
  if (valid.length === 0) return;

  const messages = valid.map(token => ({
    to: token, title, body, data,
    sound: 'default', priority: 'high',
  }));
  const payload = JSON.stringify(messages);

  return new Promise((resolve) => {
    const req = https.request({
      hostname: 'exp.host',
      path:     '/--/api/v2/push/send',
      method:   'POST',
      headers: {
        'Content-Type':   'application/json',
        'Accept':         'application/json',
        'Content-Length': Buffer.byteLength(payload),
      },
    }, (res) => { res.resume(); res.on('end', resolve); });
    req.on('error', resolve);
    req.write(payload);
    req.end();
  });
}

exports.requestUpload = async (req, res) => {
  const { storeId }          = req;
  const { cameraId, timestamp } = req.body;

  if (!cameraId || !timestamp) {
    return res.status(400).json({ error: 'cameraId and timestamp are required' });
  }

  try {
    const clipPath = `clips/${storeId}/${timestamp}_cam${cameraId}.mp4`;

    const uploadUrl = await getSignedUrl(
      r2,
      new PutObjectCommand({
        Bucket:      process.env.R2_BUCKET_NAME,
        Key:         clipPath,
        ContentType: 'video/mp4',
      }),
      { expiresIn: 300 }
    );

    res.json({ uploadUrl, clipPath });
  } catch (err) {
    console.error('Request upload error:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
};

function parseCameraTimestamp(ts) {
  // Engine sends compact ISO8601: 20260621T171013Z → 2026-06-21 17:10:13
  const m = ts.match(/^(\d{4})(\d{2})(\d{2})T(\d{2})(\d{2})(\d{2})Z$/);
  if (m) return `${m[1]}-${m[2]}-${m[3]} ${m[4]}:${m[5]}:${m[6]}`;
  return ts;
}

exports.createIncident = async (req, res) => {
  const { storeId }                                      = req;
  const { cameraId, timestamp, clipPath, incidentType } = req.body;

  if (!cameraId || !timestamp || !clipPath) {
    return res.status(400).json({ error: 'cameraId, timestamp, and clipPath are required' });
  }

  try {
    const incidentId  = crypto.randomUUID();
    const mysqlTs     = parseCameraTimestamp(timestamp);

    await pool.query(
      `INSERT INTO incidents (id, store_id, camera_id, timestamp, clip_path, incident_type)
       VALUES (?, ?, ?, ?, ?, ?)`,
      [incidentId, storeId, cameraId, mysqlTs, clipPath, incidentType || null]
    );

    // Push notifications — fire-and-forget, never fail the response
    pool.query(
      `SELECT pt.token FROM push_tokens pt
       JOIN users u ON u.id = pt.user_id
       WHERE u.store_id = ?`,
      [storeId]
    ).then(([rows]) => {
      if (rows.length === 0) return;
      return sendPushNotifications(
        rows.map(r => r.token),
        'Concealment Detected',
        `Camera ${cameraId} flagged suspicious activity.`,
        { incidentId }
      );
    }).catch(err => console.error('Push notification error:', err));

    res.status(201).json({ success: true, id: incidentId });
  } catch (err) {
    console.error('Create incident error:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
};
