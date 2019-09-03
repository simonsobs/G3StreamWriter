#ifndef _SMURFSTREAMER_H
#define _SMURFSTREAMER_H

#include <G3Frame.h>
#include <G3Timestream.h>

#include <string>
#include <mutex>
#include <random>
#include <thread>

#include <smurf_processor.h>

#include "SampleData.h"
#include "StreamConfig.h"

namespace ris = rogue::interfaces::stream;
namespace bp = boost::python;

/*
 * This module sends downsampled G3 frames with time-ordered-data over TCP using
 * the G3NetworkSender. The SmurfProcessor object receives frames from ROGUE
 * and downsamples them, passing the downsampled frames to the transmit method.
 */

class SmurfStreamer: public SmurfProcessor{
public:

    SmurfStreamer(std::string config_file);

    // Called by SmurfProcessor with downsampled data packet
    void transmit(SmurfPacket_RO packet);

    // Reads config file into config struct
    void read_config(std::string filename);
    // Called whenever frame is passed from master
    // void acceptFrame ( ris::FramePtr frame );

    // Streams data over G3Network
    void run();
    void stop();
    bool running;

    StreamConfig config;

    SampleBuffer sample_buffer;

    std::thread run_thread;

    float frame_time;

    G3IntPtr frame_num;
    G3IntPtr session_id;
    G3VectorStringPtr chan_keys;
    G3TimestreamPtr timestreams[smurfsamples];
    G3TimestreamMapPtr ts_map;

    G3NetworkSender writer;
};
#endif