#include "dependency_tracking.hh"
#include "exception.hh"

#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cstdlib>
#include <unistd.h>

using namespace std;

class StreamTracker {
private:
  vector<unsigned> key_frame_sizes_;
  vector<unsigned> key_frame_nums_;
  unsigned num_key_frames_;
  unsigned total_frames_;
  unsigned total_displayed_frames_;
  unsigned frames_per_second_;

  vector<string> frames_;

public:
  StreamTracker( const vector<string> & frames, const unordered_map<string, unsigned> & frame_sizes )
    : key_frame_sizes_(),
      key_frame_nums_(),
      num_key_frames_( 0 ),
      total_frames_( frames.size() ),
      total_displayed_frames_( 0 ),
      frames_per_second_( 24 ),
      frames_( frames )
  {
    for ( const string & frame_name : frames_ ) {
      TargetHash target_component( frame_name );
      if ( target_component.shown ) {
        SourceHash source_component( frame_name );
        total_displayed_frames_++;

        // Is this a keyframe
        if ( not source_component.state_hash.initialized() and not source_component.continuation_hash.initialized() and
             not source_component.last_hash.initialized() and not source_component.golden_hash.initialized() and
             not source_component.alt_hash.initialized() ) {
          num_key_frames_++;
          key_frame_sizes_.push_back( frame_sizes.find( frame_name )->second );
          key_frame_nums_.push_back( total_displayed_frames_ );
        }
      }
    }
  }

  unsigned total_frames( void ) const { return total_frames_; }

  double total_time( void ) const
  {
    return (double)total_displayed_frames_ / frames_per_second_;
  }

  double mean_keyframe_interval( void ) const
  {
    vector<unsigned> keyframe_intervals;

    for ( unsigned i = 1; i < key_frame_nums_.size(); i++ ) {
      keyframe_intervals.push_back( key_frame_nums_[ i ] - key_frame_nums_[ i - 1 ] );
    }
    assert( keyframe_intervals.size() == num_key_frames_ - 1 );

    double total_size = accumulate( keyframe_intervals.begin(), keyframe_intervals.end(), 0 );

    return total_size / keyframe_intervals.size() / frames_per_second_ ;
  }

  double mean_keyframe_size( void ) const
  {
    unsigned total_size = 0;
    for ( unsigned frame_size : key_frame_sizes_ ) {
      total_size += frame_size;
    }

    return (double)total_size / num_key_frames_;
  }

  double median_keyframe_size( void ) const
  {
    vector<unsigned> frame_sizes( key_frame_sizes_ );
    sort( frame_sizes.begin(), frame_sizes.end() );

    if ( num_key_frames_ % 2 == 0 ) {
      return (double)( frame_sizes[ num_key_frames_ / 2 - 1 ] + frame_sizes[ num_key_frames_ / 2 ] ) / 2;
    }
    else {
      return frame_sizes[ num_key_frames_ / 2 ];
    }
  }
};

int main( int argc, char * argv[] )
{
  if ( argc < 2 ) {
    cerr << "Usage: " << argv[ 0 ] << " VIDEO_DIR\n";
    return EXIT_FAILURE;
  }

  if ( chdir( argv[ 1 ] ) ) {
    throw unix_error( "chdir failed" );
  }

  ifstream frame_manifest( "frame_manifest" );
  ifstream stream_manifest( "stream_manifest" );

  unordered_map<string, unsigned> frame_sizes;
  while ( not frame_manifest.eof() ) {
    string frame_name;
    unsigned frame_size;
    frame_manifest >> frame_name >> frame_size;
    if ( frame_name == "" ) {
      break;
    }

    frame_sizes[ frame_name ] = frame_size;
  }

  vector<vector<string>> stream_frames;
  while ( not stream_manifest.eof() ) {
    unsigned stream_idx;
    string frame_name;
    stream_manifest >> stream_idx >> frame_name;
    if ( frame_name == "" ) {
      break;
    }

    if ( stream_idx >= stream_frames.size() ) {
      stream_frames.resize( stream_idx + 1 );
    }
    stream_frames[ stream_idx ].push_back( frame_name );
 }

  vector<StreamTracker> streams;
  for ( auto & frames : stream_frames ) {
    streams.emplace_back( frames, frame_sizes );
  }

  for ( unsigned stream_idx = 0; stream_idx < streams.size(); stream_idx++ ) {
    cout << "Stream " << stream_idx << ":\n";

    const StreamTracker & stream = streams[ stream_idx ];

    cout << "Length: " << stream.total_time() << " seconds\n";
    cout << "Total frames: " << stream.total_frames() << "\n";
    cout << "KeyFrame frequency: " << stream.mean_keyframe_interval() << " seconds between keyframes\n";
    cout << "KeyFrame median size: " << stream.median_keyframe_size() << " bytes\n";
    cout << "KeyFrame mean size: " << stream.mean_keyframe_size() << " bytes\n";

    cout << endl;
  }
}
