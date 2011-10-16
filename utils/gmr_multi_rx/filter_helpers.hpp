/* Conversion of Python routines from Sylvain Munaut <tnt@246tNt.com>
 * (C) 2011 by Dimitri Stolnikov <horiz0n@gmx.net>
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

#ifndef FILTER_HELPERS_HPP
#define FILTER_HELPERS_HPP

#include <boost/math/common_factor.hpp> // gcd
#include <cmath> // ceil, floor

int gcd( int n, int d )
{
    return boost::math::gcd( n, d );
}

void reduce_fract( int & n_out, int & d_out, int n_in, int d_in )
{
    int g = gcd( n_in, d_in );

    n_out = n_in / g;
    d_out = d_in / g;
}

bool find_decim( int & hw_decim, int & sw_decim, int & ai, int d, int max_decim )
{
    int x = max_decim;

    while ( x >= 4 )
    {
        if ( d % x == 0 )
        {
            // If it's odd, the usrp can't do it. But maybe we can decimate twice
            // as much and then interpolate more in sw
            if ( x % 2 == 1 )
            {
                if ( x * 2 > max_decim )
                {
                    x--;
                    continue;
                }

                hw_decim = 2 * x;
                sw_decim = d / x;
                ai = 2;
                return true;
            }
            else
            {
                hw_decim = x;
                sw_decim = d / x;
                ai = 1;
                return true;
            }
        }

        x--;
    }

    hw_decim = 0;
    sw_decim = 0;
    ai = 1;

    return false;
}

void find_split( int & first_decim, int & second_decim, int decim, int inter)
{
    int mx = decim / (inter * 2);
    first_decim = 1;

    for ( int x = 2; x < mx + 1; x++ )
    {
        if (decim % x == 0)
            first_decim = x;
    }

    second_decim = decim / first_decim;
}

void compute_filter_params( int & hw_decim,
                            int & s1_decim,
                            int & s2_decim,
                            int & s2_inter,
                            int req_bw,
                            int fpga_freq,
                            int symbol_rate,
                            int osr )
{
    // Required samplerate
    int req_symrate = int(std::ceil(req_bw / symbol_rate)) * symbol_rate;
    if ( req_symrate < osr * symbol_rate )
        req_symrate = osr * symbol_rate;

    // Find the total decim / inter ratio for 1 channel
    int tot_inter, tot_decim;
    reduce_fract( tot_inter, tot_decim, osr * symbol_rate, fpga_freq);

    // Need to split the decimation between USRP and SW
    // (but we need _at_ least req_symrate out of the USRP)
    int max_decim = int(std::floor(fpga_freq / req_symrate));
    if ( max_decim > 256 )
        max_decim = 256;

    int sw_decim, ai;
    find_decim( hw_decim, sw_decim, ai, tot_decim, max_decim );

    if ( hw_decim == 0 )
    {
        // No good solution found ... so settle for a bad one
        hw_decim = max_decim;
        reduce_fract( tot_inter, sw_decim, tot_inter * hw_decim, tot_decim );
    }
    else
    {
        tot_inter *= ai;
    }

    s2_inter = tot_inter;

    // Split the sw decim in 2 stage
    find_split( s1_decim, s2_decim, sw_decim, tot_inter );
}

#endif // FILTER_HELPERS_HPP
