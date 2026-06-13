const pool = require('../db/connection');

module.exports = async function engineAuth(req, res, next) {
  const apiKey = req.headers['x-api-key'];
  if (!apiKey) {
    return res.status(401).json({ error: 'Missing X-API-Key header' });
  }

  try {
    const [rows] = await pool.query(
      'SELECT id FROM stores WHERE api_key = ?',
      [apiKey]
    );
    if (rows.length === 0) {
      return res.status(401).json({ error: 'Invalid API key' });
    }
    req.storeId = rows[0].id;
    next();
  } catch (err) {
    console.error('Engine auth error:', err);
    return res.status(500).json({ error: 'Internal server error' });
  }
};
