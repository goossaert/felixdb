/*******************************************************************************
 * Copyright (c) 2012-2013 Emmanuel Goossaert
 * This file is part of FelixDB.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or any later
 * version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#ifndef HERMESDB_UTIL_TEST
#define HERMESDB_UTIL_TEST

#include <cstdio>

#include "status.h"
#include "data_manager.h"

namespace felixdb {

class MockupDataManager : public DataManager {
 public:
  MockupDataManager() {}
  virtual ~MockupDataManager() {}
  virtual Status Open(const std::string& filename, uint64_t size_data) {
    size_data_ = size_data;
    db_name_ = filename;
    return Status::OK();
  }
  virtual Status ReadData(offset_t offset, int size, std::string* data) const { return Status::OK(); }
  virtual Status WriteData(offset_t offset, int size, const std::string& data) { return Status::OK(); }
  virtual int CompareData(offset_t offset, int size, const std::string& data) const { return 0; }
  virtual Status ExpandMemory(uint64_t size_data_new) {
    size_data_ = size_data_new;
    return Status::OK();
  }
  virtual uint64_t GetDataSize() const { return size_data_; }
  virtual Status Synchronize() { return Status::OK(); }

 private:
  std::string db_name_;
  uint64_t size_data_; 
};



void CleanUpFiles();

} // namespace felixdb

#endif // HERMESDB_UTIL_TEST
