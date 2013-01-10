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

#include "data_manager.h"

namespace felixdb {

Status FileDataManager::Open(const std::string& filename, uint64_t size_data) {
  Status s = OpenFile("/tmp/felixdb/data", size_data);
  if (!s.IsOK()) return s; 
  struct statfs stat;
  fstatfs(fd_data_, &stat);
  float mul = stat.f_bsize * stat.f_bfree;
  //printf("Disk stats: %d %d %.0f | %.0f Mb\n", (int)stat.f_bsize, (int)stat.f_bfree, mul, mul / (1024 * 1024));
  return Status::OK();
}

Status FileDataManager::ReadData(offset_t offset, int size, std::string* data) const {
  //printf("FileDataManager::ReadData() %d %d\n", (int)offset, (int)size);
  char* buffer = new char[size + 1];
  buffer[size] = '\0';

  memcpy(buffer, data_ + offset, size);
  *data = std::string(buffer);
  delete[] buffer;
  return Status::OK();
}


Status FileDataManager::WriteData(offset_t offset, int size, const std::string& data) {
  memcpy(data_ + offset, data.c_str(), size);
  return Status::OK();
}


int FileDataManager::CompareData(offset_t offset, int size, const std::string& data) const {
  LOG_TRACE(classname(), "CompareData() - key:[%s] data_:%p offset:%d size:%d size_data:%llu\n", data.c_str(), data_, offset, size, header_->size_data);
  char* buffer = new char[size + 1];
  buffer[size] = '\0';
  memcpy(buffer, data_ + offset, size);
  LOG_TRACE(classname(), "CompareData() - [%s] [%s]\n", data.c_str(), buffer);

  return memcmp(data_ + offset, data.c_str(), size);
}


Status FileDataManager::CreateFile(const std::string& filename, uint64_t size_data) {
  int page_size = getpagesize();
  char buffer[page_size];
  if ((fd_data_ = open(filename.c_str(), O_RDWR | O_CREAT, 0644)) < 0) {
    std::string msg = std::string("Count not open data file [") + filename + std::string("]");
    return Status::IOError(msg, strerror(errno));
  }

  memset(buffer, 0, page_size);
  struct HeaderData *header = (struct HeaderData*) buffer;
  header->size_data = size_data;
  if (write(fd_data_, buffer, page_size) < 0)
  {
    std::string msg = std::string("Count not write header to data file [") + filename + std::string("]");
    return Status::IOError(msg, strerror(errno));
  }

  memset(buffer, 0, page_size);
  for (int i = 0; i < size_data / page_size; ++i) {
    if (write(fd_data_, buffer, page_size) < 0)
    {
      std::string msg = std::string("Count not write dummy to data file [") + filename + std::string("]");
      return Status::IOError(msg, strerror(errno));
    }
  }

  LOG_DEBUG(classname(), "Data file created with size_data:%llu\n", size_data);

  close(fd_data_);
  return Status::OK();
}



Status FileDataManager::OpenFile(const std::string& filename, uint64_t size_data) {
  Status s;
  if (access(filename.c_str(), F_OK) != -1) {
    // do nothing
    // printf("data file exists\n");
  } else {
    //printf("data file doesn't exist\n");
    s = CreateFile(filename, size_data);
    if (!s.IsOK()) return s;
  }
 
  if ((fd_data_ = open(filename.c_str(), O_RDWR)) < 0) {
    std::string msg = std::string("Count not open data file [") + filename + std::string("]");
    return Status::IOError(msg, strerror(errno));
  }

  int page_size = getpagesize();

  header_ = static_cast<struct HeaderData *>(mmap(0,
                                                  page_size,
                                                  PROT_READ | PROT_WRITE,
                                                  MAP_SHARED,
                                                  fd_data_,
                                                  0));

  data_ = static_cast<char *>(mmap(0,
                              header_->size_data,
                              PROT_READ | PROT_WRITE,
                              MAP_SHARED,
                              fd_data_,
                              page_size));

  //printf("FileDataManager - mmap addr:[%p]\n", data_);

  if (data_ == MAP_FAILED) {
    return Status::IOError("mmap() failed", strerror(errno));
  }
  return Status::OK();
}

Status FileDataManager::CloseFile() {
  int page_size = getpagesize();
  munmap(data_, header_->size_data);
  munmap(header_, page_size);
  close(fd_data_);
  return Status::OK();
}


Status FileDataManager::ExpandFile(uint64_t size_data_new) {
  uint64_t page_size = getpagesize();
  munmap(data_, header_->size_data);

  if (ftruncate(fd_data_, page_size + size_data_new) != 0) {
    return Status::IOError("Could not expand data file", strerror(errno));
  }

  //printf("FileDataManager::ExpandFile() - [%llu]->[%llu]\n", header_->size_data, size_data_new);
  header_->size_data = size_data_new;

  data_ = static_cast<char *>(mmap(0,
                                   size_data_new,
                                   PROT_READ | PROT_WRITE,
                                   MAP_SHARED,
                                   fd_data_,
                                   page_size));

  //printf("FileDataManager::ExpandFile() - mmap addr:[%p]\n", data_);

  //printf("FileDataManager::ExpandFile() - before\n");
  if (data_ == MAP_FAILED) {
    //printf("FileDataManager::ExpandFile() - mmap() failed\n");
    return Status::IOError("mmap() failed", strerror(errno));
  }
  //printf("FileDataManager::ExpandFile() - after\n");

  //printf("FileDataManager::ExpandFile() - success!\n");
  return Status::OK();
}


Status FileDataManager::Synchronize() {
  uint64_t page_size = getpagesize();
  if (msync(header_, page_size, MS_SYNC) != 0) {
    return Status::IOError("Could not synchronize header in data file", strerror(errno));
  }

  if (msync(data_, header_->size_data, MS_SYNC) != 0) {
    return Status::IOError("Could not synchronize data in data file", strerror(errno));
  }

  return Status::OK();
}


};
