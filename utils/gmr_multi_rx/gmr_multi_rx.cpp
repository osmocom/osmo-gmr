/* (C) 2011 by Dimitri Stolnikov <horiz0n@gmx.net>
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*

  build:

  "make" for USRP, requires libgnuradio-usrp
  "make TARGET=fcd" for FCD, requires libgnuradio-fcd
  "make TARGET=uhd" for UHD, requires libgnuradio-uhd

  usage:

  for FunCube Dongle call:

  ./gmr_multi_rx --gain 30 --gmr1-dl 941 942 943

  - when receiving 3 consecutive channels, the center channel will be
    distorted by the center peak caused by dc offset / iq imbalance,

  - when receiving 2 channels, each channel will have a small contribution
    of the center peak on the right or left side.

  - best results may be achieved when receiving only one channel.

  for default USRP clock, call:

  - only 75% of the master output rate will be used on the usrp, because of
    insufficient attenuation of the fpga channelizer at filter edges.

  ./gmr_multi_rx --gain 45 --gmr1-dl 941 942 943

  for modified clocks, use --mcr to tell custom fpga frequency in Hz

  ./gmr_multi_rx --gain 45 --gmr1-dl 941 942 943 --mcr 52e6

  mcr of 59.904e6 Hz is gmr1-friendly, thus alowing to save on interpolation

  ./gmr_multi_rx --gain 45 --gmr1-dl 941 942 943 --mcr 59.904e6

 */

#include <cstring>
#include <csignal>

#include <map>
#include <iostream>
#include <algorithm>
#include <iterator>

#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>

#include <gr_top_block.h>

#ifdef HAVE_FCD
#include <fcd/fcd_source_c.h>
#elif defined(HAVE_UHD)
#include <gr_uhd_usrp_source.h>
#else
#include <usrp_source_c.h>
#endif

#include <gr_firdes.h>
#include <gr_freq_xlating_fir_filter_ccf.h>
#include <gr_rational_resampler_base_ccf.h>

#include <gr_file_sink.h>
#include <gr_udp_sink.h>

#include "gmr_channels.hpp"
#include "filter_helpers.hpp"

namespace po = boost::program_options;

////////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
    /* variables to be set by po */
    std::string prefix, udp, ant, side;
#ifdef HAVE_FCD
    std::string device;
    double correct;
#else
#ifdef HAVE_UHD
    std::string addr, subdev;
#endif
#endif
    double gain, mcr;
    unsigned int time, osr;

    /* setup the program options */
    po::options_description opt_desc("Allowed options");
    opt_desc.add_options()
        ("help,h", "print the command line help")
        ("prefix,P", po::value<std::string>(&prefix)->default_value("/tmp/"), "prefix of file to write channel samples to")
        ("time,T", po::value<unsigned int>(&time)->default_value(0), "recording time in seconds, 0 means infinite")
        ("udp,u", po::value<std::string>(&udp), "UDP destination (host:port) to send radio samples to")
        ("gain,g", po::value<double>(&gain)->default_value(10), "gain for the RF chain")
        ("osr,s", po::value<unsigned int>(&osr)->default_value(4), "oversampling rate, samples per symbol")
#ifdef HAVE_FCD
        ("device", po::value<std::string>(&device)->default_value("hw:1"), "FCD audio device name")
        ("correct", po::value<double>(&correct)->default_value(-21), "FCD frequency correction (ppm)")
#else
#ifdef HAVE_UHD
        ("addr", po::value<std::string>(&addr)->default_value("type=usrp1"), "UHD device address")
        ("subdev,S", po::value<std::string>(&subdev), "UHD sub-device spec, A:0 or B:0")
#else
        ("side,S", po::value<std::string>(&side), "USRP daughterboard side, A or B")
#endif
        ("ant,a", po::value<std::string>(&ant), "daughterboard antenna selection")
        ("mcr", po::value<double>(&mcr), "USRP fpga master clock rate in Hz")
