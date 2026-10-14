#include "Storage.hpp"
#include <iostream>

LevelDBStorage::LevelDBStorage(const std::string& dbPath) : db(nullptr) {
    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, dbPath, &db);
    if (!status.ok()) {
        db = nullptr;
    } else {
        std::cout << "[LEVELDB STORAGE] DB active at: '" << dbPath << "'" << std::endl;
    }
}

LevelDBStorage::~LevelDBStorage() {
    if (db) delete db;
}

bool LevelDBStorage::put(const std::string& key, const std::string& value) {
    if (!db) return false;
    return db->Put(leveldb::WriteOptions(), key, value).ok();
}

std::string LevelDBStorage::get(const std::string& key) {
    if (!db) return "NOT_FOUND";
    std::string val;
    leveldb::Status s = db->Get(leveldb::ReadOptions(), key, &val);
    return s.ok() ? val : "NOT_FOUND";
}