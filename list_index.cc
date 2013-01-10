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

#include "list_index.h"

namespace felixdb {


void ListIndex::PrintHeader(char*msg)
{
  fprintf(stdout, "Header ListIndex\n");
}

Status ListIndex::Open(const std::string& filename, const DataManager* data_manager, int num_buckets, int num_entries) {
  filename_ = filename;
  data_manager_ = data_manager;
  Status s = OpenFile("/tmp/felixdb/index", num_buckets, num_entries);
  if(!s.IsOK()) return s;
  return Status::OK();
}


Status ListIndex::Close() {
  if (close(fd_index_) < 0) {
    std::string msg = std::string("Count not close index file [") + filename_ + std::string("]");
    return Status::IOError(msg, strerror(errno));
  }
  return Status::OK();
}



int ListIndex::HashFunctionMul(const std::string& key) const {
  int sum = 0;
  int hash_multiplier = 31;
  for (int i = 0; i < key.size(); ++i) {
    sum = key[i] + sum * hash_multiplier;
  }
  if (sum < 0) sum *= -1;
  return sum;
}



int ListIndex::HashFunctionMul2(const std::string& key) const {
  int sum = 0;
  int hash_multiplier = 31;
  for (int i = 0; i < key.size(); ++i) {
    sum = sum + key[i] * hash_multiplier;
  }
  if (sum < 0) sum *= -1;
  return sum;
}



Status ListIndex::FindItem(const std::string& key, index_t* index_out) {
  int32_t hash1 = HashFunctionMul(key);
  int32_t hash2 = HashFunctionMul2(key);

  int bucket = hash1 % header_->num_buckets;
 
  int index_current = buckets_[bucket].index_entry_head;
  LOG_TRACE(classname(), "FindItem() - key:[%s] hash1:[%d] hash2:[%d] bucket:[%d] index_init:[%d]", key.c_str(), hash1, hash2, bucket, index_current);

  LOG_TRACE(classname(), "FindItem() - index_current:%d\n", index_current);
  while (index_current != 0) {
    if (   entries_[index_current].hash_second == hash2
        && entries_[index_current].size_key == key.size()) {
      if (data_manager_->CompareData(entries_[index_current].offset_record, key.size(), key) == 0) {
        *index_out = index_current;
        return Status::OK();
      }
    }

    index_current = entries_[index_current].index_entry_next;
    LOG_TRACE(classname(), "FindItem() - index_current:%d\n", index_current);
  }

  return Status::NotFound("Item not found in index");
}

/*
 *
  offset_t offset = memory_manager_->AllocateMemory(size_kv, &size_total);

  data_manager_.set(offset,      key,   key.size());
  data_manager_.set(offset + key.size, value, value.size());
 * */

Status ListIndex::PutItem(const std::string& key,
                          const std::string& value,
                          offset_t offset,
                          kvsize_t size_total) {
  Status s;
  int32_t hash1 = HashFunctionMul(key);
  int32_t hash2 = HashFunctionMul2(key);
  int bucket = hash1 % header_->num_buckets;

  index_t index;
  s = FindItem(key, &index);
  if (s.IsOK()) {
    s = DeleteItem(key);
    if (!s.IsOK()) return s;
  } else if (!s.IsNotFound()) {
    return s;
  }
  LOG_TRACE(classname(), "PutItems() - Not found\n");

  int size_kv = key.size() + value.size();

  index_t index_entry;
  s = GetIndexFreeEntry(&index_entry);

  LOG_TRACE(classname(), "PutItems() - key:[%s]\n", key.c_str());
  LOG_TRACE(classname(), "PutItems() - free:[%d] bucket:[%d]\n", index_entry, bucket);
  entries_[index_entry].size_key = key.size();
  entries_[index_entry].size_value = value.size();
  entries_[index_entry].size_padding = size_total - size_kv;
  entries_[index_entry].hash_second = hash2;
  entries_[index_entry].offset_record = offset;
  entries_[index_entry].index_entry_prev = 0;
  entries_[index_entry].index_entry_next = buckets_[bucket].index_entry_head;
 
  /*
  printf("ListIndex::PutItems() - sizes: %d %d %d\n",
      (int) entries_[index_entry].size_key,
      (int) entries_[index_entry].size_value,
      (int) entries_[index_entry].size_padding
     );
  */

  buckets_[bucket].index_entry_head = index_entry;
  return Status::OK();
}



Status ListIndex::GetItem(const std::string& key,
                          offset_t* offset,
                          kvsize_t* size_value,
                          kvsize_t* size_padding) {
  //
  index_t index;
  Status s = FindItem(key, &index);
  if (!s.IsOK()) return s;

  *offset = entries_[index].offset_record;
  *size_value = entries_[index].size_value;
  *size_padding = entries_[index].size_padding;

  return Status::OK();
}


Status ListIndex::DeleteItem(const std::string& key) {
  int32_t hash1 = HashFunctionMul(key);
  int32_t hash2 = HashFunctionMul2(key);
  int bucket = hash1 % header_->num_buckets;

  index_t index;
  Status s = FindItem(key, &index);
  if (!s.IsOK()) return s;

  int index_prev = entries_[index].index_entry_prev;
  int index_next = entries_[index].index_entry_next;

  if (index_prev != 0) {
    entries_[index_prev].index_entry_next = index_next;
  } else {
    buckets_[bucket].index_entry_head = index_next;
  }

  if (index_next != 0) {
    entries_[index_next].index_entry_prev = index_prev;
  }

  // Put the entry in the list of free entries.
  // The prev is always set to 0, because we don't need to maintain
  // this list doubly-directed.
  entries_[index].index_entry_prev = 0;
  entries_[index].index_entry_next = header_->index_free_entries_head;
  header_->index_free_entries_head = index;
  header_->num_free_entries += 1;

  return Status::OK();
}
              

Status ListIndex::GetIndexFreeEntry(index_t* index_out) {
  index_t index_entry; 
  if (header_->index_free_entries_head > 0) {
    index_entry = header_->index_free_entries_head;
    header_->index_free_entries_head = entries_[index_entry].index_entry_next;
    //printf("ListIndex::GetIndexFreeEntry(): getting from list\n");
  } else {
    if (header_->index_free_entries_stack == header_->num_free_entries) {
      //printf("ListIndex::GetIndexFreeEntry(): try to expand index\n");
      Status s = ExpandFile(); 
      if (!s.IsOK()) return s;
      //else printf("ListIndex::GetIndexFreeEntry(): expand success\n");
    }
    
    // Check again after the file was maybe expanded
    if (header_->index_free_entries_stack == header_->num_free_entries) {
      index_entry = -1;
      //printf("ListIndex::GetIndexFreeEntry(): No entry available!\n");
    } else {
      index_entry = header_->index_free_entries_stack;
      header_->index_free_entries_stack += 1;
      //printf("ListIndex::GetIndexFreeEntry(): getting from stack\n");
    }
  }

  header_->num_free_entries -= 1;
  *index_out = index_entry;
  return Status::OK();
}




Status ListIndex::CreateFile(const std::string& filename, int num_buckets, int num_entries, int num_free_entries) {
  int page_size = getpagesize();
  char buffer[page_size];
  if ((fd_index_ = open(filename.c_str(), O_RDWR | O_CREAT, 0644)) < 0) {
    std::string msg = std::string("Count not create index file [") + filename + std::string("]");
    return Status::IOError(msg, strerror(errno));
  }

  // Writing the header
  memset(buffer, 0, page_size);
  struct Header* header = reinterpret_cast<struct Header*>(buffer);
  header->num_buckets = num_buckets;
  header->num_entries = num_entries;
  header->num_free_entries = num_entries - 1;
  header->index_free_entries_head = 0;
  header->index_free_entries_stack = 1;
  write(fd_index_, buffer, page_size);

  // Writing the bucket array
  memset(buffer, 0, page_size);
  for (int i = 0; i < size_buckets_ / page_size; ++i) {
    write(fd_index_, buffer, page_size);
  }

  // Writing the entries
  memset(buffer, 0, page_size);
  for (int i = 0; i < size_entries_ / page_size; ++i) {
    write(fd_index_, buffer, page_size);
  }

  /*
  struct Entry* entry = (struct Entry*) buffer;
  for (int i = 0; i < num_entries; ++i) {
    entry->index_entry_prev = i - 1;
    entry->index_entry_next = i + 1;
    if (i == num_entries - 1) {
      entry->index_entry_next = 0;
    }
    write(fd_index_, buffer, sizeof(struct Entry));
  }

  // Padding for the entries
  write(fd_index_, buffer, size_entries_ - sizeof(struct ListIndex::Entry) * num_entries);
  */

  close(fd_index_);
  return Status::OK();
}



Status ListIndex::OpenFile(const std::string& filename, num_t num_buckets, num_t num_entries) {
  size_header_ = FitToPageSize(sizeof(struct ListIndex::Header));
  size_buckets_ = FitToPageSize(sizeof(struct ListIndex::Bucket) * num_buckets);
  size_entries_ = FitToPageSize(sizeof(struct ListIndex::Entry) * num_entries);

  if (access(filename.c_str(), F_OK) != -1) {
    printf("index file exists\n");
  } else {
    printf("index file doesn't exist\n");
    CreateFile(filename, num_buckets, num_entries, num_entries);
  }
 
  if ((fd_index_ = open(filename.c_str(), O_RDWR)) < 0) {
    std::string msg = std::string("Count not open index file [") + filename + std::string("]");
    return Status::IOError(msg, strerror(errno));
  }

  int page_size = getpagesize();

  header_ = static_cast<struct Header*>(mmap(0,
                                             size_header_,
                                             PROT_READ | PROT_WRITE,
                                             MAP_SHARED,
                                             fd_index_,
                                             0));

  buckets_ = static_cast<struct Bucket*>(mmap(0,
                                              size_buckets_,
                                              PROT_READ | PROT_WRITE,
                                              MAP_SHARED,
                                              fd_index_,
                                              size_header_));

  entries_ = static_cast<struct Entry*>(mmap(0,
                                             size_entries_,
                                             PROT_READ | PROT_WRITE,
                                             MAP_SHARED,
                                             fd_index_,
                                             size_header_ + size_buckets_));

  //printf("mmap addr:[%p] [%p] [%p]\n", header_, buckets_, entries_);

  if (header_ == MAP_FAILED || buckets_ == MAP_FAILED || entries_ == MAP_FAILED) {
    return Status::IOError("Could not mmap() index file", strerror(errno));
  }

  return Status::OK();
}



Status ListIndex::CloseFile() {
  munmap(header_,  size_header_);
  munmap(buckets_, size_buckets_);
  munmap(entries_, size_entries_);
  close(fd_index_);
  return Status::OK();
}


Status ListIndex::ExpandFile() {
  munmap(entries_, size_entries_);

  uint64_t expansion_coef = 2;
  num_t num_entries_current = header_->num_entries;
  num_t num_entries_new = header_->num_entries * expansion_coef;
  uint64_t size_entries_new = FitToPageSize(sizeof(struct ListIndex::Entry) * num_entries_new);

  if (ftruncate(fd_index_, size_header_ + size_buckets_ + size_entries_new) != 0) {
    return Status::IOError("Could not extend index file", strerror(errno));
  }

  // Update file header
  header_->num_entries = num_entries_new;
  header_->num_free_entries += num_entries_new - num_entries_current;
  size_entries_ = size_entries_new;

  LOG_DEBUG(classname(), "ExpandFile - num_buckets:%llu num_entries:%llu num_free_entries:%llu index_free_entries_head:%d index_free_entries_stack:%d", header_->num_buckets, header_->num_entries, header_->num_free_entries, (int)header_->index_free_entries_head, (int)header_->index_free_entries_stack);

  // Re-mmap() file
  entries_ = static_cast<struct Entry*>(mmap(0,
                                             size_entries_new,
                                             PROT_READ | PROT_WRITE,
                                             MAP_SHARED,
                                             fd_index_,
                                             size_header_ + size_buckets_));

  LOG_DEBUG(classname(), "ExpandFile - mmap addr:[%p]", entries_);

  if (entries_ == MAP_FAILED) {
    return Status::IOError("Could not mmap() index file", strerror(errno));
  }

  return Status::OK();
}

Status ListIndex::Synchronize() {
  if (msync(header_, size_header_, MS_SYNC) != 0) {
    return Status::IOError("Could not synchronize header in index file", strerror(errno));
  }

  if (msync(buckets_, size_buckets_, MS_SYNC) != 0) {
    return Status::IOError("Could not synchronize buckets in index file", strerror(errno));
  }

  if (msync(entries_, size_entries_, MS_SYNC) != 0) {
    return Status::IOError("Could not synchronize entries in index file", strerror(errno));
  }

  return Status::OK();
}


}
