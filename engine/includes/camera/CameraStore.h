#ifndef CAMERASTORE_H
#define CAMERASTORE_H

#include <string>
#include <vector>

#include "CameraConfig.h"

class CameraStore
{
public:
	explicit CameraStore(const std::string& dbPath);
	~CameraStore();

	bool open();

	int  save(const CameraConfig& config);
	void remove(int id);
	std::vector<CameraConfig> loadAll() const;

	std::string getConfig(const std::string& key) const;
	void        setConfig(const std::string& key, const std::string& value);

private:
	std::string    m_dbPath;
	struct sqlite3* m_db = nullptr;

	bool createTable();
	bool createConfigTable();
};
#endif
