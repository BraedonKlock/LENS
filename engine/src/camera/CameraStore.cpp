#include "camera/CameraStore.h"

#include <sqlite3.h>
#include <filesystem>

CameraStore::CameraStore(const std::string& dbPath) : m_dbPath(dbPath) {}

CameraStore::~CameraStore()
{
	if (m_db) sqlite3_close(m_db);
}

bool CameraStore::open()
{
	std::filesystem::create_directories(std::filesystem::path(m_dbPath).parent_path());

	if (sqlite3_open(m_dbPath.c_str(), &m_db) != SQLITE_OK)
		return false;

	return createTable();
}

bool CameraStore::createTable()
{
	const char* sql =
		"CREATE TABLE IF NOT EXISTS cameras ("
		"  id       INTEGER PRIMARY KEY AUTOINCREMENT,"
		"  name     TEXT NOT NULL,"
		"  url      TEXT NOT NULL,"
		"  password TEXT,"
		"  location TEXT"
		");";

	return sqlite3_exec(m_db, sql, nullptr, nullptr, nullptr) == SQLITE_OK;
}

int CameraStore::save(const CameraConfig& config)
{
	const char* sql =
		"INSERT INTO cameras (name, url, password, location) VALUES (?, ?, ?, ?);";

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
		return -1;

	sqlite3_bind_text(stmt, 1, config.name.c_str(),     -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, config.url.c_str(),      -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 3, config.password.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 4, config.location.c_str(), -1, SQLITE_STATIC);

	const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
	sqlite3_finalize(stmt);

	return ok ? static_cast<int>(sqlite3_last_insert_rowid(m_db)) : -1;
}

void CameraStore::remove(int id)
{
	const char* sql = "DELETE FROM cameras WHERE id = ?;";

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
		return;

	sqlite3_bind_int(stmt, 1, id);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
}

std::vector<CameraConfig> CameraStore::loadAll() const
{
	std::vector<CameraConfig> configs;

	const char* sql =
		"SELECT id, name, url, password, location FROM cameras ORDER BY id;";

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
		return configs;

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		CameraConfig cfg;
		cfg.id       = sqlite3_column_int(stmt, 0);
		cfg.name     = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
		cfg.url      = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
		cfg.password = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
		cfg.location = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
		configs.push_back(cfg);
	}

	sqlite3_finalize(stmt);
	return configs;
}
