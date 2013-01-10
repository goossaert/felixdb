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

#ifndef HERMESDB_HASH
#define HERMESDB_HASH

#include "common.h"

#include "list_index.h"
#include "bst_index.h"
#include "memory_manager.h"
#include "data_manager.h"

#include <string>
#include <iostream>

namespace felixdb {

class HashMap {
 public:
  HashMap() {}
  ~HashMap() {
    Close();
  }
  Status Open(const std::string& db_name, int num_buckets, int num_entries, bool overwrite);
  Status Close();
  Status Get(const std::string& key, std::string* value) const;
  Status Put(const std::string& key, const std::string& value);
  Status Delete(const std::string& key);
  Status Synchronize();

 private:
  std::string db_name_;
  int num_buckets_;

  Index* index_;
  DataManager* data_manager_;
  MemoryManager* memory_manager_;
  char* classname() { return "HashMap"; }
};


}; // end namespace felixdb

#endif // HERMESDB_HASH
