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

#ifndef HERMESDB_LIST_INDEX
#define HERMESDB_LIST_INDEX

#include "common.h"

#include <string>
#include <iostream>

#include "data_manager.h"
#include "index.h"


namespace felixdb {

class ListIndex : public Index {
 public:
  ListIndex() {}
  virtual ~ListIndex() {}
  virtual Status Open(const std::string& filename,
                      const DataManager* data_manager,
                      int num_buckets,
                      int num_entries);
  virtual Status Close();
  virtual Status GetItem(const std::string& key,
                         offset_t* offset,
                         kvsize_t* size_value,
                         kvsize_t* size_padding);
  virtual Status PutItem(const std::string& key,
                         const std::string& value,
                         offset_t offset,
                         kvsize_t size_total);
  virtual Status DeleteItem(const std::string& key);
  virtual void PrintHeader(char* msg);
  virtual Status Synchronize();



 private:
  std::string filename_;
  const DataManager* data_manager_;

  uint64_t size_header_;
  uint64_t size_buckets_;
  uint64_t size_entries_;

  int fd_index_;

  Status FindItem(const std::string& key, index_t* index_out);
  Status CreateFile(const std::string& filename, int num_buckets, int num_entries, int num_free_entries);
  Status OpenFile(const std::string& filename, num_t num_buckets, num_t num_entries);
  Status CloseFile();
  Status ExpandFile();

  int HashFunctionMul(const std::string& key) const;
  int HashFunctionMul2(const std::string& key) const;
  Status GetIndexFreeEntry(index_t* index_out);

  struct Header {
    num_t num_buckets;
    num_t num_entries;
    num_t num_free_entries;
    index_t index_free_entries_head;
    index_t index_free_entries_stack;
  };

  struct Bucket {
    index_t index_entry_head;
  };

  struct Entry {
    int32_t size_key;
    int32_t size_value;
    int32_t size_padding;
    int32_t hash_second;
    offset_t offset_record;
    index_t index_entry_prev;
    index_t index_entry_next;
  };

  Header *header_;
  Bucket *buckets_;
  Entry  *entries_;

  //int GetNumBuckets() { return header_.num_buckets; }
  //void SetNumBuckets(int n) { header_.num_buckets = n };
  char* classname() { return "ListIndex"; }
};

}

#endif // HERMESDB_LIST_INDEX
