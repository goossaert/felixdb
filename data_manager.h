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

#ifndef HERMESDB_DATA_MANAGER
#define HERMESDB_DATA_MANAGER


// for statfs
//#include <sys/statvfs.h>
#include <sys/param.h>
#include <sys/mount.h>

#include <string>
#include <iostream>

#include "common.h"


namespace felixdb {

class DataManager {
 public:
  DataManager() {}
  virtual ~DataManager() {}
  virtual Status Open(const std::string& filename, uint64_t size_data) = 0;
  virtual Status ReadData(offset_t offset, int size, std::string* data) const = 0;
  virtual Status WriteData(offset_t offset, int size, const std::string& data) = 0;
  virtual int CompareData(offset_t offset, int size, const std::string& data) const = 0;
  virtual Status ExpandMemory(uint64_t size_data_new) = 0;
  virtual uint64_t GetDataSize() const = 0;
  virtual Status Synchronize() = 0;
}; 


class FileDataManager : public DataManager {
 public:
  FileDataManager() {}
  virtual ~FileDataManager() {}
  virtual Status Open(const std::string& filename, uint64_t size_data);
  virtual Status ReadData(offset_t offset, int size, std::string* data) const;
  virtual Status WriteData(offset_t offset, int size, const std::string& data);
  virtual int CompareData(offset_t offset, int size, const std::string& data) const;
  virtual Status ExpandMemory(uint64_t size_data_new) { return ExpandFile(size_data_new); }
  virtual uint64_t GetDataSize() const { return header_->size_data; }
  virtual Status Synchronize();

 private:
  std::string db_name_;
  int fd_data_;

  Status CreateFile(const std::string& filename, uint64_t size_data);
  Status OpenFile(const std::string& filename, uint64_t size_data);
  Status CloseFile();
  Status ExpandFile(uint64_t size_data_new);

  char *data_;

  struct HeaderData {
    uint64_t size_data; 
  };

  struct HeaderData *header_;

  char* classname() const { return "FileDataManager"; };
};

};



#endif // HERMESDB_DATA_MANAGER
