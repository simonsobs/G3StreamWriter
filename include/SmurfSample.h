#ifndef SMURFSAMPLE_H
#define SMURFSAMPLE_H

#include <G3Frame.h>
#include <G3TimeStamp.h>
#include <vector>

#include <pybindings.h>
#include <serialization.h>

#include "smurf/core/common/SmurfPacket.h"

#define N_TES_BIAS 16



class StatusSample : public G3FrameObject{
public:
    StatusSample(): G3FrameObject(), Timestamp(0) {}
    StatusSample(G3Time time, std::string status) :
        G3FrameObject(), Timestamp(time), status_(status){}

    std::string status_;

    static void setup_python();

    G3Time Timestamp;
};

G3_POINTERS(StatusSample);

enum TimestampType {Timing_LowPrecision, Timing_HighPrecision};
static const char * TimestampTypeStrings[] = {"Low Precision", "High Precision"};

class SmurfSample : public G3FrameObject{
public:

    SmurfSample() : G3FrameObject(), time_(0), timing_type_(Timing_LowPrecision) {}
    SmurfSample(G3Time time, size_t nchannels, TimestampType timing_type);

    SmurfPacketRO::data_t *Channels() const;


    void setTESBias(size_t n, uint32_t value);
    uint32_t getTESBias (size_t n) const;

    const int NChannels() const;

    G3Time time_;
    TimestampType timing_type_;

    template <class A> void serialize(A &ar, unsigned v);

    static void setup_python();

private:
    std::vector<SmurfPacketRO::data_t> channels;
    std::vector<uint32_t> tes_biases;
};

G3_POINTERS(SmurfSample);
// MAKE SURE TO BUMP THIS IF ANYTHING CHANGES IN THE DATA INTERFACE
G3_SERIALIZABLE(SmurfSample, 2);

#endif
