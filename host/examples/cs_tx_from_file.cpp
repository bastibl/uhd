//
// Copyright 2012 André Puschmann <andre.puschmann@tu-ilmenau.de>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <uhd/utils/thread_priority.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <fstream>
#include <complex>
#include <csignal>
#include <uhd/types/tune_request.hpp>
#include <boost/filesystem.hpp>
#include <random>

using namespace std;
using namespace boost;
namespace po = boost::program_options;

static bool stop_signal_called = false;

void sig_int_handler(int){
    stop_signal_called = true;
    exit(1);
}

void recv_and_transmit(
    uhd::usrp::multi_usrp::sptr usrp,
    bool use_cs,
    std::string &file,
    double delay)
{

    std::cout << boost::format("Begin streaming now ..") << std::endl;
    uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    stream_cmd.stream_now = true;
    usrp->issue_stream_cmd(stream_cmd);


    // setup metadata
    uhd::tx_metadata_t md;
    md.start_of_burst = true;
    md.end_of_burst   = false;
    md.has_time_spec  = false;
    md.use_cs         = use_cs;
    md.sifs           = 3200;

    // create streamer
    uhd::stream_args_t stream_args("fc32", "sc16");
    stream_args.args["underrun_policy"] = "wait";
    uhd::tx_streamer::sptr tx_streamer = usrp->get_tx_stream(stream_args);

    // read data from file
    std::ifstream infile(file.c_str(), std::ifstream::binary);
    float tmp;
    std::vector<float> packet = std::vector<float>();
    while(1) {
       infile.read((char*)&tmp, sizeof(float));
       if(infile.eof()) {
           break;
       }
       assert(infile.gcount() == 4);
       assert(infile);
       packet.push_back(tmp);
    }
    size_t samples = packet.size();

    std::cout << "sending data / packet length: " << samples << std::endl;

    while(1) {
        unsigned int n = 0;
        md.start_of_burst = true;
        md.end_of_burst = false;
        md.use_cs = use_cs;
        md.backoffs[0] = rand() % 32;
        md.backoffs[1] = rand() % 64;
        tx_streamer->send(&packet.front() + n, samples/2, md, 3600);
        std::cout << "send: " << n << std::endl;

        // send eob packet
        md.start_of_burst = false;
        md.end_of_burst = true;
        md.use_cs = false;
        tx_streamer->send("", 0, md);

        uhd::async_metadata_t async_md;
        while(1) {

            if(usrp->get_device()->recv_async_msg(async_md,3600)) {
                switch(async_md.event_code) {

                case uhd::async_metadata_t::EVENT_CODE_BURST_ACK:
                    cout << "BURST ACK" << endl;
                    goto next;
                    break;
                case uhd::async_metadata_t::EVENT_CODE_UNDERFLOW:
                    cout << "UNDERFLOW" << endl;
                    break;
                case uhd::async_metadata_t::EVENT_CODE_SEQ_ERROR:
                    cout << "SEQ ERROR" << endl;
                    break;
                case uhd::async_metadata_t::EVENT_CODE_TIME_ERROR:
                    cout << "TIME ERROR" << endl;
                    break;
                case uhd::async_metadata_t::EVENT_CODE_UNDERFLOW_IN_PACKET:
                    cout << "UNDERFLOW IN PACKET" << endl;
                    break;
                case uhd::async_metadata_t::EVENT_CODE_SEQ_ERROR_IN_BURST:
                    cout << "SEQ ERROR IN BURST" << endl;
                    break;
                case uhd::async_metadata_t::EVENT_CODE_USER_PAYLOAD:
                    cout << "USER PAYLOAD" << endl;
                     break;
                default:
                    cout << "UNKOWN COMMAND!" << endl;
                    return;
                }

            } else {
                cout << "Metadata not valid!" << endl;
                return;
            }

        }
next:
        if(delay >=  0.0) boost::this_thread::sleep(boost::posix_time::milliseconds(delay));
    }
}


int UHD_SAFE_MAIN(int argc, char *argv[]){
    uhd::set_thread_priority_safe();

    // variables to be set by po
    bool        use_cs;
    double      rate, freq, rxGain, txGain, delay;
    uint16_t    slot_time, threshold;
    std::string args, wirefmt, file;


    // setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "single uhd device address args")

        // phy
        ("rate", po::value<double>(&rate)->default_value(2000e3), "rate of incoming samples")
        ("freq",po::value<double>(&freq)->default_value(2400e6),"Sets Center Frequency")
        ("rxgain",po::value<double>(&rxGain)->default_value(5),"Sets receive gain")
        ("txgain",po::value<double>(&txGain)->default_value(32),"Sets transmit gain")

        // csma
        ("use-cs", po::value<bool>(&use_cs)->default_value(true), "enable or disable usrp carrier sense")
        ("slot-time", po::value<uint16_t>(&slot_time)->default_value(10000), "slot-time for usrp carrier sense mechanism")
        ("cs-hw-threshold", po::value<uint16_t>(&threshold)->default_value(50), "threshold for usrp carrier sense")

        // file
        ("file", po::value<std::string>(&file)->default_value("usrp_samples.dat"), "name of the file to read binary samples from")
        ("delay", po::value<double>(&delay)->default_value(0.0), "specify a delay between repeated transmission of file")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // print the help message
    if (vm.count("help")){
        std::cout << boost::format("Carrier Sense->Transmit Delay Benchmark %s") % desc << std::endl;
        return 1;
    }

    // create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);
    std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;

    // setup RX
    std::cout << boost::format("Setting RX Center Frequency: %f") % freq << std::endl;
    usrp->set_rx_freq(freq);
    std::cout << boost::format("Setting RX Gain %f") % rxGain << std::endl;
    usrp->set_rx_gain(rxGain);
    std::cout << boost::format("Setting RX Rate: %f Msps...") % (rate/1e6) << std::endl;
    usrp->set_rx_rate(rate);
    std::cout << boost::format("Actual RX Rate: %f Msps...") % (usrp->get_rx_rate()/1e6) << std::endl << std::endl;

    // setup TX
    std::cout << boost::format("Setting TX Center Frequency: %f") % freq << std::endl;
    usrp->set_tx_freq(freq);
    std::cout << boost::format("Setting TX Gain %f") % txGain << std::endl;
    usrp->set_tx_gain(txGain);
    std::cout << boost::format("Setting TX Rate: %f Msps...") % (rate/1e6) << std::endl;
    usrp->set_tx_rate(rate);
    std::cout << boost::format("Actual TX Rate: %f Msps...") % (usrp->get_tx_rate()/1e6) << std::endl << std::endl;

    std::cout << boost::format("Setting device timestamp to 0...") << std::endl;
    usrp->set_time_now(uhd::time_spec_t(0.0));

    // setup CSMA
    usrp->set_csma_slottime(slot_time);
    usrp->set_csma_threshold(threshold);

    // send it (this call blocks)
    std::signal(SIGINT, &sig_int_handler);
    std::cout << "Press Ctrl + C to stop measurement ..." << std::endl;
    recv_and_transmit(usrp, use_cs, file, delay);

    // finished
    boost::this_thread::sleep(boost::posix_time::seconds(1));
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return 0;
}
