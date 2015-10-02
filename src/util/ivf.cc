#include <stdexcept>
#include <cstring>

#include "ivf.hh"
#include "file.hh"
#include "safe_array.hh"

using namespace std;

template <typename FileType>
GenericIVF<FileType>::GenericIVF( const string & filename )
try :
  file_( filename ),
    header_( file_( 0, supported_header_len ) ),
    fourcc_( header_( 8, 4 ).to_string() ),
    width_( header_( 12, 2 ).le16() ),
    height_( header_( 14, 2 ).le16() ),
    frame_rate_( header_( 16, 4 ).le32() ),
    time_scale_( header_( 20, 4 ).le32() ),
    frame_count_( header_( 24, 4 ).le32() ),
    frame_index_()
      {
	if ( header_( 0, 4 ).to_string() != "DKIF" ) {
	  throw Invalid( "missing IVF file header" );
	}

	if ( header_( 4, 2 ).le16() != 0 ) {
	  throw Unsupported( "not an IVF version 0 file" );
	}

	if ( header_( 6, 2 ).le16() != supported_header_len ) {
	  throw Unsupported( "unsupported IVF header length" );
	}

	/* build the index */
	frame_index_.reserve( frame_count_ );

	uint64_t position = supported_header_len;
	for ( uint32_t i = 0; i < frame_count_; i++ ) {
	  Chunk frame_header = file_( position, frame_header_len );
	  const uint32_t frame_len = frame_header.le32();

	  frame_index_.emplace_back( position + frame_header_len, frame_len );
	  position += frame_header_len + frame_len;
	}
      }
catch ( const out_of_range & e )
  {
    throw Invalid( "IVF file truncated" );
  }

template <typename FileType>
Chunk GenericIVF<FileType>::frame( const uint32_t & index ) const
{
  const auto & entry = frame_index_.at( index );
  return file_( entry.first, entry.second );
}

template class GenericIVF<File>;

template <typename T> void zero( T & x ) { memset( &x, 0, sizeof( x ) ); }

static void memcpy_le16( uint8_t * dest, const uint16_t val )
{
  uint16_t swizzled = htole16( val );
  memcpy( dest, &swizzled, sizeof( swizzled ) );
}

static void memcpy_le32( uint8_t * dest, const uint32_t val )
{
  uint32_t swizzled = htole32( val );
  memcpy( dest, &swizzled, sizeof( swizzled ) );
}

template <>
GenericIVF<MutableFile>::GenericIVF( const string & filename,
				     const string & s_fourcc,
				     const uint16_t s_width,
				     const uint16_t s_height,
				     const uint32_t s_frame_rate,
				     const uint32_t s_time_scale )
  : file_( filename ),
    header_( nullptr, 0 ),
    fourcc_( s_fourcc ),
    width_( s_width ),
    height_( s_height ),
    frame_rate_( s_frame_rate ),
    time_scale_( s_time_scale ),
    frame_count_( 0 ),
    frame_index_()
{
  if ( s_fourcc.size() != 4 ) {
    throw internal_error( "IVF", "FourCC must be four bytes long" );
  }

  /* build the header */
  SafeArray<uint8_t, supported_header_len> new_header;
  zero( new_header );

  memcpy( &new_header.at( 0 ), "DKIF", 4 );
  /* skip version number (= 0) */
  memcpy_le16( &new_header.at( 6 ), supported_header_len );
  memcpy( &new_header.at( 8 ), fourcc_.data(), 4 );
  memcpy_le16( &new_header.at( 12 ), width_ );
  memcpy_le16( &new_header.at( 14 ), height_ );
  memcpy_le32( &new_header.at( 16 ), frame_rate_ );
  memcpy_le32( &new_header.at( 20 ), time_scale_ );

  /* write the header */
  file_.append( Chunk( &new_header.at( 0 ), new_header.size() ) );

  header_ = file_( 0, supported_header_len );
}

template <>
void GenericIVF<MutableFile>::append_frame( const Chunk & chunk )
{
  /* verify the existing frame count */
  assert( frame_count_ == header_( 24, 4 ).le32() );

  /* get current position in the file */
  uint64_t position = file_.chunk().size();
  
  /* append the frame to the file */
  file_.append( chunk );

  /* increment the frame count in the file's header */
  frame_count_++;
  memcpy_le32( file_.mutable_buffer() + 24, frame_count_ );

  /* verify the new frame count */
  assert( frame_count_ == header_( 24, 4 ).le32() );

  /* add an entry in the index */
  const uint32_t frame_len = chunk.le32();
  frame_index_.emplace_back( position + frame_header_len, frame_len );
}
