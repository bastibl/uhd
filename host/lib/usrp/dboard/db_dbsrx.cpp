//
// Copyright 2010 Ettus Research LLC
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

// No RX IO Pins Used

// RX IO Functions

#include "max2118_regs.hpp"
#include <uhd/utils/static.hpp>
#include <uhd/utils/assert.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/usrp/subdev_props.hpp>
#include <uhd/usrp/dboard_base.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <boost/math/special_functions/round.hpp>
#include <utility>

using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;

/***********************************************************************
 * The DBSRX constants
 **********************************************************************/
static const bool dbsrx_debug = true;

static const freq_range_t dbsrx_freq_range(0.8e9, 2.4e9);

static const freq_range_t dbsrx_pfd_freq_range(0.15e6, 2.01e6);

static const prop_names_t dbsrx_antennas = list_of("J3");

static const uhd::dict<std::string, gain_range_t> dbsrx_gain_ranges = map_list_of
    ("GC1", gain_range_t(0, 56, 0.5))
    ("GC2", gain_range_t(0, 24, 1))
;

/***********************************************************************
 * The DBSRX dboard class
 **********************************************************************/
class dbsrx : public rx_dboard_base{
public:
    dbsrx(ctor_args_t args, boost::uint8_t max2118_addr);
    ~dbsrx(void);

    void rx_get(const wax::obj &key, wax::obj &val);
    void rx_set(const wax::obj &key, const wax::obj &val);

private:
    double _lo_freq;
    float _bandwidth;
    uhd::dict<std::string, float> _gains;
    max2118_write_regs_t _max2118_write_regs;
    max2118_read_regs_t _max2118_read_regs;
    boost::uint8_t _max2118_addr; //0x67 or 0x65 depending on which side

    void set_lo_freq(double target_freq);
    void set_gain(float gain, const std::string &name);
    void set_bandwidth(float bandwidth);

    void send_reg(boost::uint8_t start_reg, boost::uint8_t stop_reg){
        start_reg = boost::uint8_t(std::clip(int(start_reg), 0x0, 0x5));
        stop_reg = boost::uint8_t(std::clip(int(stop_reg), 0x0, 0x5));

        for(boost::uint8_t start_addr=start_reg; start_addr <= stop_reg; start_addr += sizeof(boost::uint32_t) - 1){
            int num_bytes = int(stop_reg - start_addr + 1) > int(sizeof(boost::uint32_t)) - 1 ? sizeof(boost::uint32_t) - 1 : stop_reg - start_addr + 1;

            //create buffer for register data (+1 for start address)
            byte_vector_t regs_vector(num_bytes + 1);

            //first byte is the address of first register
            regs_vector[0] = start_addr;

            //get the register data
            for(int i=0; i<num_bytes; i++){
                regs_vector[1+i] = _max2118_write_regs.get_reg(start_addr+i);
                if(dbsrx_debug) std::cerr << boost::format(
                    "DBSRX: send reg 0x%02x, value 0x%04x, start_addr = 0x%04x, num_bytes %d"
                ) % int(start_addr+i) % int(regs_vector[1+i]) % int(start_addr) % num_bytes << std::endl;
            }

            //send the data
            this->get_iface()->write_i2c(
                _max2118_addr, regs_vector
            );
        }
    }

    void read_reg(boost::uint8_t start_reg, boost::uint8_t stop_reg){
        static const boost::uint8_t status_addr = 0x0;
        start_reg = boost::uint8_t(std::clip(int(start_reg), 0x0, 0x1));
        stop_reg = boost::uint8_t(std::clip(int(stop_reg), 0x0, 0x1));

        for(boost::uint8_t start_addr=start_reg; start_addr <= stop_reg; start_addr += sizeof(boost::uint32_t)){
            int num_bytes = int(stop_reg - start_addr + 1) > int(sizeof(boost::uint32_t)) ? sizeof(boost::uint32_t) : stop_reg - start_addr + 1;

            //create address to start reading register data
            byte_vector_t address_vector(1);
            address_vector[0] = start_addr;

            /*
            //send the address
            this->get_iface()->write_i2c(
                _max2118_addr, address_vector
            );
            */

            //create buffer for register data
            byte_vector_t regs_vector(num_bytes);

            //read from i2c
            regs_vector = this->get_iface()->read_i2c(
                _max2118_addr, num_bytes
            );

            for(boost::uint8_t i=0; i < num_bytes; i++){
                if (i + start_addr >= status_addr){
                    _max2118_read_regs.set_reg(i + start_addr, regs_vector[i]);
                    if(dbsrx_debug) std::cerr << boost::format(
                        "DBSRX: set reg 0x%02x, value 0x%04x"
                    ) % int(i + start_addr) % int(_max2118_read_regs.get_reg(i + start_addr)) << std::endl;
                }
                if(dbsrx_debug) std::cerr << boost::format(
                    "DBSRX: read reg 0x%02x, value 0x%04x, start_addr = 0x%04x, num_bytes %d"
                ) % int(start_addr+i) % int(regs_vector[i]) % int(start_addr) % num_bytes << std::endl;
            }
        }
    }

