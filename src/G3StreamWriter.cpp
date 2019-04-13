#include <rogue/interfaces/stream/Slave.h>
#include <rogue/interfaces/stream/Frame.h>
#include <rogue/interfaces/stream/FrameIterator.h>
#include <rogue/GilRelease.h>

#include <smurf_processor.h>

#include <boost/python.hpp>
#include <boost/python/module.hpp>

#include <G3Frame.h>
#include <G3Writer.h>
#include <G3Data.h>
#include <G3Vector.h>
#include <G3Timestream.h>
#include <G3TimeStamp.h>
#include <G3Units.h>
#include <G3NetworkSender.h>

#include <string>
#include <math.h>
#include <thread>

#include "SampleData.h"
#include "G3StreamWriter.h"


namespace ris = rogue::interfaces::stream;
namespace bp = boost::python;

// Maps from raw data to int to +/- pi
double toPhase(int32_t phase_int){return phase_int * M_PI / (1 << 15);}

/*
    Params:
        int port: port that G3Frames will be written. Defalts 4536
        int frame_time: Time per G3Frame (seconds)
        int max_queue_size: Max queue size for G3NetworkStreamer
        int sample_buff_size: Size of Frame buffer that stores data. Defaults to
                                10000, but can be decreasod once we start getting
                                downsampled dat.
*/
G3StreamWriter::G3StreamWriter(int port, float frame_time, int max_queue_size):
        SmurfProcessor(),
        frame_time(frame_time),
        ts_map(new G3TimestreamMap),
        chan_keys(new G3VectorString(smurfsamples)),
        writer(new G3NetworkSender("*", port, max_queue_size)),
        sample_buffer(),
        run_thread(&G3StreamWriter::run, this),
        frame_num(new G3Int(0)), running(true)
{
    G3TimePtr session_start_time = G3TimePtr(new G3Time(G3Time::Now()));
    session_id = G3IntPtr(new G3Int(std::hash<G3Time*>()(session_start_time.get())));

    // Writes Session frame
    G3FramePtr f(new G3Frame(G3Frame::Observation));
    std::deque<G3FramePtr> junk;
    f->Put("session_id", session_id);
    f->Put("session_start_time", session_start_time);
    writer->Process(f, junk);

    for (int i = 0; i < smurfsamples; i++){
        timestreams[i] = G3TimestreamPtr(new G3Timestream());
        (*chan_keys)[i] = std::to_string(i);
        ts_map->insert(std::make_pair((*chan_keys)[i], timestreams[i]));
    }
}

/*
    Loops and writes G3Frames every so often. Should be run in separate thread.
*/
void G3StreamWriter::run(){
    rogue::GilRelease noGil;

    printf("Starting run thread for G3Streamer\n");
    running = true;

    while (running){
        usleep(frame_time * 1000000);

        // Swaps buffers so that we can continue accepting new frames.
        int nsamples = sample_buffer.swap();

        if (nsamples == 0)
            continue;

        // Reads sample data into TimestreamMap
        for (int i = 0; i < smurfsamples; i++)
            timestreams[i]->resize(nsamples);

        SampleDataPtr x;
        for (int i = 0; i < nsamples; i ++){
            x = sample_buffer.read_buffer[i];

            for (int j = 0; j < smurfsamples; j++){
                if (i == 0)
                    timestreams[j]->start = x->timestamp;
                (*timestreams[j])[i] = x->data[j];
                // (*timestreams[j])[i] = toPhase(x->data[j]);
            }
        }
        for (int i = 0; i < smurfsamples; i++)
            timestreams[i]->stop = x->timestamp;

        G3FramePtr f(new G3Frame(G3Frame::Scan));
        std::deque<G3FramePtr> junk;
        f->Put("keys", chan_keys);
        f->Put("data", ts_map);
        f->Put("frame_num", frame_num);
        f->Put("session_id", session_id);

        printf("Writing %lu samples @ %.2f Hz\n", ts_map->NSamples(), ts_map->GetSampleRate() / G3Units::Hz);
        writer->Process(f, junk);
        frame_num->value+=1;
    }
}

void G3StreamWriter::stop(){
    printf("Stopping stream...\n");
    running = false;
    run_thread.join();
    printf("Stopped Stream.\n");
}


void G3StreamWriter::transmit(smurf_tx_data_t* data){
    if (!running)
        return;

    SampleDataPtr sample(new SampleData(data));
    sample_buffer.write(sample);
};

boost::shared_ptr<G3StreamWriter> G3StreamWriterInit(
        int port=4536, float frame_time=1, int max_queue_size=100
    ){
        boost::shared_ptr<G3StreamWriter> writer(new G3StreamWriter(port, frame_time, max_queue_size));
        return writer;
}

BOOST_PYTHON_MODULE(G3StreamWriter) {
    PyEval_InitThreads();
    try {
        bp::class_<G3StreamWriter, boost::shared_ptr<G3StreamWriter>,
                    bp::bases<ris::Slave>, boost::noncopyable >("G3StreamWriter",
                    bp::no_init)
        .def("__init__", bp::make_constructor(
            &G3StreamWriterInit, bp::default_call_policies(), (
                bp::arg("port")=4536,
                bp::arg("frame_time")=1.0,
                bp::arg("max_queue_size")=100
            )
        ))
        .def("stop", &G3StreamWriter::stop)
        .def("printTransmitStatistic", &G3StreamWriter::printTransmitStatistic)
        .def("setDebug",  &G3StreamWriter::setDebug)
        ;
        bp::implicitly_convertible<boost::shared_ptr<G3StreamWriter>, ris::SlavePtr>();
    } catch (...) {
        printf("Failed to load module. import rogue first\n");
    }
    printf("Loaded my module\n");
};
