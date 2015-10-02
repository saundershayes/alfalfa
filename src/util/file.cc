#include <cassert>
#include <fcntl.h>
#include <sys/mman.h>

#include "file.hh"
#include "exception.hh"

using namespace std;

File::File( const string & filename,
	    const int open_flags )
  : fd_( SystemCall( filename, open( filename.c_str(), open_flags ) ) ),
    size_( fd_.size() ),
    buffer_( static_cast<uint8_t *>( mmap( nullptr, size_, PROT_READ, MAP_SHARED, fd_.num(), 0 ) ) ),
    chunk_( buffer_, size_ )
{
  if ( buffer_ == MAP_FAILED ) {
    throw unix_error( "mmap" );
  }
}

File::File( const string & filename )
  : File( filename, O_RDONLY )
{}

File::~File()
{
  if ( buffer_ ) { 
    SystemCall( "munmap", munmap( buffer_, size_ ) );
  }
}

File::File( File && other )
  : fd_( move( other.fd_ ) ),
    size_( other.size_ ),
    buffer_( other.buffer_ ),
    chunk_( move( other.chunk_ ) )
{
  other.buffer_ = nullptr;
}

AppendableFile::AppendableFile( const string & filename )
  : File( filename, O_RDWR | O_APPEND | O_CREAT )
{}

void AppendableFile::append( const Chunk & new_chunk )
{
  /* append to file */
  Chunk left_to_write = new_chunk;
  while ( left_to_write.size() > 0 ) {
    ssize_t bytes_written = SystemCall( "write", write( fd_.num(),
							new_chunk.buffer(),
							new_chunk.size() ) );
    if ( bytes_written == 0 ) {
      throw internal_error( "write", "returned 0" );
    }

    left_to_write = new_chunk( bytes_written );
  }

  /* remap */
  size_t new_size = size_ + new_chunk.size();
  assert( fd_.size() == new_size );

  const uint8_t *new_buffer = static_cast<uint8_t *>( mremap( buffer_, size_, new_size, 0 ) );

  if ( new_buffer == MAP_FAILED ) {
    throw unix_error( "mremap" );
  } else if ( new_buffer != buffer_ ) {
    throw internal_error( "mremap", "unexpectedly changed mapped location" );
  }

  size_ = new_size;

  chunk_ = Chunk( buffer_, size_ );
}
