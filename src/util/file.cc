#include <cassert>
#include <fcntl.h>
#include <sys/mman.h>

#include "file.hh"
#include "exception.hh"

using namespace std;

void File::map( const int mmap_flags )
{
  assert( buffer_ == nullptr );
  buffer_ = static_cast<uint8_t *>( mmap( nullptr, size_, mmap_flags, MAP_SHARED, fd_.num(), 0 ) );
  if ( buffer_ == MAP_FAILED ) {
    throw unix_error( "mmap" );
  }
  chunk_ = Chunk( buffer_, size_ );
}

File::File( const string & filename,
	    const int open_flags )
  : fd_( SystemCall( filename, open( filename.c_str(), open_flags,
				     S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH ) ) ),
    size_( fd_.size() )
{}

File::File( const string & filename )
  : File( filename, O_RDONLY )
{
  map( PROT_READ );
}

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

MutableFile::MutableFile( const string & filename )
  : File( filename, O_RDWR | O_APPEND | O_CREAT )
{
  if ( size_ ) {
    map( PROT_READ | PROT_WRITE );
  }
}

void MutableFile::append( const Chunk & new_chunk )
{
  if ( size_ == 0 ) {
    assert( buffer_ == nullptr );
  }

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

  if ( not buffer_ ) {
    size_ = new_size;
    map( PROT_READ | PROT_WRITE );
  } else {
    const uint8_t *new_buffer = static_cast<uint8_t *>( mremap( buffer_, size_, new_size, 0 ) );

    if ( new_buffer == MAP_FAILED ) {
      throw unix_error( "mremap" );
    } else if ( new_buffer != buffer_ ) {
      throw internal_error( "mremap", "unexpectedly changed mapped location" );
    }

    size_ = new_size;
    chunk_ = Chunk( buffer_, size_ );
  }
}
