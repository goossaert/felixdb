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

#ifndef HERMESDB_MEMORY_MANAGER
#define HERMESDB_MEMORY_MANAGER

#include "common.h"

#include <string>
#include <iostream>
#include <map>
#include <set>

#include "fb_file_manager.h"
#include "data_manager.h"

namespace felixdb {

//class SimpleFBFileManager;

class MemoryManager {
 public:
  MemoryManager() {}
  virtual ~MemoryManager() {}
  virtual Status Open(const std::string& filename,
                      DataManager* data_manager,
                      kvsize_t size_data,
                      offset_t offset_init) = 0;
  virtual Status Close() = 0;
  virtual Status AllocateMemory(kvsize_t size, offset_t* offset, kvsize_t* size_total) = 0;
  virtual Status FreeMemory(offset_t offset, uint64_t size) = 0;
  virtual int TEST_NumMemoryBlocks() const = 0;
  virtual Status Synchronize() = 0;
};


class SimpleMemoryManager : public MemoryManager {
 public:
  SimpleMemoryManager() {}
  virtual ~SimpleMemoryManager() {
    delete file_manager_;
  }
  virtual Status Open(const std::string& filename,
                      DataManager* data_manager,
                      kvsize_t size_data,
                      offset_t offset_init);
  virtual Status Close();
  virtual Status AllocateMemory(kvsize_t size, offset_t* offset, kvsize_t* size_total);
  virtual Status FreeMemory(offset_t offset, uint64_t size);
  virtual int TEST_NumMemoryBlocks() const { return freememory_map_.size(); }
  virtual Status Synchronize();

 private:
  std::string filename_;
  DataManager* data_manager_;
  SimpleFBFileManager* file_manager_;

  std::map< offset_t, FreeMemoryBlock > freememory_map_;
  std::set< FreeMemoryBlock > freememory_set_;

  char* classname() { return "SimpleMemoryManager"; }
};

};

#endif // HERMESDB_MEMORY_MANAGER
