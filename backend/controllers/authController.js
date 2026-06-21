const bcrypt = require('bcryptjs');
const jwt    = require('jsonwebtoken');
const crypto = require('crypto');
const pool   = require('../db/connection');

exports.savePushToken = async (req, res) => {
  const { userId } = req.user;
  const { token }  = req.body;

  if (!token) return res.status(400).json({ error: 'token required' });

  try {
    await pool.query(
      `INSERT INTO push_tokens (id, user_id, token) VALUES (?, ?, ?)
       ON DUPLICATE KEY UPDATE user_id = VALUES(user_id)`,
      [crypto.randomUUID(), userId, token]
    );
    res.json({ success: true });
  } catch (err) {
    console.error('Save push token error:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
};

exports.login = async (req, res) => {
  const { email, password } = req.body;

  if (!email || !password) {
    return res.status(400).json({ error: 'Email and password are required' });
  }

  try {
    const [rows] = await pool.query(
      `SELECT u.id, u.email, u.password_hash, u.store_id,
              s.store_name, s.store_number
       FROM users u
       JOIN stores s ON s.id = u.store_id
       WHERE u.email = ?`,
      [email]
    );

    if (rows.length === 0) {
      return res.status(401).json({ error: 'Invalid email or password' });
    }

    const user = rows[0];
    const passwordMatch = await bcrypt.compare(password, user.password_hash);
    if (!passwordMatch) {
      return res.status(401).json({ error: 'Invalid email or password' });
    }

    const token = jwt.sign(
      { userId: user.id, storeId: user.store_id },
      process.env.JWT_SECRET,
      { expiresIn: process.env.JWT_EXPIRES_IN || '7d' }
    );

    res.json({
      token,
      user: {
        id:          user.id,
        email:       user.email,
        storeId:     user.store_id,
        storeName:   user.store_name,
        storeNumber: user.store_number,
      },
    });
  } catch (err) {
    console.error('Login error:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
};

exports.register = async (req, res) => {
  const { server_id, email, password, store_name, store_number } = req.body;

  if (!server_id || !email || !password || !store_name || !store_number) {
    return res.status(400).json({ error: 'All fields are required' });
  }

  const conn = await pool.getConnection();
  try {
    await conn.beginTransaction();

    const [servers] = await conn.query(
      'SELECT id, in_use FROM servers WHERE id = ?',
      [server_id]
    );
    if (servers.length === 0) {
      await conn.rollback();
      return res.status(404).json({ error: 'Server ID not found' });
    }
    if (servers[0].in_use) {
      await conn.rollback();
      return res.status(409).json({ error: 'Server ID is already registered to an account' });
    }

    const [existing] = await conn.query(
      'SELECT id FROM users WHERE email = ?',
      [email]
    );
    if (existing.length > 0) {
      await conn.rollback();
      return res.status(409).json({ error: 'Email is already registered' });
    }

    const storeId      = crypto.randomUUID();
    const userId       = crypto.randomUUID();
    const engineApiKey = crypto.randomBytes(32).toString('hex');
    const passwordHash = await bcrypt.hash(password, 12);

    await conn.query(
      'INSERT INTO stores (id, store_name, store_number, api_key) VALUES (?, ?, ?, ?)',
      [storeId, store_name, store_number, engineApiKey]
    );

    await conn.query(
      'INSERT INTO users (id, email, password_hash, store_id) VALUES (?, ?, ?, ?)',
      [userId, email, passwordHash, storeId]
    );

    await conn.query(
      'UPDATE servers SET in_use = TRUE, store_id = ? WHERE id = ?',
      [storeId, server_id]
    );

    await conn.commit();

    const token = jwt.sign(
      { userId, storeId },
      process.env.JWT_SECRET,
      { expiresIn: process.env.JWT_EXPIRES_IN || '7d' }
    );

    res.status(201).json({
      token,
      engine_api_key: engineApiKey,
      user: {
        id:          userId,
        email,
        storeId,
        storeName:   store_name,
        storeNumber: store_number,
      },
    });
  } catch (err) {
    await conn.rollback();
    console.error('Register error:', err);
    res.status(500).json({ error: 'Internal server error' });
  } finally {
    conn.release();
  }
};
