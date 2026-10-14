#ifndef STORAGE_HPP
#define STORAGE_HPP

#include <string>
#include <leveldb/db.h>

class LevelDBStorage {
private:
    leveldb::DB* db;

public:
    LevelDBStorage(const std::string& dbPath);
    ~LevelDBStorage();

    bool put(const std::string& key, const std::string& value);
    std::string get(const std::string& key);
};

#endif