#endif
        ;

    po::options_description chan_desc("Supported channels");
    chan_desc.add_options()
        ("gmr1-dl", po::value< std::vector<unsigned int> >()->multitoken(), "GMR-1 L-band downlink channel [1...1087]")
        ("gmr1-ul", po::value< std::vector<unsigned int> >()->multitoken(), "GMR-1 L-band uplink channel [1...1087]")
        ;

    po::options_description visible("visible");
    visible.add(opt_desc).add(chan_desc);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(visible).run(), vm);
    po::notify(vm);

    /* print the help message */
    if (vm.count("help")) {
        std::cout << boost::format("\nGMR-1 Acquisition tool\n") << std::endl;
        std::cout << boost::format("%s") % opt_desc << std::endl;
        std::cout << boost::format("%s") % chan_desc << std::endl;
        return ~0;
    }

    if (not vm.count("gmr1-dl") and not vm.count("gmr1-ul")) {
        std::cerr << boost::format("No channel number(s) given.") << std::endl;
        return ~0;
    }

    std::string type = "";
    std::vector< unsigned int > arfcns;

    if (vm.count("gmr1-dl")) {
        type = "gmr1-dl";
        arfcns = vm[type].as< std::vector<unsigned int> >();
    } else if (vm.count("gmr1-ul")) {
        type = "gmr1-ul";
        arfcns = vm[type].as< std::vector<unsigned int> >();
    }

    /* remove duplicate ARFCNs */
    sort( arfcns.begin(), arfcns.begin() + arfcns.size() );
    arfcns.resize( std::unique( arfcns.begin(), arfcns.end() ) - arfcns.begin() );

    std::vector< channel_base > channels;

    std::cout << "Given " << type << " ARFCNs:" << std::endl;
    BOOST_FOREACH(unsigned int arfcn, arfcns)
    {
        channel_base channel("", 0);

        if ( "gmr1-dl" == type )
            channel = gmr1_dl_channel( arfcn );
        else if ( "gmr1-ul" == type )
            channel = gmr1_ul_channel( arfcn );

        if ( arfcn < 1 || arfcn > channel._max_channels ) {
            std::cerr << boost::format("Please specify the channel number in range [1...%d]")
                         % channel._max_channels << std::endl;
            return ~0;
        }

        std::cout << boost::format("    %i = %f MHz")
                     % arfcn
                     % (channel.frequency()/1e6)
                  << std::endl;

        channels.push_back( channel );
    }

    double required_bandwidth = \
            channels.back().frequency() \
            - channels.front().frequency() \
            + channels.back()._bandwidth / 2.0 \
            + channels.front()._bandwidth / 2.0;

    std::cout << boost::format("Required bandwidth is: %f kHz")
                 % (required_bandwidth / 1e3)
              << std::endl;

    double sample_rate = 0;
#ifndef HAVE_FCD
    /* use only 75% of BW due to USRP filter shape */
    const double useful_bw_factor = 0.75;
#else
    const double useful_bw_factor = 1.0;
#endif
    channel_base last_channel = channels.back();
    int channel_rate = last_channel._symbol_rate * osr;
    int hw_decim, first_decim, second_decim, interpolation;

    std::cout << std::endl;

#ifdef HAVE_FCD
    std::cout << boost::format("Creating the FCD device...") << std::endl;
    fcd_source_c_sptr radio = fcd_make_source_c( device );
    radio->set_freq_corr( correct );
    sample_rate = 96000; /* fixed by design */
    hw_decim = 1;
    first_decim = 1;
    int gcd = boost::math::gcd( int(sample_rate), channel_rate );
    second_decim = sample_rate / gcd;
    interpolation = channel_rate / gcd;
#elif defined(HAVE_UHD)
    std::cout << boost::format("Creating the UHD device...") << std::endl;
    boost::shared_ptr<uhd_usrp_source> radio = \
            uhd_make_usrp_source( addr,
                                  uhd::io_type_t::COMPLEX_FLOAT32,
                                  1 );

    if (vm.count("mcr")) radio->set_clock_rate(mcr);
    mcr = radio->get_clock_rate();

    if (vm.count("subdev")) radio->set_subdev_spec(subdev, 0);

    std::cout << boost::format("Using %s")
                 % radio->get_device().get()->get_pp_string()
              << std::endl;

    compute_filter_params( hw_decim,
                           first_decim,
                           second_decim,
                           interpolation,
                           int(required_bandwidth / useful_bw_factor),
                           int(mcr),
                           last_channel._symbol_rate,
                           osr );

    radio->set_samp_rate( mcr / double(hw_decim) );

    sample_rate = radio->get_samp_rate();
