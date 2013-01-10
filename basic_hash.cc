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

#include "hash.h"

namespace felixdb
{

int HashMap::GetUtil( const std::string& key, std::string* value ) const
{
    int hash = HashFunction( key );
    fprintf( stdout, "Get [%s] [%d]\n", key.c_str(), hash );

    int index_entry_current = table_[ hash ].index_entry_begin;

    //while( 1 )
    for( int i=0; i < 5; i++ )
    {
        if( index_entry_current == 0 ) break;
        int index_current = 0;
        char buffer[ DATA_SIZE + 1 ];
        int iteration = 0;
        int size_current = 0;
        int key_size = entries_[ index_entry_current ].key_size;
        int value_size = entries_[ index_entry_current ].value_size;
        std::string local = "";
        fprintf( stdout, "Get indexes [%d] [%d]\n", index_entry_current, index_current );

        //while( 1 )
        for( int i=0; i < 5; i++ )
        {
            int index_next = 0;
            if( iteration == 0 )
            {
                size_current = DATAENTRY_SIZE;
                memcpy( buffer, entries_[ index_entry_current ].data, size_current );
                index_next = entries_[ index_entry_current ].index_next_data;
            }
            else
            {
                size_current = DATA_SIZE;
                memcpy( buffer, data_[ index_current ].data, size_current );
                index_next = data_[ index_current ].index_next_data;
            }
            buffer[ size_current ] = NULL;
            local += buffer;
            fprintf( stdout, "Get indexes inner [%d] [%d]\n", index_current, index_next );
            if( index_next == 0 ) break;
            index_current = index_next;
            iteration++;
        }

        if( key == local.substr( 0, key_size ) )
        {
            *value = local.substr( key_size, value_size );
            fprintf( stdout, "key found\n" );
            return index_entry_current;
        }
        else
        {
            fprintf( stdout, "key miss [%s]\n", local.substr( 0, key_size ).c_str() );
            index_entry_current = entries_[ index_entry_current ].index_next_entry;
        }
    }

    *value = "";
    return 0;
}


bool HashMap::Get( const std::string& key, std::string* value ) const
{
    int index = GetUtil( key, value );
    if( index > 0 )
    {
        return true;
    }
    else
    {
        return false; 
    }
}

bool HashMap::Put( const std::string& key, const std::string& value )
{
    std::string value_temp;
    int index_entry = GetUtil( key, &value_temp );
    if( index_entry > 0 )
    {
        DeleteUtil( key, index_entry ); 
    }
    return PutUtil( key, value );
}


bool HashMap::PutUtil( const std::string& key, const std::string& value )
{
    if( value.size() > num_free_entries_ * DATA_SIZE )
    {
        // TODO: this test is invalid, as it does not take into account
        //       the DATAENTRY_SIZE which is different
        fprintf( stderr, "No more space, dying\n" );
        return false;
    }

    fprintf( stdout, "num_free_entries_ [%d]\n", num_free_entries_ );
    std::string keyvalue = key + value;

    int index_begin   = index_free_entries_;
    int index_current = index_begin;
    int next_index_free_entries = 0;
    int nb_iterations = 1;
    nb_iterations += ( keyvalue.size() - DATAENTRY_SIZE ) / DATA_SIZE;
    if( ( keyvalue.size() - DATAENTRY_SIZE ) % DATA_SIZE > 0 )
    {
        nb_iterations += 1;
    }
    int size_current = 0;
    int offset_current = 0;
    fprintf( stdout, "nb_iterations [%d]\n", nb_iterations );

    for( int i = 0; i < nb_iterations; ++i )
    {
        if( i == 0 )
        {
            size_current = DATAENTRY_SIZE; 
        }
        else if( i < nb_iterations - 1 )
        {
            size_current = DATA_SIZE;
        }
        else
        {
            size_current = ( keyvalue.size() - DATAENTRY_SIZE ) % DATA_SIZE; 
        }

        fprintf( stdout, "loop [%d]: size_current [%d]\n", i, size_current );
        fprintf( stdout, "loop [%d]: index_current [%d]\n", i, index_current );
        fprintf( stdout, "loop [%d]: index_prev_data [%d]\n", i, data_[ index_current ].index_prev_data );
        fprintf( stdout, "loop [%d]: index_next_data [%d]\n", i, data_[ index_current ].index_next_data );
        next_index_free_entries = data_[ index_current ].index_next_data;

        if( i == 0 )
        {
            entries_[ index_current ].key_size = key.size();
            entries_[ index_current ].value_size = value.size();
            fprintf( stdout, "loop [%d]: key_size [%d]\n", i, (int)key.size() );
            fprintf( stdout, "loop [%d]: value_size [%d]\n", i, (int)value.size() );
            memcpy( entries_[ index_current ].data, keyvalue.c_str() + offset_current, size_current );

            // using temporary variables because the data and entry structures are not
            // guaranteed to store the indexes the same way
            int tmp_index_prev_data = data_[ index_current ].index_prev_data;
            int tmp_index_next_data = data_[ index_current ].index_next_data;
            entries_[ index_current ].index_prev_data = tmp_index_prev_data;
            entries_[ index_current ].index_next_data = tmp_index_next_data;
        }
        else
        {
            memcpy( data_[ index_current ].data, keyvalue.c_str() + offset_current, size_current );
        }
        
        if( i < nb_iterations - 1 )
        {
            index_current = data_[ index_current ].index_next_data;
        }

        offset_current += size_current;
        num_free_entries_ -= 1;
    }

    entries_[ index_current ].index_next_data = 0;//tmp_index_next_data;
        
    index_free_entries_ = next_index_free_entries;
    fprintf( stdout, "index_free_entries_ [%d]\n", index_free_entries_ );
    data_[ index_current ].index_next_data = 0;

    int hash = HashFunction( key );
    fprintf( stdout, "Put [%s] [%d]\n", key.c_str(), hash );
    fprintf( stdout, "Put index_begin before [%d]\n", table_[ hash ].index_entry_begin );
    entries_[ index_begin ].index_prev_entry = 0;
    entries_[ index_begin ].index_next_entry = table_[ hash ].index_entry_begin;
    table_[ hash ].index_entry_begin = index_begin;
    fprintf( stdout, "Put index_begin after [%d]\n", table_[ hash ].index_entry_begin );
    fprintf( stdout, "Put index entry [%d,%d]\n", entries_[ index_begin ].index_prev_entry, entries_[ index_begin ].index_next_entry );

    return true;
}


void HashMap::FindLastIndexData( int index_entry, int* index_last_data, int* num_entries )
{
    int index_data = entries_[ index_entry ].index_next_data;
    *num_entries = 0;
    while( 1 )
    {
        *num_entries++;
        if(    index_data == 0 
            || data_[ index_data ].index_next_data == 0 )
        {
            break;
        }

        index_data = data_[ index_data ].index_next_data;
    }
    *index_last_data = index_data;
}


bool HashMap::Delete( const std::string& key )
{
    std::string value_temp;
    int index_entry = GetUtil( key, &value_temp );
    if( index_entry == 0 ) return false;
    return DeleteUtil( key, index_entry );
}


bool HashMap::DeleteUtil( const std::string& key, const int index_entry )
{
    int index_last_data, num_entries;
    int index_first_data = index_entry;
    FindLastIndexData( index_entry, &index_last_data, &num_entries );

    // Turn the entry into the first data
    int index_next_data, index_prev_data;
    int index_next_entry, index_prev_entry;
    index_prev_data = entries_[ index_entry ].index_prev_data;
    index_next_data = entries_[ index_entry ].index_next_data;
    index_prev_entry = entries_[ index_entry ].index_prev_entry;
    index_next_entry = entries_[ index_entry ].index_next_entry;

    //memset( &(entries_[ index_entry ]), 0, sizeof( entries_[ index_entry ] ) );
    data_[ index_first_data ].index_prev_data = index_prev_data;
    data_[ index_first_data ].index_next_data = index_next_data;

    // Connect things for the last data entity
    num_free_entries_ += num_entries;
    data_[ index_last_data ].index_next_data     = index_free_entries_;
    data_[ index_free_entries_ ].index_prev_data = index_last_data;
    // TODO: what if index_free_entries_ is 0?

    // Connect things for the first data entity
    data_[ index_first_data ].index_prev_data = 0;
    index_free_entries_ = index_first_data;

    // Connect surrounding entries
    if( index_prev_entry != 0 ) entries_[ index_prev_entry ].index_next_entry = index_next_entry; 
    if( index_next_entry != 0 ) entries_[ index_next_entry ].index_prev_entry = index_prev_entry; 

    // Update the table
    int hash = HashFunction( key );
    if( index_prev_entry == 0 )
    {
        fprintf( stdout, "Delete: update table\n" );
        table_[ hash ].index_entry_begin = index_next_entry;
    }

    fprintf( stdout, "Delete: Done\n" );

    return true;
}










int HashMap::HashFunction( const std::string& key ) const
{
    int sum = 0;
    for( int i = 0; i < key.size(); ++i )
    {
        //sum = key[ i ] + sum * hash_multiplier_;
        sum = sum + key[ i ] * hash_multiplier_;
    }
    return sum % num_buckets_;
}


int HashMap::FitToPageSize( int size )
{
    int page_size = getpagesize();
    if( size % page_size == 0 )
    {
        return size;
    }
    else
    {
        int mul = size / page_size;
        return ( mul + 1 ) * page_size;
    }
}


void HashMap::CreateFile( const std::string& filename )
{
    int fd_hashmap_;
    int page_size = getpagesize();
    char *buffer[page_size];
    memset( buffer, 0, page_size );
    num_free_entries_ = num_buckets_;

    size_table_ = FitToPageSize( ( sizeof( struct Bucket ) ) * num_buckets_ );
    size_data_  = FitToPageSize( ( sizeof( struct Entry ) ) * num_buckets_ );

    if( (fd_hashmap_ = open( filename.c_str(), O_RDWR | O_CREAT, 0644 ) ) < 0 )
    {
        //err_sys ("can't create %s for writing", "[file]");
        std::cerr << "Error file in Create - open() [" << errno << "]: " << strerror (errno) << std::endl;
    }

    for( int i = 0; i < size_table_ / page_size; ++i )
    {
        write( fd_hashmap_, buffer, page_size );
    }

    Data dm;
    memset( dm.data, 0, DATA_SIZE );
    for( int i = 0; i < num_buckets_; ++i )
    {
        dm.index_prev_data = i - 1;
        dm.index_next_data = i + 1;
        write( fd_hashmap_, &dm, sizeof( struct Data ) );
    }

    close( fd_hashmap_ );
}


void HashMap::OpenFile( const std::string& filename )
{
    CreateFile( filename );
    if( (fd_hashmap_ = open( filename.c_str(), O_RDWR )) < 0 )
    {
        std::cerr << "Error file in Open - open() [" << errno << "]: " << strerror (errno) << std::endl;
    }



    int page_size = getpagesize();
    table_ = (struct Bucket*) mmap( 0,
                                    size_table_,
                                    PROT_READ | PROT_WRITE,
                                    MAP_SHARED,
                                    fd_hashmap_,
                                    0 );

    printf("mmap addr:[%p]\n", table_ );

    if( table_ == MAP_FAILED )
    {
        std::cout << "Error mmap() [" << errno << "]: " << strerror (errno) << std::endl;
    }



    data_  = (struct Data*)  mmap( 0,
                                    size_data_,
                                    PROT_READ | PROT_WRITE,
                                    MAP_SHARED,
                                    fd_hashmap_,
                                    size_table_ );

    printf("mmap addr:[%p]\n", data_ );

    if( data_ == MAP_FAILED )
    {
        std::cout << "Error mmap() [" << errno << "]: " << strerror (errno) << std::endl;
    }

    entries_ = (struct Entry*)data_;
}

void HashMap::CloseFile()
{
    munmap( table_, size_table_ );
    munmap( data_, size_data_ );
    close( fd_hashmap_ );
}



};

