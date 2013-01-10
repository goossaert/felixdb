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

#ifndef HERMESDB_INDEX
#define HERMESDB_INDEX

#include "common.h"
#include "data_manager.h"

namespace felixdb {

class Index {
 public:
  Index() {}
  virtual ~Index() {}
  virtual Status Open(const std::string& db_name,
                      const DataManager* data_manager,
                      int num_buckets,
                      int num_entries) = 0;
  virtual Status Close() = 0;
  virtual Status GetItem(const std::string& key,
                         offset_t* offset,
                         kvsize_t* size_value,
                         kvsize_t *size_padding) = 0;
  virtual Status PutItem(const std::string& key,
                       const std::string& value,
                       offset_t offset,
                       kvsize_t size_total) = 0;
  virtual Status DeleteItem(const std::string& key) = 0;
  virtual void PrintHeader(char* msg) = 0;
  virtual Status Synchronize() = 0;
};

};

#endif // HERMESDB_INDEX
