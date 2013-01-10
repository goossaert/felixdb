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

#include <string>

#include <gtest/gtest.h>

#include "util_test.h"
#include "status.h"
#include "data_manager.h"
#include "memory_manager.h"


/*
 * Need to include test for:
 * - Adding very small items
 * - Adding very large items
 * - Adding random size items
 * - Have parameter to change the numer of items.
 * - Deleting in normal order, after all items have been added.
 * - Deleting in reverse order, after all items have been added.
 * - Deleting in random order, after all items have been added.
 * - Deleting randomly as the items are being added
 * - Close and reopen the file at random moments of the processes above, just a
 *   few times.
 */


namespace felixdb {

struct SimpleMemoryManagerParameters {
  SimpleMemoryManagerParameters(int num_items_in,
                                int size_items_min_in,
                                int size_items_max_in,
                                int num_restarts_in)
      : num_items(num_items_in),
        size_items_min(size_items_min_in),
        size_items_max(size_items_max_in),
        num_restarts(num_restarts_in) {
  }

  int num_items; // number of items that will be added
  int size_items_min;  // min size of the items
  int size_items_max;  // max size of the items -- if different from min value, the size will be random between min and max
  int num_restarts; // number of times during the process that the file is closed and reopened
};



class SimpleMemoryManagerTest : public ::testing::TestWithParam<SimpleMemoryManagerParameters> {
 public:
  SimpleMemoryManagerTest() {
  }
 protected:
  virtual void SetUp() {
    CleanUpFiles();

    felixdb::Status s;
    int page_size = getpagesize();
    int nb_pages_data = 1;
    kvsize_t size_data = page_size * nb_pages_data; // in bytes
    data_manager_ = new MockupDataManager();
    s = data_manager_->Open("mydata", size_data);
    ASSERT_TRUE(s.IsOK());

    offset_t offset_init = 0;
    memory_manager_ = new felixdb::SimpleMemoryManager();
    s = memory_manager_->Open("mymm", data_manager_, size_data, offset_init);
    ASSERT_TRUE(s.IsOK());
  }

  virtual void TearDown() {
    memory_manager_->Synchronize();
    delete data_manager_;
    delete memory_manager_;
    CleanUpFiles();
  }


  felixdb::DataManager *data_manager_;
  felixdb::MemoryManager *memory_manager_;
};



TEST_P(SimpleMemoryManagerTest, MemoryAllocationSequence) {
  felixdb::Status s;
  offset_t offset;
  kvsize_t size_total;
  std::map<offset_t, kvsize_t> mymap;
  SimpleMemoryManagerParameters params = GetParam();
  int size;
  int range = params.size_items_max - params.size_items_min + 1;
  srand(params.size_items_min + params.size_items_max);

  for (int i=0; i<params.num_items; ++i) {
    if (params.size_items_min == params.size_items_max) {
      size = params.size_items_min;
    } else {
      size = params.size_items_min + (int) ( range * (double)rand() / (RAND_MAX + 1.0) );
    }
    s = memory_manager_->AllocateMemory(size, &offset, &size_total);
    mymap[offset] = size_total;
  }

  std::map<offset_t, kvsize_t>::const_iterator it; 
  for (it = mymap.begin(); it != mymap.end(); ++it) {
    s = memory_manager_->FreeMemory(it->first, it->second);
  }

  ASSERT_EQ(1, memory_manager_->TEST_NumMemoryBlocks());
}



TEST_P(SimpleMemoryManagerTest, MemoryAllocationRandom) {
  felixdb::Status s;
  offset_t offset;
  kvsize_t size_total;
  std::map<offset_t, kvsize_t> mymap;
  SimpleMemoryManagerParameters params = GetParam();
  int size;
  int range = params.size_items_max - params.size_items_min + 1;
  srand(params.size_items_min + params.size_items_max);

  for (int i=0; i<params.num_items; ++i) {
    if (params.size_items_min == params.size_items_max) {
      size = params.size_items_min;
    } else {
      size = params.size_items_min + (int) ( range * (double)rand() / (RAND_MAX + 1.0) );
    }
    s = memory_manager_->AllocateMemory(size, &offset, &size_total);
    mymap[offset] = size_total;
  }

  int random;
  while(!mymap.empty()) {
    random = (int) (mymap.size() * (double)rand() / (RAND_MAX + 1.0));
    std::map<offset_t, kvsize_t>::iterator it = mymap.begin();
    std::advance(it, random);
    mymap.erase(it);
    s = memory_manager_->FreeMemory(it->first, it->second);
  }

  ASSERT_EQ(1, memory_manager_->TEST_NumMemoryBlocks());
}




TEST_P(SimpleMemoryManagerTest, MemoryAllocationRandomOnGoing) {
  felixdb::Status s;
  offset_t offset;
  kvsize_t size_total;
  std::map<offset_t, kvsize_t> mymap;
  SimpleMemoryManagerParameters params = GetParam();
  int size;
  int random;
  int range = params.size_items_max - params.size_items_min + 1;
  srand(params.size_items_min + params.size_items_max);

  for (int i=0; i<params.num_items; ++i) {
    if (params.size_items_min == params.size_items_max) {
      size = params.size_items_min;
    } else {
      size = params.size_items_min + (int) ( range * (double)rand() / (RAND_MAX + 1.0) );
    }
    s = memory_manager_->AllocateMemory(size, &offset, &size_total);
    mymap[offset] = size_total;

    // Delete random object
    if (rand() % 4 == 0 && !mymap.empty()) {
      random = (int) (mymap.size() * (double)rand() / (RAND_MAX + 1.0));
      std::map<offset_t, kvsize_t>::iterator it = mymap.begin();
      std::advance(it, random);
      mymap.erase(it);
      s = memory_manager_->FreeMemory(it->first, it->second);
    }

  }

  // Delete the rest of the objects
  std::map<offset_t, kvsize_t>::const_iterator it; 
  for (it = mymap.begin(); it != mymap.end(); ++it) {
    s = memory_manager_->FreeMemory(it->first, it->second);
  }


  ASSERT_EQ(1, memory_manager_->TEST_NumMemoryBlocks());
}








INSTANTIATE_TEST_CASE_P(InstantiationName,
                        SimpleMemoryManagerTest,
                        ::testing::Values(SimpleMemoryManagerParameters(100000, 160, 160, 0),
                                          SimpleMemoryManagerParameters(100000, 160, 180, 0),
                                          SimpleMemoryManagerParameters(100000, 10, 20, 0)
                                         ));

/*
*/




} // namespace felixdb


