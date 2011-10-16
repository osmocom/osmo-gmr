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

#ifndef GMR_CHANNELS_HPP
#define GMR_CHANNELS_HPP

////////////////////////////////////////////////////////////////////////////////
class channel_base
{
public:
    channel_base( const std::string & name, unsigned int number ) :
        _name(name), _number(number) {}
    virtual ~channel_base() {}

    virtual double frequency() /* as per GMR-1 05.005, p4.2 */
        { return _base_freq + _bandwidth * _number; }

    std::string _name;
    double _symbol_rate;
    double _bandwidth;
    double _base_freq;
    unsigned int _max_channels;
    unsigned int _number;
};

////////////////////////////////////////////////////////////////////////////////
class gmr1_dl_channel : public channel_base
{
public:
    gmr1_dl_channel( unsigned int number ) :
        channel_base( "gmr1-dl", number )
    {
        _symbol_rate = 23400; /* from GMR-1 05.004, p4.2 */
        _bandwidth = 31250;
        _base_freq = 1525e6;
        _max_channels = 1087;
    }
};

////////////////////////////////////////////////////////////////////////////////
class gmr1_ul_channel : public channel_base
{
public:
    gmr1_ul_channel( unsigned int number ) :
        channel_base( "gmr1-ul", number )
    {
        _symbol_rate = 23400; /* from GMR-1 05.004, p4.2 */
        _bandwidth = 31250;
        _base_freq = 1525e6 + 101.5e6;
        _max_channels = 1087;
    }
};

#endif // GMR_CHANNELS_HPP
