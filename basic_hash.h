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

#ifndef HERMESDB_BASICHASH
#define HERMESDB_BASICHASH

#include <stdint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#include <string>
#include <iostream>

#define index_t int32_t
#define DATA_SIZE 256
#define DATAENTRY_SIZE ((DATA_SIZE) - 4 * 4)

namespace felixdb
{


struct Bucket
{
    index_t index_entry_begin;
};


/*
struct EntryNew
{
    // TODO index must become offset
    index_t index_prev_entry;
    index_t index_next_entry;
    index_t index_data;
    int key_size;
    int value_size;
};


struct DataHeader
{
    // TODO index must become offset
    index_t index_prev_data;
    index_t index_next_data;
    char data[ DATA_SIZE ];
};
*/



struct Entry
{
    index_t index_prev_entry;
    index_t index_next_entry;
    index_t index_prev_data;
    index_t index_next_data;
    int32_t key_size;
    int32_t value_size;
    char data[ DATAENTRY_SIZE ];
};

struct Data
{
    index_t index_prev_data;
    index_t index_next_data;
    char data[ DATA_SIZE ];
};




class HashMap
{
public:
    HashMap()
    : num_buckets_( 99997 ),
      hash_multiplier_( 31 ),
      index_free_entries_( 1 ),
      num_free_entries_( 0 )
    {
        fprintf(stdout, "SIZES %d %d\n", (int)sizeof( struct Entry ), (int)sizeof( struct Data ) );
        OpenFile( "/tmp/file.hmt" );
    }

    ~HashMap()
    {
        CloseFile();
    }

    void salut(){

        std::cout << "hash map" << std::endl;
    }

    bool Get( const std::string& key, std::string* value ) const;
    bool Put( const std::string& key, const std::string& value );
    bool Delete( const std::string& key );
    int HashFunction( const std::string& key ) const;
    void CreateFile( const std::string& filename );
    void OpenFile( const std::string& filename );
    void CloseFile();

private:
    int num_buckets_;
    int num_free_entries_;
    int hash_multiplier_;
    struct Bucket* table_;
    struct Entry* entries_;
    struct Data* data_;
    struct FreeMemory* free_memory_;
    index_t index_free_entries_;
    int size_table_;
    int size_data_;
    int fd_hashmap_;

    int GetUtil( const std::string& key, std::string* value ) const;
    bool PutUtil( const std::string& key, const std::string& value );
    bool DeleteUtil( const std::string& key, const int index_entry );
    void FindLastIndexData( int index_entry, int* index_last_data, int* num_entries );
    int FitToPageSize( int size );
};


#endif // HERMESDB_BASICHASH

}; // end namespace felixdb
