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

#ifndef HERMESDB_FB_FILE_MANAGER
#define HERMESDB_FB_FILE_MANAGER

#include "common.h"

#include <string>
#include <iostream>
#include <map>
#include <vector>

namespace felixdb {

// FB == FreeBlock
class FBFileManager {
 public:
  FBFileManager() {}
  virtual ~FBFileManager() {}
  virtual Status Open(const std::string& filename) = 0;
  virtual Status AddBlock(offset_t off, uint64_t size) = 0;
  virtual Status DeleteBlock(offset_t off) = 0;
  virtual Status UpdateBlock(offset_t off, offset_t off_new, uint64_t size) = 0;
  virtual Status Synchronize() = 0;
};

class SimpleFBFileManager : public FBFileManager {
 public:
  SimpleFBFileManager(const std::string& db_name, uint64_t size_data, offset_t offset_init)
      : db_name_(db_name),
        size_data_(size_data),
        offset_init_(offset_init) {}
  virtual ~SimpleFBFileManager() {}

  std::map<offset_t, FreeMemoryBlockExtended> freememory_map_;
  std::vector<index_t> slots_free_;

  virtual Status Open(const std::string& filename);
  virtual Status AddBlock(offset_t off, uint64_t size);
  virtual Status DeleteBlock(offset_t off);
  virtual Status UpdateBlock(offset_t off, offset_t off_new, uint64_t size);
  virtual Status Synchronize();

 private:
  std::string db_name_;
  int fd_freememory_; 
  
  Status CreateFile(const std::string& filename);
  Status OpenFile(const std::string& filename);
  Status CloseFile();
  Status ExpandFile();

  FreeMemoryHeader* header_;
  FreeMemoryBlock* freememory_;
  uint64_t size_freememory_;
  uint64_t size_data_;
  offset_t offset_init_;

  char* classname() { return "SimpleFBFileManager"; }
};

};

#endif // HERMESDB_FB_FILE_MANAGER