#else
    /* create a usrp device */
    std::cout << boost::format("Creating the USRP device...") << std::endl;
    usrp_source_c_sptr radio = usrp_make_source_c( 0 );

    if (vm.count("mcr")) radio->set_fpga_master_clock_freq(long(mcr));
    mcr = double(radio->fpga_master_clock_freq());

    usrp_subdev_spec spec(0, 0); /* use A-side daughterboard by default */

    if (vm.count("side")) spec.side = (side == "B" ? 1 : 0);

    radio->set_mux( radio->determine_rx_mux_value(spec) );

    db_base_sptr dboard = radio->selected_subdev(spec);

    std::cout << boost::format("Using side %s (%g - %g MHz)")
                 % dboard->side_and_name()
                 % (dboard->freq_min() / 1e6)
                 % (dboard->freq_max() / 1e6)
              << std::endl;

    compute_filter_params( hw_decim,
                           first_decim,
                           second_decim,
                           interpolation,
                           int(required_bandwidth / useful_bw_factor),
                           int(mcr),
                           last_channel._symbol_rate,
                           osr );

    radio->set_decim_rate( hw_decim );

    sample_rate = mcr / radio->decim_rate();
#endif

#ifndef HAVE_FCD
    std::cout << boost::format("Master Clock Rate is: %f MHz")
#ifdef HAVE_UHD
                 % (radio->get_clock_rate()/1e6)
#else
                 % (radio->fpga_master_clock_freq()/1e6)
#endif
              << std::endl;
#endif
    std::cout << boost::format("Output sample rate is: %f kHz")
                 % (sample_rate/1e3)
              << std::endl;

    double available_bandwidth = sample_rate * useful_bw_factor;

    std::cout << boost::format("Available bandwidth is: %f kHz")
                 % (available_bandwidth / 1e3)
              << std::endl;

    if ( available_bandwidth < required_bandwidth ) {
        std::cerr << "Please specify less channels or increase acquisition bandwidth." << std::endl;
        return ~0;
    }

    /* calculate the frequency of the last channel of the group */
    double center_freq = last_channel.frequency();

    /* tune away from center peak caused by dc offset / phase imbalance */
    center_freq -= last_channel._bandwidth / 2.0;

    /* tune even more from the center if the acquisition bandwidth allows it */
    double offset = available_bandwidth / 2.0;
    while ( offset > last_channel._bandwidth )
        offset -= last_channel._bandwidth;
    offset = available_bandwidth / 2.0 - offset;
    center_freq -= offset;

    double tune_error = 0;

    std::cout << boost::format("Setting RX Freq: %f MHz...") % (center_freq/1e6) << std::endl;
#ifdef HAVE_FCD
    double fitting_channels = required_bandwidth / last_channel._bandwidth;

    if ( 1 == fitting_channels )
        center_freq += offset - (sample_rate / 16.0);
    else if ( 2 == fitting_channels )
        center_freq += offset;
    else if ( 3 == fitting_channels )
        center_freq += offset - last_channel._bandwidth / 2.0;

    radio->set_freq( float(center_freq) );
#elif defined(HAVE_UHD)
    uhd::tune_result_t tres = radio->set_center_freq( center_freq );
    /*tune_error = tres.target_rf_freq - tres.actual_rf_freq - tres.actual_dsp_freq;*/
#else
    freq_result_t fres = dboard->set_freq( center_freq );
    std::cout << boost::format("Actual RX Freq: %f MHz...") % (fres.baseband_freq/1e6) << std::endl;
    tune_error = center_freq - fres.baseband_freq;
#endif

    if ( tune_error != 0 ) /* we have to compensate this later in each ddc tuner */
        std::cout << boost::format("Tuning error is: %f kHz...") % (tune_error/1e3) << std::endl;

    /* set the rf gain */
    if (vm.count("gain")){
        std::cout << boost::format("Setting RX Gain: %f dB...") % gain << std::endl;
#ifdef HAVE_FCD
        radio->set_lna_gain( gain );
#else
#ifdef HAVE_UHD
        radio->set_gain( gain );
#else
        if ( dboard->set_gain( gain ) )
            std::cout << boost::format("Actual RX Gain: %f dB...") % gain << std::endl;
        else
            std::cerr << "Failed to apply RX Gain" << std::endl;
#endif
#endif
    }

#ifndef HAVE_FCD
    /* set the antenna */
    if (vm.count("ant"))
#ifdef HAVE_UHD
        radio->set_antenna( ant );
#else
        dboard->select_rx_antenna( ant );
#endif
#endif

#if 1
    std::cout << boost::format("hw decimation stage: %i") % hw_decim << std::endl;
    std::cout << boost::format("1st sw decimation stage: %i") % first_decim << std::endl;
    std::cout << boost::format("2nd sw decimation stage: %i") % second_decim << std::endl;
    std::cout << boost::format("interpolation stage: %i") % interpolation << std::endl;
