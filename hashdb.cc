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

#include "hashdb.h"

namespace felixdb {

Status HashMap::Open(const std::string& db_name, int num_buckets, int num_entries, bool overwrite) {
  // TODO: need a cleaner way to sanitize the input name
  if (db_name.size() > 0 && db_name[db_name.size()-1] == '/') {
    db_name_ = db_name.substr(0, db_name.size() - 1);
  } else {
    db_name_ = db_name;
  }
  LOG_DEBUG(classname(), "Building HashMap");

  // Making sure all the files are in a valid state
  std::string filename_freememory = db_name_ + std::string("/freememory");
  std::string filename_index = db_name_ + std::string("/index");
  std::string filename_data = db_name_ + std::string("/data");

  if (overwrite) {
    std::string filenames[] = { filename_freememory, filename_index, filename_data };

    for (int i=0; i<3; ++i) {
      if (access(filenames[i].c_str(), F_OK) != -1 && remove(filenames[i].c_str()) != 0) {
        std::string msg = std::string("Count not remove the file [") + filenames[i] + std::string("]");
        return Status::IOError(msg, strerror(errno));
      }
    }

    if (access(db_name_.c_str(), F_OK) != -1 && rmdir(db_name_.c_str()) < 0) {
      std::string msg = std::string("Count not remove the directory [") + db_name_ + std::string("]");
      return Status::IOError(msg, strerror(errno));
    }
  }

  bool exists_freememory = false, exists_index = false, exists_data = false;
  bool exists_all = false, exists_none = false;

  if (access(filename_freememory.c_str(), F_OK) != -1) exists_freememory = true;
  if (access(filename_index.c_str(), F_OK) != -1) exists_index = true;
  if (access(filename_data.c_str(), F_OK) != -1) exists_data = true;

  if (exists_freememory && exists_index && exists_data) exists_all = true;
  if (!exists_freememory && !exists_index && !exists_data) exists_none = true;

  if (!exists_all && !exists_none) {
    // There are some files but not all of them
    std::string msg = std::string("The database is in an invalid configuration, some files are missing in the directory [") + db_name_ + std::string("]");
    return Status::IOError(msg, strerror(errno));
  }

  if (access(db_name_.c_str(), F_OK) == -1) {
    if (mkdir(db_name_.c_str(), 0744) < 0) {
      std::string msg = std::string("Count not create the directory [") + db_name_ + std::string("]");
      return Status::IOError(msg, strerror(errno));
    }
  }

  LOG_DEBUG(classname(), "Getting down to business!");

  // Now open the different components of the hash table
  int page_size = getpagesize();
  int nb_pages_data = 1;
  kvsize_t size_data = page_size * nb_pages_data; // in bytes
  data_manager_ = new FileDataManager();
  Status s;
  LOG_DEBUG(classname(), "Step 0");
  s = data_manager_->Open(filename_data, size_data);
  LOG_DEBUG(classname(), "Step 1");
  if (!s.IsOK()) return s;
  LOG_DEBUG(classname(), "Step 2");
  LOG_DEBUG(classname(), "Data ready");

  offset_t offset_init = 0;
  memory_manager_ = new SimpleMemoryManager();
  LOG_DEBUG(classname(), "Step 3");
  s = memory_manager_->Open(filename_freememory, data_manager_, size_data, offset_init);
  LOG_DEBUG(classname(), "Step 4");
  if (!s.IsOK()) return s;
  LOG_DEBUG(classname(), "Step 5");
  LOG_DEBUG(classname(), "Memory manager ready");

  index_ = new BSTIndex();
  //index_ = new ListIndex();
  s = index_->Open(filename_index, data_manager_, num_buckets, num_entries);
  if (!s.IsOK()) return s;
  LOG_DEBUG(classname(), "Index ready");

  LOG_DEBUG(classname(), "HashMap is ready");

  return Status::OK();
}


Status HashMap::Close() {
  if (index_ != NULL) {
    index_->Close();
    delete index_;
    index_ = NULL;
  }

  if (memory_manager_ != NULL) {
    memory_manager_->Close();
    delete memory_manager_;
    memory_manager_ = NULL;
  }

  if (data_manager_ != NULL) {
    data_manager_->Close();
    delete data_manager_;
    data_manager_ = NULL;
  }

  return Status::OK();
}


Status HashMap::Get(const std::string& key, std::string* value) const {
  offset_t offset;
  kvsize_t size_value, size_padding;
  Status s;

  s = index_->GetItem(key, &offset, &size_value, &size_padding);
  if (!s.IsOK()) return s;

  s = data_manager_->ReadData(offset + key.size(), size_value, value);
  if (!s.IsOK()) return s;

  return Status::OK();
}



Status HashMap::Put(const std::string& key, const std::string& value) {
  Status s;
  // Check if already exists. If so, delete
  LOG_TRACE(classname(), "Put() - check exists");
  offset_t offset;
  kvsize_t size_value, size_padding;
  bool is_enabled_index = true; // for debugging
  if (is_enabled_index) {
    s = index_->GetItem(key, &offset, &size_value, &size_padding);
    if (s.IsOK()) {
      s = Delete(key);
      if (!s.IsOK()) return s;
    } else if (!s.IsNotFound()) {
      return s;
    }
  }

  // Allocate memory, set index and write data
  LOG_TRACE(classname(), "Put() - allocate");
  kvsize_t size_total;
  s = memory_manager_->AllocateMemory(key.size() + value.size(), &offset, &size_total);
  if (!s.IsOK()) return s;

  LOG_TRACE(classname(), "Put() - PutItem offset:%llu size_total:%d", offset, size_total);
  if (is_enabled_index) {
    s = index_->PutItem(key, value, offset, size_total);
    if (!s.IsOK()) return s;
  }

  LOG_TRACE(classname(), "Put() - WriteData key");
  s = data_manager_->WriteData(offset, key.size(), key.c_str());
  if (!s.IsOK()) return s;

  LOG_TRACE(classname(), "Put() - WriteData value");
  s = data_manager_->WriteData(offset + key.size(), value.size(), value.c_str());
  if (!s.IsOK()) return s;

  return Status::OK();
}


Status HashMap::Delete(const std::string& key) {
  offset_t offset;
  kvsize_t size_value, size_padding;

  Status s = index_->GetItem(key, &offset, &size_value, &size_padding);
  if (!s.IsOK()) return s;

  s = index_->DeleteItem(key);
  if (!s.IsOK()) return s;

  s = memory_manager_->FreeMemory(offset, key.size() + size_value + size_padding);
  if (!s.IsOK()) return s;

  return Status::OK();
}


Status HashMap::Synchronize() {
  Status s;

  s = index_->Synchronize();
  if (!s.IsOK()) return s;

  s = data_manager_->Synchronize();
  if (!s.IsOK()) return s;

  s = memory_manager_->Synchronize();
  if (!s.IsOK()) return s;

  return Status::OK();
}


};

