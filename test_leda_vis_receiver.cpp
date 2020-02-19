
#include <iostream>
using std::cout;
using std::endl;
#include <cstdlib>
#include <fstream>
#include <unistd.h>

#include "SimpleUDT.hpp"
#include "stopwatch.hpp"

static const char test_header[4096] =
"BW 2.616\n"
"RA 00:00:00.00\n"
"DEC 00:00:00.0\n"
"FREQ 200.000000\n"
"HDR_SIZE     4096\n"
"HDR_VERSION 1.0\n"
"INSTRUMENT LEDA\n"
"MODE TPS\n"
"NBIT         32\n"
"NCHAN 109\n"
"NDIM 2\n"
"NPOL 2\n"
"NSTATION 256\n"
"XENGINE_NTIME 8000\n"
"NAVG 216000\n"
"DATA_ORDER REG_TILE_TRIANGULAR_2x2\n"
"SKY_STATE_PHASE 0\n"
"BYTES_PER_AVG 115187712\n"
"BYTES_PER_SECOND 115187712\n"
"OBS_OFFSET   84087029760\n"
"PID P999\n"
"RECEIVER DIPOLE\n"
"SOURCE LEDA_TEST\n"
"TELESCOPE LEDAOVRO\n"
"TSAMP 41.6667\n"
"PROC_FILE ledaa.dbdisk\n"
"OBS_XFER 0\n"
"CFREQ 31.308\n"
"UTC_START    2014-11-19-20:24:02\n"
"FILE_NUMBER  0\n"
"FILE_SIZE    1151877120\n";

int main(int argc, char* argv[])
{
	if( argc <= 3 ) {
		cout << "Usage: " << argv[0] << " address port interval_secs [visibilities.dada]" << endl;
		return -1;
	}
	std::string    addr = argv[1];
	unsigned short port = atoi(argv[2]);
	float interval_secs = atof(argv[3]);
	std::string infilename;
	if( argc > 4 ) {
		infilename = argv[4];
	}
	SimpleUDT_Client socket;
	cout << "Connecting to " << addr << ":" << port << endl;
	socket.connect(addr, port);
	socket.set_timeout(1.0);
	std::vector<char> header(4096);
	size_t size = 115187712;
	std::vector<char> data(size);
	if( infilename != "" ) {
		std::ifstream infile(infilename.c_str());
		infile.read(&header[0], 4096);
		infile.read(&data[0], size);
	}
	else {
		header.assign(test_header, test_header + 4096);
		for( int i=0; i<(int)(size/sizeof(float)); ++i ) {
			((float*)&data[0])[i] = drand48();
		}
	}
	cout << "Sending header" << endl;
	socket.send(&header[0], 4096);
	Stopwatch timer;
	timer.start();
	float next_time = timer.getTime();
	while( true ) {
		cout << "Sending data" << endl;
		socket.send(&data[0], size);
		next_time += interval_secs;
		while( timer.getTime() < next_time ) {
			usleep(100*1000);
		}
	}
	/*
	int nrep = 25;
	timer.start();
	for( int r=0; r<nrep; ++r ) {
		cout << r << " Sending " << size << " bytes" << endl;
		socket.send(&data[0], size);
	}
	timer.stop();
	cout << "Total sent: " << size*nrep/1e9 << " GB" << endl;
	cout << "Time/rep = " << timer.getTime()/nrep << " s" << endl;
	cout << "         = " << nrep*size/timer.getTime()/1e6 << " MB/s" << endl;
	cout << "         = " << nrep*size/timer.getTime()/1e6*8 << " Mb/s" << endl;
	*/
	return 0;
}