#endif

    double ddc_cutoff = last_channel._bandwidth / 2.0;

    std::vector<float> ddc_taps = \
            gr_firdes::low_pass( 1, /* gain */
                                 sample_rate,
                                 ddc_cutoff,
                                 ddc_cutoff * 0.6 ); /* transition width */

    double resampler_gain = interpolation;
    double resampler_sample_rate = sample_rate * interpolation;
    double resampler_cutoff = (interpolation * sample_rate) / (second_decim * 2.0);

    if ( interpolation > second_decim )
        resampler_cutoff = sample_rate / 2.0;

    std::vector<float> channel_taps = \
            gr_firdes::low_pass( resampler_gain,
                                 resampler_sample_rate,
                                 resampler_cutoff,
                                 resampler_cutoff * 0.2 ); /* transition width */

    std::cout << std::endl;

    gr_top_block_sptr fg = gr_make_top_block( "flowgraph" );

    if ( udp.length() )
    {
        std::vector< std::string > tokens;
        split( tokens, udp, boost::is_any_of(":") );

        if ( tokens.size() == 2 )
        {
            unsigned short port = \
                    boost::lexical_cast< unsigned short >( tokens[1] );

            gr_udp_sink_sptr udp_sink = gr_make_udp_sink( sizeof(gr_complex),
                                                          tokens[0].c_str(),
                                                          port );

            fg->connect(radio, 0, udp_sink, 0);

            std::cout << boost::format("Sending samples to %s:%u ...")
                         % tokens[0]
                         % port
                      << std::endl;
        }
    }

    for ( unsigned int i = 0; i < channels.size(); i++ )
    {
        channel_base channel = channels[ i ];

        /* calculate ddc offset from center frequency */
        double tune_freq = channel.frequency() - center_freq;

        /* we use the ddc to compensate the tuning error (if any) */
        tune_freq += tune_error;
#if 0
        std::cout << boost::format("DDC%d tune_freq is %f")
                     % i
                     % tune_freq
                  << std::endl;
#endif
        gr_freq_xlating_fir_filter_ccf_sptr tuner = \
                gr_make_freq_xlating_fir_filter_ccf( first_decim,
                                                     ddc_taps,
                                                     -tune_freq,
                                                     sample_rate );

        gr_rational_resampler_base_ccf_sptr resampler = \
                gr_make_rational_resampler_base_ccf( interpolation,
                                                     second_decim,
                                                     channel_taps );

        std::string file_name = \
                str(boost::format("%s%s-%d-sps%d.cfile")
                        % prefix
                        % channel._name
                        % channel._number
                        % channel_rate);

        gr_file_sink_sptr file_sink = \
                gr_make_file_sink( sizeof(gr_complex),
                                   file_name.c_str() );

        fg->connect(radio, 0, tuner, 0);
        fg->connect(tuner, 0, resampler, 0);
        fg->connect(resampler, 0, file_sink, 0);

        std::cout << boost::format("Writing samples for ARFCN %i to %s ...")
                     % channel._number
                     % file_name
                  << std::endl;
    }

    /* Block all signals for background thread. */
    sigset_t new_mask;
    sigfillset(&new_mask);
    sigset_t old_mask;
    pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);

    /* Launch the flowgraph in a separate thread. */
    boost::thread fg_thread( boost::bind( &gr_top_block::run, fg.get(), 10000) );

    /* Restore previous signals. */
    pthread_sigmask(SIG_SETMASK, &old_mask, 0);

    /* Wait for signal indicating time to shut down. */
    sigset_t wait_mask;
    sigemptyset(&wait_mask);
    sigaddset(&wait_mask, SIGINT);
    sigaddset(&wait_mask, SIGQUIT);
    sigaddset(&wait_mask, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &wait_mask, 0);
    int sig = 0;

    if ( time )
    {
        std::cout << std::endl << "Recording for " << time << " seconds..."
                  << std::endl;

        sleep( time );

        std::cout << std::endl << "Finished recording, ";
    }
    else
    {
        std::cout << std::endl << "Press Ctrl + C to stop the receiver..."
                  << std::endl;

        sigwait(&wait_mask, &sig);

        std::cout << std::endl << strsignal(sig) << ", ";
    }

    std::cout << "stopping flowgraph... " << std::flush;

    fg->stop();

    fg_thread.join();

    std::cout << "done" << std::endl << std::endl;

    return 0;
}
