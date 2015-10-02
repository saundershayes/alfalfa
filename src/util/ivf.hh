#ifndef IVF_HH
#define IVF_HH

#include <string>
#include <cstdint>
#include <vector>

#include "file.hh"

template <typename FileType>
class GenericIVF
{
protected:
  static constexpr int supported_header_len = 32;
  static constexpr int frame_header_len = 12;

  FileType file_;
  Chunk header_;

  std::string fourcc_;
  uint16_t width_, height_;
  uint32_t frame_rate_, time_scale_, frame_count_;

  std::vector< std::pair<uint64_t, uint32_t> > frame_index_;

public:
  GenericIVF( const std::string & filename );

  GenericIVF( const std::string & filename,
	      const std::string & s_fourcc,
	      const uint16_t s_width,
	      const uint16_t s_height,
	      const uint32_t s_frame_rate,
	      const uint32_t s_time_scale );

  void append_frame( const Chunk & chunk );
  
  const std::string & fourcc( void ) const { return fourcc_; }
  uint16_t width( void ) const { return width_; }
  uint16_t height( void ) const { return height_; }
  uint32_t frame_rate( void ) const { return frame_rate_; }
  uint32_t time_scale( void ) const { return time_scale_; }
  uint32_t frame_count( void ) const { return frame_count_; }

  Chunk frame( const uint32_t & index ) const;
};

using IVF = GenericIVF<File>;

#endif /* IVF_HH */
