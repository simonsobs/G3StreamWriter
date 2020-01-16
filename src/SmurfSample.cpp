#include <pybindings.h>
#include <serialization.h>

#include "SmurfSample.h"
#include <cereal/types/map.hpp>
#include <cereal/types/vector.hpp>

SmurfSample::SmurfSample(
        G3Time timestamp, size_t nchannels, TimestampType timing_type
    ) : G3FrameObject(),
        channels(std::vector<SmurfPacketRO::data_t>(nchannels)),
        tes_biases(std::vector<uint32_t>(16)),
        time_(timestamp), timing_type_(timing_type) {}

SmurfPacketRO::data_t* SmurfSample::Channels() const {
    return (SmurfPacketRO::data_t *) &channels[0];
}

void SmurfSample::setTESBias(size_t n, uint32_t value){
    tes_biases[n] = value;
}

uint32_t SmurfSample::getTESBias(size_t n) const{
    return tes_biases[n];
}

const int SmurfSample::NChannels() const {return channels.size();}

template <class A> void SmurfSample::serialize(A &ar, unsigned v){
    using namespace cereal;

    G3_CHECK_VERSION(v);

    ar & make_nvp("G3FrameObject", base_class<G3FrameObject>(this));
    ar & make_nvp("channels", channels);
    ar & make_nvp("tes_biases", tes_biases);
    ar & make_nvp("time", time_);
    ar & make_nvp("timing_type", timing_type_);
}

void StatusSample::setup_python(){}

void SmurfSample::setup_python(){
    namespace bp = boost::python;

    bp::class_< SmurfSample, bp::bases<G3FrameObject>,
                SmurfSamplePtr, boost::noncopyable>
                ("SmurfSample",
                "Samples from all readout_channels and TES Biases comming from "
                "a single SmurfPacket",
                bp::init<G3Time, size_t, TimestampType>(bp::args("time", "nchannels", "timing_type")))
        .def_readwrite("time", &SmurfSample::time_)
        .def_readwrite("timing_type", &SmurfSample::timing_type_)
        .def_pickle(g3frameobject_picklesuite<SmurfSample>())
    ;
    register_pointer_conversions<SmurfSample>();
}


// void AggregatedSmurfSample(G3Time time, size_t nchans) :
//     G3FrameObject()
//     {}
