#ifndef FILE_HH
#define FILE_HH

/* memory-mapped read-only file wrapper */

#include <string>

#include "file_descriptor.hh"
#include "chunk.hh"

class File
{
protected:
  FileDescriptor fd_;
  size_t size_;
  uint8_t * buffer_ { nullptr };
  Chunk chunk_ { nullptr, 0 };

  File( const std::string & filename,
	const int open_flags );

  void map( const int mmap_flags );
  
public:
  File( const std::string & filename );
  ~File();

  const Chunk & chunk( void ) const { return chunk_; }
  const Chunk operator() ( const uint64_t & offset, const uint64_t & length ) const
  {
    return chunk_( offset, length );
  }

  /* Disallow copying */
  File( const File & other ) = delete;
  File & operator=( const File & other ) = delete;

  /* Allow moving */
  File( File && other );
};

class MutableFile : public File
{
public:
  MutableFile( const std::string & filename );

  /* Allow appending */
  void append( const Chunk & new_chunk );

  uint8_t * mutable_buffer() { return buffer_; }
};

#endif /* FILE_HH */
