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

//#include "index.h"
//#include "memory_manager.h"
//#include "data_manager.h"
//
//
//

#include <string>
#include <sstream>

#include "status.h"
#include "logger.h"
#include "hashdb.h"

std::string concatenate(std::string const& str, int i)
{
    std::stringstream s;
    s << str << i;
    return s.str();
}



int main()
{
    felixdb::Logger::set_current_level( felixdb::Logger::DEBUG );
    std::string value_out("value_out");
    //bool ret;
    printf("blah!\n");

    felixdb::HashMap db;
    db.Open("file.db");

    /*
    db.Put("key31", "qwertyuiopasdfghjklzxcvbnm1234qwertyuiopasdfghjklzxcvbnm1234qwertyuiopasdfghjklzxcvbnm1234qwertyuiopasdfghjklzxcvbnm1234qwertyuiopasdfghjklzxcvbnm1234");
    db.Get("key31", &value_out );
    std::cout << "out: " << value_out << std::endl;

    db.Put("key1", "value1");
    db.Get("key1", &value_out );
    std::cout << "out: " << value_out << std::endl;
    
    db.Put("key2", "value2");
    db.Put("key13", "qwertyuiopasdfghjklzxcvbnm1234qwertyuiopasdfghjklzxcvbnm1234qwertyuiopasdfghjklzxcvbnm1234qwertyuiopasdfghjklzxcvbnm1234qwertyuiopasdfghjklzxcvbnm1234");

    db.Get("key2", &value_out );
    std::cout << "out: " << value_out << std::endl;

    ret = db.Get("key13", &value_out );
    std::cout << "out: " << value_out << std::endl;

    */

    //std::string value_long("qwertyuiopasdfghjklzxcvbnm1234qwertyuiopasdfghjklzxcvbnm1234qwertyuiopasdfghjklzxcvbnm1234qwertyuiopasdfghjklzxcvbnm1234qwertyuiopasdfghjklzxcvbnm1234");
    std::string value_long("qwertyuiopasdfghjklzxcvbnm1234qwertyuiopasdfghjklzxcvbnm1234qwertyuiopasdfghjklzxcvbnm1234qwertyuiopasdfghjklzxcvbnm1234");
    /*
    char bigstring[100001];
    bigstring[100000] = '\0';
    for (int i = 0; i < 100000; ++i) {
      bigstring[i] = 'a';
    }
    std::string value_long(bigstring);
    */

    printf("length value: %d\n", value_long.size());

    int num_items = 1000000;
    int step = 100000;
    felixdb::Status s;

    for (int i = 0; i < num_items; i++) {
        s = db.Put( concatenate( "key", i ), value_long );
        if (!s.IsOK()) {
          fprintf(stderr, "Error at key %d\n", i);
          fprintf(stderr, "%s\n", s.ToString().c_str());
          exit(-1);
        }
        if (i % step == 0) {
          std::cout << "just added: " << i << std::endl;
        }

        s = db.Get( concatenate( "key", i ), &value_out );
        if (s.IsNotFound()) {
          fprintf(stderr, "Was lost at key %d\n", i);
          exit(-1);
        }
        /*
        */

    }


    /*
    for( int i = 0; i < num_items; i++ )
    {
        value_out = "value_out";
        s = db.Get( concatenate( "key", i ), &value_out );
        if (i % step == 0) {
          std::cout << "key: " << i << " - out: " << value_out << std::endl;
          std::cout << "status:" << s.ToString() << std::endl;
        }
    }

    //for( int i = (num_items - 1) / 2; i >= 0; i-- )
    for( int i = 0; i < num_items; i++ )
    {
        if (i == 13500 || i == 38954 || i == 259794 || i == 999973) {
          printf("at key %d\n", i);
        }

        value_out = "value_out";
        s = db.Get( concatenate( "key", i ), &value_out );
        if (s.IsNotFound()) {
          printf("Error in get at key %d\n", i);
          break;
        }
        if (i % step == 0) {
          std::cout << "key: " << i << " - out: " << value_out << std::endl;
          std::cout << "status:" << s.ToString() << std::endl;
        }
        db.Delete( concatenate( "key", i ) );
        s = db.Get( concatenate( "key", i ), &value_out );
        if (!s.IsNotFound()) {
          printf("Error in delete at key %d\n", i);
          break;
        }

        if( false ) {
          s = db.Get( concatenate( "key", 38954 ), &value_out );
          if (s.IsNotFound()) {
            fprintf(stderr, "Key 38954 was lost after delete of key %d\n", i);
            //exit(-1);
          }
        }
    }


    for( int i = 0; i < num_items; i++ )
    {
        value_out = "value_out";
        s = db.Get( concatenate( "key", i ), &value_out );
        if (i % step == 0) {
          std::cout << "key: " << i << " - out: " << value_out << std::endl;
          std::cout << "status:" << s.ToString() << std::endl;
        }
        if (!s.IsNotFound()) {
          fprintf(stderr, "Key %d was not deleted properly\n", i);
          exit(-1);
        }

    }
    */


    /*
    std::cout << "ret: " << ret << std::endl;
    ret = db.Get("ke1y3", &value_out );
    std::cout << "ret: " << ret << std::endl;
    ret = db.Get("ke1y4", &value_out );
    std::cout << "ret: " << ret << std::endl;

    db.Delete("key13");

    ret = db.Get("key13", &value_out );
    std::cout << "out: " << value_out << std::endl;
    std::cout << "ret: " << ret << std::endl;
    */

    /*
    db.Put("key1", "value1");
    db.Put("key2", "value2");
    db.Put("key1", "value1b");
    db.Get("key1", &value_out );
    std::cout << "out: " << value_out << std::endl;
    std::cout << db.HashFunction( "blah1" ) << std::endl;
    std::cout << db.HashFunction( "blah2" ) << std::endl;
    */

    /*
    felixdb::SimpleMemoryManager mm("temp_smm");

    offset_t offset1 = mm.AllocateMemory( 1000 );
    printf( "offset1: %d\n", offset1 );

    offset_t offset2 = mm.AllocateMemory( 1000 );
    printf( "offset2: %d\n", offset2 );

    mm.FreeMemory( offset1 );
    mm.FreeMemory( offset2 );

    felixdb::IndexList index("blah");

    offset_t offset3 = index.FindItem("key1");
    if( offset3 == -1 )
    {
        printf("item not found\n");
        index.PutItem( "key1", "value1", 0, 30 );
    }
    else
    {
        printf("item found\n");
    }


    felixdb::FileDataManager dm("mydata");
    */


    //offset_t offset3 = mm.AllocateMemory( 900 );
    //printf( "offset3: %d\n", offset3 );


    printf( "Success\n" );
    return 0;
}