    /*!
     * Is the LO locked?
     * \return true for locked
     */
    bool get_locked(void){
        read_reg(0x0, 0x1);

        //mask and return lock detect
        bool locked = 5 >= _max2118_read_regs.adc && _max2118_read_regs.adc >= 2;

        if(dbsrx_debug) std::cerr << boost::format(
            "DBSRX: locked %d"
        ) % locked << std::endl;

        return locked;
    }

};

/***********************************************************************
 * Register the DBSRX dboard
 **********************************************************************/
// FIXME 0x67 is the default i2c address on USRP2
//       need to handle which side for USRP1 with different address
static dboard_base::sptr make_dbsrx(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new dbsrx(args, 0x67));
}

//FIXME different dbid for USRP1 also
UHD_STATIC_BLOCK(reg_dbsrx_dboard){
    //register the factory function for the rx dbid
    dboard_manager::register_dboard(0x000D, &make_dbsrx, "DBSRX");
}

/***********************************************************************
 * Structors
 **********************************************************************/
dbsrx::dbsrx(ctor_args_t args, boost::uint8_t max2118_addr) : rx_dboard_base(args){
    //enable only the clocks we need
    this->get_iface()->set_clock_enabled(dboard_iface::UNIT_RX, true);

    //set the gpio directions and atr controls (identically)
    this->get_iface()->set_pin_ctrl(dboard_iface::UNIT_RX, 0x0); // All unused in atr
    this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_RX, 0x0); // All Inputs

    //set the i2c address for the max2118
    _max2118_addr = max2118_addr;

    //send initial register settings
    this->send_reg(0x0, 0x5);

    //set defaults for LO, gains
    set_lo_freq(dbsrx_freq_range.min);
    BOOST_FOREACH(const std::string &name, dbsrx_gain_ranges.keys()){
        set_gain(dbsrx_gain_ranges[name].min, name);
    }

    set_bandwidth(22.27e6); // default bandwidth from datasheet

    get_locked();
}

dbsrx::~dbsrx(void){
}


/***********************************************************************
 * Tuning
 **********************************************************************/
