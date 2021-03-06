// use direct path for to save compile flags
#define _CRT_SECURE_NO_WARNINGS
#include <cstring>
#include <dmlc/io.h>
#include <dmlc/logging.h>
#include "io/line_split.h"
#include "io/recordio_split.h"
#include "io/single_file_split.h"
#include "io/filesys.h"
#include "io/local_filesys.h"
#include "io/threaded_input_split.h"

#if DMLC_USE_HDFS
#include "io/hdfs_filesys.h"
#endif

#if DMLC_USE_S3
#include "io/s3_filesys.h"
#endif

namespace dmlc {
namespace io {
FileSystem *FileSystem::GetInstance(const std::string &protocol) {
  if (protocol == "file://" || protocol.length() == 0) {
    return LocalFileSystem::GetInstance();
  }
  if (protocol == "hdfs://") {
#if DMLC_USE_HDFS
    return HDFSFileSystem::GetInstance();
#else
    LOG(FATAL) << "Please compile with DMLC_USE_HDFS=1 to use hdfs";
#endif
  }
  if (protocol == "s3://" || protocol == "http://" || protocol == "https://") {
#if DMLC_USE_S3
    return S3FileSystem::GetInstance();
#else
    LOG(FATAL) << "Please compile with DMLC_USE_S3=1 to use S3";
#endif
  }
  LOG(FATAL) << "unknown filesystem protocol " + protocol;
  return NULL;
}
} // namespace io

InputSplit* InputSplit::Create(const char *uri,
                               unsigned part,
                               unsigned nsplit,
                               const char *type) {
  using namespace std;
  using namespace dmlc::io;
  if (!strcmp(uri, "stdin")) {
    return new SingleFileSplit(uri);
  }
  CHECK(part < nsplit) << "invalid input parameter for InputSplit::Create";
  URI path(uri);
  InputSplitBase *split = NULL;
  if (!strcmp(type, "text")) {
    split =  new LineSplitter(FileSystem::GetInstance(path.protocol),
                            uri, part, nsplit);
  } else if (!strcmp(type, "recordio")) {
    split =  new RecordIOSplitter(FileSystem::GetInstance(path.protocol),
                                  uri, part, nsplit);
  } else {
    LOG(FATAL) << "unknown input split type " << type;
  }
#if DMLC_USE_CXX11
  return new ThreadedInputSplit(split);
#else
  return split;
#endif
}

Stream *Stream::Create(const char *uri,
                       const char * const flag,
                       bool try_create) {
  io::URI path(uri);
  return io::FileSystem::
      GetInstance(path.protocol)->Open(path, flag, try_create);
}

SeekStream *SeekStream::CreateForRead(const char *uri, bool try_create) {
  io::URI path(uri);
  return io::FileSystem::
      GetInstance(path.protocol)->OpenForRead(path, try_create);
}
}  // namespace dmlc
