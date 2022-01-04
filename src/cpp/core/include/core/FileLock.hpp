/*
 * FileLock.hpp
 *
 * Copyright (C) 2022 by RStudio, PBC
 *
 * Unless you have received this program directly from RStudio pursuant
 * to the terms of a commercial license agreement with RStudio, then
 * this program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */

#ifndef CORE_FILE_LOCK_HPP
#define CORE_FILE_LOCK_HPP

// inclusion of boost/asio requires us to include
// winsock2.h beforehand on windows
#ifdef _WIN32
# include <winsock2.h>
#endif

#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/asio.hpp>

#include <core/Log.hpp>
#include <core/Settings.hpp>

// env var to enable distributed locking mode
#define kRStudioDistributedLockingEnabled "RSTUDIO_DISTRIBUTED_LOCKING_ENABLED"

namespace rstudio {
namespace core {

class Error;
class FilePath;

class FileLock : boost::noncopyable
{
public:
   
   enum LockType { LOCKTYPE_ADVISORY, LOCKTYPE_LINKBASED };
   
   // initialize (read configuration)
   static void initialize();
   static void initialize(FileLock::LockType fallbackLockType);
   
   // clean up
   static void cleanUp();
   
   // control what kind of locks are generated by 'create()' method
   static boost::shared_ptr<FileLock> create(LockType type = s_defaultType);
   static boost::shared_ptr<FileLock> createDefault();
   
   // refreshes all FileLock implementations
   static void refresh();
   static void refreshPeriodically(boost::asio::io_service& service,
                                   boost::posix_time::seconds interval = s_refreshRate);
   
   // sub-classes implement locking semantics
   virtual Error acquire(const FilePath& lockFilePath) = 0;
   virtual Error release() = 0;
   virtual FilePath lockFilePath() const = 0;
   
   // NOTE: 'isLocked()' does not ask whether _this lock_ has the lock; rather,
   // whether _any lock_ has the lock. it's implemented as a virtual member function
   // to allow for polymorphism (ie, select method at runtime)
   virtual bool isLocked(const FilePath& lockFilePath) const = 0;
   
   // warns if FileLock::initialize() hasn't been called yet
   static bool verifyInitialized();
   
   FileLock()
   {
      verifyInitialized();
   }

   virtual ~FileLock() {}
   
public:
   static void log(const std::string& message);
   
   static boost::posix_time::seconds getTimeoutInterval() { return s_timeoutInterval; }
   static void setTimeoutInterval(boost::posix_time::seconds interval) { s_timeoutInterval = interval; }

   // getters only: set through 'initialize' method
   static LockType getDefaultType() { return s_defaultType; }
   static boost::posix_time::seconds getRefreshRate() { return s_refreshRate; }
   static bool isLoggingEnabled() { return s_loggingEnabled; }
   static bool isLoadBalanced() { return s_isLoadBalanced; }
   static bool isNoLockAvailable(const Error& error)
   {
      return error == systemError(boost::system::errc::no_lock_available, ErrorLocation());
   }
   
protected:
   static LockType s_defaultType;
   static boost::posix_time::seconds s_timeoutInterval;
   static boost::posix_time::seconds s_refreshRate;
   static bool s_loggingEnabled;
   static bool s_isLoadBalanced;
   static FilePath s_logFile;
};

class AdvisoryFileLock : public FileLock
{
public:
   static void refresh();
   static void cleanUp();
   
   Error acquire(const FilePath& lockFilePath);
   Error release();
   bool isLocked(const FilePath& lockFilePath) const;
   FilePath lockFilePath() const;
   
   AdvisoryFileLock();
   ~AdvisoryFileLock();
   
private:
   struct Impl;
   boost::scoped_ptr<Impl> pImpl_;
};

class LinkBasedFileLock : public FileLock
{
public:
   static bool isLockFileStale(const FilePath& lockFilePath);
   static void refresh();
   static void cleanUp();
   
   Error acquire(const FilePath& lockFilePath);
   Error release();
   bool isLocked(const FilePath& lockFilePath) const;
   FilePath lockFilePath() const;
   
   LinkBasedFileLock();
   ~LinkBasedFileLock();
   
private:
   struct Impl;
   boost::scoped_ptr<Impl> pImpl_;
};

// ScopedFileLock for acquiring a lock on construction
// and ensuring the lock is released on destruction
class ScopedFileLock : boost::noncopyable
{
public:
   ScopedFileLock(const boost::shared_ptr<FileLock>& fileLock,
                  const FilePath& filePath) :
                  fileLock_(fileLock)
   {
      error_ = fileLock_->acquire(filePath);
   }

   ~ScopedFileLock()
   {
      if (!error_)
      {
         error_ = fileLock_->release();
         if (error_)
            LOG_ERROR(error_);
      }
   }

   Error error()
   {
      return error_;
   }

private:
   Error error_;
   boost::shared_ptr<FileLock> fileLock_;
};

} // namespace core
} // namespace rstudio


#endif // CORE_FILE_LOCK_HPP
