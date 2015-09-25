#include "dependency_tracking.hh"

#include <iostream>

class StreamTracker {
private:
  vector<unsigned> key_frame_sizes_;
  unsigned num_key_frames_;
};

int main( int argc, char * argv[] )
{
  if ( argc < 3 ) {
    cerr << "Usage: " << argv[ 0 ] << " VIDEO_DIR OUTPUT\n";
    return EXIT_FAILURE;
  }

  ofstream stats_output( argv[ 2 ] );

  if ( chdir( argv[ 1 ] ) ) {
    throw unix_error( "chdir failed" );
  }

  ifstream frame_manifest( "frame_manifest" );
  ifstream stream_manifest( "stream_manifest" );


}