void dbsrx::set_lo_freq(double target_freq){
    target_freq = std::clip(target_freq, dbsrx_freq_range.min, dbsrx_freq_range.max);

    double actual_freq=0.0, pfd_freq=0.0, ref_clock=0.0;
    int R=0, N=0, r=0;

    //choose refclock
    std::vector<double> clock_rates = this->get_iface()->get_clock_rates(dboard_iface::UNIT_RX);
    BOOST_FOREACH(ref_clock, std::reversed(std::sorted(clock_rates))){
        if (ref_clock != 4e6) continue;

        //choose R
        for(r = 0; r <= 6; r += 1) {
            //compute divider from setting
            R = pow(2, r+1);
            if (dbsrx_debug) std::cerr << boost::format("DBSRX R:%d\n") % R << std::endl;

            //compute PFD compare frequency = ref_clock/R
            pfd_freq = ref_clock / R;

            //constrain the PFD frequency to specified range
            if ((pfd_freq < dbsrx_pfd_freq_range.min) or (pfd_freq > dbsrx_pfd_freq_range.max)) continue;

            //compute N
            N = int(std::floor(target_freq/pfd_freq));

            //constrain N to specified range
            if ((N < 256) or (N > 32768)) continue;

            goto done_loop;
        }
    } 

    //Assert because we failed to find a suitable combination of ref_clock, R and N 
    UHD_ASSERT_THROW((pfd_freq < dbsrx_pfd_freq_range.min) or (pfd_freq > dbsrx_pfd_freq_range.max));
    UHD_ASSERT_THROW((N < 256) or (N > 32768));
    done_loop:

    //compute resulting output frequency
    actual_freq = pfd_freq * N;

    //apply ref_clock, R, and N settings
    this->get_iface()->set_clock_rate(dboard_iface::UNIT_RX, ref_clock);
    ref_clock = this->get_iface()->get_clock_rate(dboard_iface::UNIT_RX);
    _max2118_write_regs.r_divider = (max2118_write_regs_t::r_divider_t) r;
    _max2118_write_regs.set_n_divider(N);
    _max2118_write_regs.ade_vco_ade_read = max2118_write_regs_t::ADE_VCO_ADE_READ_ENABLED;
    send_reg(0x4,0x4);
    send_reg(0x0,0x2);
    
    //compute prescaler variables
    int scaler = actual_freq > 1125e6 ? 2 : 4;
    _max2118_write_regs.div2 = scaler == 4 ? max2118_write_regs_t::DIV2_DIV4 : max2118_write_regs_t::DIV2_DIV2;

    //compute vco frequency and select vco
    double vco_freq = actual_freq * scaler;
    int vco;
    if (vco_freq < 2433e6)
        vco = 0;
    else if (vco_freq < 2711e6)
        vco=1;
    else if (vco_freq < 3025e6)
        vco=2;
    else if (vco_freq < 3341e6)
        vco=3;
    else if (vco_freq < 3727e6)
        vco=4;
    else if (vco_freq < 4143e6)
        vco=5;
    else if (vco_freq < 4493e6)
        vco=6;
    else
        vco=7;

    //apply vco selection
    _max2118_write_regs.osc_band = vco;
    send_reg(0x2, 0x2);

    //check vtune for lock condition
    read_reg(0x0, 0x1);

    //if we are out of lock for chosen vco, change vco
    while ((_max2118_read_regs.adc == 0) or (_max2118_read_regs.adc == 7)){
        //vtune is too low, try lower frequency vco
        if (_max2118_read_regs.adc == 0){
            UHD_ASSERT_THROW(_max2118_write_regs.osc_band <= 0);
            _max2118_write_regs.osc_band -= 1;
        }

        //vtune is too high, try higher frequency vco
        if (_max2118_read_regs.adc == 7){
            UHD_ASSERT_THROW(_max2118_write_regs.osc_band >= 7);
            _max2118_write_regs.osc_band += 1;
        }

        //update vco selection and check vtune
        send_reg(0x2, 0x2);
        read_reg(0x0, 0x0);
    }
      
    //select charge pump bias current
    if (_max2118_read_regs.adc <= 2) _max2118_write_regs.cp_current = max2118_write_regs_t::CP_CURRENT_I_CP_100UA;
    else if (_max2118_read_regs.adc >= 5) _max2118_write_regs.cp_current = max2118_write_regs_t::CP_CURRENT_I_CP_400UA;
    else _max2118_write_regs.cp_current = max2118_write_regs_t::CP_CURRENT_I_CP_200UA;
    
    //update charge pump bias current setting
    send_reg(0x2, 0x2);

    //compute actual tuned frequency
    _lo_freq = this->get_iface()->get_clock_rate(dboard_iface::UNIT_RX) / pow(2,(1 + _max2118_write_regs.r_divider)) * _max2118_write_regs.get_n_divider();

    //debug output of calculated variables
    if (dbsrx_debug) std::cerr
        << boost::format("DBSRX tune:\n")
        << boost::format("    VCO=%d, CP=%d, PFD Freq=%fMHz\n") % int(_max2118_write_regs.osc_band) % _max2118_write_regs.cp_current % (pfd_freq/1e6)
        << boost::format("    R=%d, N=%f, scaler=%d, div2=%d\n") % R % N % scaler % int(_max2118_write_regs.div2)
        << boost::format("    Ref    Freq=%fMHz\n") % (ref_clock/1e6)
        << boost::format("    Target Freq=%fMHz\n") % (target_freq/1e6)
        << boost::format("    Actual Freq=%fMHz\n") % (_lo_freq/1e6)
        << std::endl;
}

/***********************************************************************
 * Gain Handling
 **********************************************************************/
/*!
 * Convert a requested gain for the GC2 vga into the integer register value.
 * The gain passed into the function will be set to the actual value.
 * \param gain the requested gain in dB
 * \return 5 bit the register value
 */
static int gain_to_gc2_vga_reg(float &gain){
    int reg = 0;
    gain = std::clip<float>(boost::math::iround(gain), dbsrx_gain_ranges["GC2"].min, dbsrx_gain_ranges["GC2"].max);

    // Half dB steps from 0-5dB, 1dB steps from 5-24dB
    if (gain < 5) {
        reg = boost::math::iround(31.0 - gain/0.5);
        gain = float(boost::math::iround(gain)) * 0.5;
    } else {
        reg = boost::math::iround(22.0 - (gain - 4.0));
        gain = float(boost::math::iround(gain));
    }

    if (dbsrx_debug) std::cerr << boost::format(
        "DBSRX GC2 Gain: %f dB, reg: %d"
    ) % gain % reg << std::endl;

    return reg;
}

/*!
 * Convert a requested gain for the GC1 rf vga into the dac_volts value.
 * The gain passed into the function will be set to the actual value.
 * \param gain the requested gain in dB
 * \return dac voltage value
 */
