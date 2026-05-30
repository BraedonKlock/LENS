#ifndef CAMERASTORE_H
#define CAMERASTORE_H

#include <QString>
#include <vector>

#include "CameraConfig.h"

class CameraStore {
public:
    explicit CameraStore(const QString& dataDir);

    bool open();
    int  save(const CameraConfig& config);
    void remove(int id);
    std::vector<CameraConfig> loadAll() const;

private:
    QString m_dataDir;

    bool createTable();
};
#endif
