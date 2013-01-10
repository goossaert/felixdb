/*******************************************************************************
 * Copyright (c) 2012-2013 Emmanuel Goossaert
 * This file is part of FelixDB, and was *largely* inspired from
 * LevelDB's code.
 *
 * Copyright (c) 2011 The LevelDB Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file of the LevelDB project. See the AUTHORS file
 * for names of contributors.
 ******************************************************************************/

#ifndef HERMESDB_LOGGER_H_
#define HERMESDB_LOGGER_H_
/* most of the code here was taken from LevelDB's logger
 *
 * */

#include <algorithm>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <string>

namespace felixdb {

// An interface for writing log messages.
class LoggerOld {
 public:
  LoggerOld() { }
  virtual ~LoggerOld() { }

  // Write an entry to the log file with the specified format.
  virtual void Logv(const char* format, va_list ap) = 0;

 private:
  // No copying allowed
  LoggerOld(const LoggerOld&) { }
  void operator=(const LoggerOld&) { }
};

class PosixLogger : public LoggerOld {
 private:
  FILE* file_;
 public:
  PosixLogger() : file_(NULL) { }
  PosixLogger(FILE* f) : file_(f) { }
  virtual ~PosixLogger() {
    if (file_ != NULL) fclose(file_);
  }
  virtual void Logv(const char* format, va_list ap) {

    // We try twice: the first time with a fixed-size stack allocated buffer,
    // and the second time with a much larger dynamically allocated buffer.
    char buffer[500];
    for (int iter = 0; iter < 2; iter++) {
      char* base;
      int bufsize;
      if (iter == 0) {
        bufsize = sizeof(buffer);
        base = buffer;
      } else {
        bufsize = 30000;
        base = new char[bufsize];
      }
      char* p = base;
      char* limit = base + bufsize;

      struct timeval now_tv;
      gettimeofday(&now_tv, NULL);
      const time_t seconds = now_tv.tv_sec;
      struct tm t;
      localtime_r(&seconds, &t);
      p += snprintf(p, limit - p,
                    "%04d/%02d/%02d-%02d:%02d:%02d.%06d ",
                    t.tm_year + 1900,
                    t.tm_mon + 1,
                    t.tm_mday,
                    t.tm_hour,
                    t.tm_min,
                    t.tm_sec,
                    static_cast<int>(now_tv.tv_usec));

      // Print the message
      if (p < limit) {
        va_list backup_ap;
        va_copy(backup_ap, ap);
        p += vsnprintf(p, limit - p, format, backup_ap);
        va_end(backup_ap);
      }

      // Truncate to available space if necessary
      if (p >= limit) {
        if (iter == 0) {
          continue;       // Try again with larger buffer
        } else {
          p = limit - 1;
        }
      }

      // Add newline if necessary
      if (p == base || p[-1] != '\n') {
        *p++ = '\n';
      }

      assert(p <= limit);
      if (file_ != NULL) {
        fwrite(base, 1, p - base, file_);
        fflush(file_);
      } else {
        //fprintf(stderr, base);
      }
      if (base != buffer) {
        delete[] base;
      }
      break;
    }
  }
};


class Logger {
 public:
  Logger(std::string name) { }
  virtual ~Logger() { }

  // Write an entry to the log file with the specified format.
  static void Logv(int level, const char* logname, const char* format, ...) {
    if (level>current_level()) return;
    va_list args;
    va_start(args, format);
    /*
    */
    fprintf(stderr, logname);
    fprintf(stderr, " - ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
    //fprintf(stderr, format, ap); 
  }

  //static int current_level() { return Logger::DEBUG; }
  static int current_level() { return level; }
  static void set_current_level( int l ) { level = l; }
  static int level;

  enum Loglevel {
    EMERG=0,
    ALERT=1,
    CRIT=2,
    ERROR=3,
    WARN=4,
    NOTICE=5,
    INFO=6,
    DEBUG=7,
    TRACE=8
  };
};



#define LOG_EMERG(logname, fmt, ...) Logger::Logv(Logger::EMERG, logname, fmt, ##__VA_ARGS__)
#define LOG_ALERT(logname, fmt, ...) Logger::Logv(Logger::ALERT, logname, fmt, ##__VA_ARGS__)
#define LOG_CRIT(logname, fmt, ...) Logger::Logv(Logger::CRIT, logname, fmt, ##__VA_ARGS__)
#define LOG_ERROR(logname, fmt, ...) Logger::Logv(Logger::ERROR, logname, fmt, ##__VA_ARGS__)
#define LOG_WARN(logname, fmt, ...) Logger::Logv(Logger::WARN, logname, fmt, ##__VA_ARGS__)
#define LOG_NOTICE(logname, fmt, ...) Logger::Logv(Logger::NOTICE, logname, fmt, ##__VA_ARGS__)
#define LOG_INFO(logname, fmt, ...) Logger::Logv(Logger::INFO, logname, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(logname, fmt, ...) Logger::Logv(Logger::DEBUG, logname, fmt, ##__VA_ARGS__)
#define LOG_TRACE(logname, fmt, ...) Logger::Logv(Logger::TRACE, logname, fmt, ##__VA_ARGS__)

}  // namespace felixdb

#endif  // HERMESDB_LOGGER_H_