static float gain_to_gc1_rfvga_dac(float &gain){
    //clip the input
    gain = std::clip<float>(gain, dbsrx_gain_ranges["GC1"].min, dbsrx_gain_ranges["GC1"].max);

    //voltage level constants
    static const float max_volts = float(2.7), min_volts = float(1.2);
    static const float slope = (max_volts-min_volts)/dbsrx_gain_ranges["GC1"].max;

    //calculate the voltage for the aux dac
    float dac_volts = gain*slope + min_volts;

    if (dbsrx_debug) std::cerr << boost::format(
        "DBSRX GC1 Gain: %f dB, dac_volts: %f V"
    ) % gain % dac_volts << std::endl;

    //the actual gain setting
    gain = (dac_volts - min_volts)/slope;

    return dac_volts;
}

void dbsrx::set_gain(float gain, const std::string &name){
    assert_has(dbsrx_gain_ranges.keys(), name, "dbsrx gain name");
    if (name == "GC2"){
        _max2118_write_regs.gc2 = gain_to_gc2_vga_reg(gain);
        send_reg(0x5, 0x5);
    }
    else if(name == "GC1"){
        //write the new voltage to the aux dac
        this->get_iface()->write_aux_dac(dboard_iface::UNIT_RX, dboard_iface::AUX_DAC_A, gain_to_gc1_rfvga_dac(gain));
    }
    else UHD_THROW_INVALID_CODE_PATH();
    _gains[name] = gain;
}

/***********************************************************************
 * Bandwidth Handling
 **********************************************************************/
void dbsrx::set_bandwidth(float bandwidth){
    //clip the input
    bandwidth = std::clip<float>(bandwidth, 4e6, 33e6);
    
    //calculate ref_freq
    float ref_freq = this->get_iface()->get_clock_rate(dboard_iface::UNIT_RX);

    //FIXME this contraint needs to be in the set_freq and needs to assert if it can't hit the range
    //calculate acceptable m_divider for filter tuning
    int m = 1;
    while (ref_freq/m < 1e6 or ref_freq/m > 2.5e6){ m++; }
    _max2118_write_regs.m_divider = m;

    _bandwidth = float((ref_freq/1e6/_max2118_write_regs.m_divider)*(4+0.145*_max2118_write_regs.f_dac)*1e6);

    _max2118_write_regs.f_dac = int(((bandwidth*_max2118_write_regs.m_divider/ref_freq) - 4)/0.145);

    if (dbsrx_debug) std::cerr << boost::format(
        "DBSRX Filter Bandwidth: %f MHz, m: %d, f_dac: %d\n"
    ) % (_bandwidth/1e6) % m % int(_max2118_write_regs.f_dac) << std::endl;

    this->send_reg(0x3, 0x5);
}

/***********************************************************************
 * RX Get and Set
 **********************************************************************/
void dbsrx::rx_get(const wax::obj &key_, wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){
    case SUBDEV_PROP_NAME:
        val = get_rx_id().to_pp_string();
        return;

    case SUBDEV_PROP_OTHERS:
        val = prop_names_t(); //empty
        return;

    case SUBDEV_PROP_GAIN:
        assert_has(_gains.keys(), name, "dbsrx gain name");
        val = _gains[name];
        return;

    case SUBDEV_PROP_GAIN_RANGE:
        assert_has(dbsrx_gain_ranges.keys(), name, "dbsrx gain name");
        val = dbsrx_gain_ranges[name];
        return;

    case SUBDEV_PROP_GAIN_NAMES:
        val = prop_names_t(dbsrx_gain_ranges.keys());
        return;

    case SUBDEV_PROP_FREQ:
        val = _lo_freq;
        return;

    case SUBDEV_PROP_FREQ_RANGE:
        val = dbsrx_freq_range;
        return;

    case SUBDEV_PROP_ANTENNA:
        val = std::string("J3");
        return;

    case SUBDEV_PROP_ANTENNA_NAMES:
        val = dbsrx_antennas;
        return;

/*
    case SUBDEV_PROP_QUADRATURE:
        val = true;
        return;

    case SUBDEV_PROP_IQ_SWAPPED:
        val = false;
        return;

    case SUBDEV_PROP_SPECTRUM_INVERTED:
        val = false;
        return;
*/
    case SUBDEV_PROP_CONNECTION:
        val = SUBDEV_CONN_COMPLEX_IQ;
        return;

    case SUBDEV_PROP_USE_LO_OFFSET:
        val = false;
        return;

    case SUBDEV_PROP_LO_LOCKED:
        val = this->get_locked();
        return;

/*
    case SUBDEV_PROP_RSSI:
        val = this->get_rssi();
        return;
*/

    case SUBDEV_PROP_BANDWIDTH:
        val = _bandwidth;
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

void dbsrx::rx_set(const wax::obj &key_, const wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){

    case SUBDEV_PROP_FREQ:
        this->set_lo_freq(val.as<double>());
        return;

    case SUBDEV_PROP_GAIN:
        this->set_gain(val.as<float>(), name);
        return;

    case SUBDEV_PROP_BANDWIDTH:
        this->set_bandwidth(val.as<float>());
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}

