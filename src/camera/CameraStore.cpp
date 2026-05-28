#include "camera/CameraStore.h"

#include <QDir>
#include <QSqlDatabase>
#include <QSqlQuery>

static const QString DB_CONNECTION = "lens_db";

CameraStore::CameraStore(const QString& dataDir) : m_dataDir(dataDir) {}

bool CameraStore::open()
{
    QDir().mkpath(m_dataDir);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", DB_CONNECTION);
    db.setDatabaseName(m_dataDir + "/lens.db");

    if (!db.open()) return false;

    return createTable();
}

bool CameraStore::createTable()
{
    QSqlQuery q(QSqlDatabase::database(DB_CONNECTION));
    return q.exec(
        "CREATE TABLE IF NOT EXISTS cameras ("
        "  id       INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name     TEXT NOT NULL,"
        "  url      TEXT NOT NULL,"
        "  password TEXT,"
        "  location TEXT"
        ")"
    );
}

int CameraStore::save(const CameraConfig& config)
{
    QSqlQuery q(QSqlDatabase::database(DB_CONNECTION));
    q.prepare(
        "INSERT INTO cameras (name, url, password, location) "
        "VALUES (:name, :url, :password, :location)"
    );
    q.bindValue(":name",     config.name);
    q.bindValue(":url",      config.url);
    q.bindValue(":password", config.password);
    q.bindValue(":location", config.location);

    if (!q.exec()) return -1;
    return static_cast<int>(q.lastInsertId().toLongLong());
}

void CameraStore::remove(int id)
{
    QSqlQuery q(QSqlDatabase::database(DB_CONNECTION));
    q.prepare("DELETE FROM cameras WHERE id = :id");
    q.bindValue(":id", id);
    q.exec();
}

std::vector<CameraConfig> CameraStore::loadAll() const
{
    std::vector<CameraConfig> configs;

    QSqlQuery q(QSqlDatabase::database(DB_CONNECTION));
    q.exec("SELECT id, name, url, password, location FROM cameras ORDER BY id");

    while (q.next()) {
        CameraConfig cfg;
        cfg.id       = q.value(0).toInt();
        cfg.name     = q.value(1).toString();
        cfg.url      = q.value(2).toString();
        cfg.password = q.value(3).toString();
        cfg.location = q.value(4).toString();
        configs.push_back(cfg);
    }

    return configs;
}
