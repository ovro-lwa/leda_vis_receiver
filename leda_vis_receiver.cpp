
// 27861

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#include <vector>
#include <stdexcept>
#include <cstdlib>
#include <cstdarg>
#include <fstream>
#include <sstream>

#include "SimpleUDT.hpp"
#include "DadaHeader.h"

int         bind_to_core(int core_id, pthread_t tid=0);
const char* ascii_header_find(const char* header, const char* keyword);
int         ascii_header_get (const char* header, const char* keyword,
                              const char* format, ...);

struct UserData {
	char *outdir;
};

// This is called in a new thread for each incoming connection
void process_connection(SimpleUDT_Connection* connection,
                        const char*           address,
                        unsigned short        port,
                        void*                 user_data) {
	cout << "New connection from " << address << ":" << port << endl;
	enum { HEADER_SIZE = 4096 };
	std::vector<char> header(HEADER_SIZE);
	try { connection->receive(&header[0], HEADER_SIZE); }
	catch( std::exception& e ) {
		cerr << "WARNING: Connection lost from " << address << ":" << port << endl;
		return;
	}
	dada::DadaHeader dheader(&header[0], HEADER_SIZE);
	char   utc_start[32] = {0};
	size_t snapshot_size = 0;
	float  cfreq         = 0;
	int    navg          = 0;
	size_t obs_offset    = 0;
	if( ascii_header_get(&header[0], "UTC_START",      "%s", &utc_start)     < 0 ||
	    ascii_header_get(&header[0], "BYTES_PER_AVG", "%lu", &snapshot_size) < 0 ||
	    ascii_header_get(&header[0], "CFREQ",          "%f", &cfreq)         < 0 ||
	    ascii_header_get(&header[0], "NAVG",           "%d", &navg)          < 0 ||
	    ascii_header_get(&header[0], "OBS_OFFSET",    "%lu", &obs_offset)    < 0 ) {
		throw std::runtime_error("Failed to find/parse required header keywords");
	}
	cout << "UTC_START:     " << utc_start << endl;
	cout << "CFREQ:         " << cfreq << " MHz" << endl;
	cout << "NAVG:          " << navg << endl;
	cout << "OBS_OFFSET:    " << obs_offset << endl;
	cout << "Snapshot size: " << snapshot_size << " bytes" << endl;
	
	std::vector<float> snapshot(snapshot_size/sizeof(float));
	char filename[80] = {0};
	char *outdir = static_cast<UserData*>(user_data)->outdir;
	while( true ) {
		try { connection->receive((char*)&snapshot[0], snapshot_size); }
		catch( std::exception& e ) {
			cerr << "WARNING: Connection lost from "
			     << address << ":" << port
			     << " (" << e.what() << ")" << endl;
			return;
		}
		//cout << "Snapshot received" << endl;
		
		// **********************************
		// Do stuff with the data here!
		// **********************************
		sprintf(filename, "%s/%s_%016zd.000000.dada", outdir, utc_start, obs_offset);
		std::stringstream ss;
		ss << obs_offset;
		dheader.rawValues()["OBS_OFFSET"] = ss.str();
		ss.str("");
		ss << snapshot_size;
		dheader.rawValues()["FILE_SIZE"] = ss.str();
		dheader.writeMap(&header[0], HEADER_SIZE);
		std::ofstream outfile(filename, std::ios_base::out | std::ios_base::binary);
		outfile.write((char*)&header[0], HEADER_SIZE);
		outfile.write((char*)&snapshot[0], snapshot_size);
		outfile.close();
		obs_offset += snapshot_size;
		if (obs_offset > 1e16 - snapshot_size) {
			cout << "OBS_OFFSET limit reached" << endl;
			exit(1);
		}
	}
}

int main(int argc, char* argv[]) {
	if( argc != 3 && argc != 4 ) {
		cout << "Usage: " << argv[0] << " <port> <output_dir> [cpu_core]" << endl;
		return -1;
	}
	cout << "Initialising server" << endl;
	unsigned short port = atoi(argv[1]);
	UserData user_data;
	user_data.outdir = argv[2];
	//char *output_dir = argv[2];
	if( argc > 3 ) {
		int core = atoi(argv[3]);
		cout << "Binding to core " << core << endl;
		bind_to_core(core);
	}
	SimpleUDT_Server server(port);
	cout << "Listening for new connections on port " << port << endl;
	while( true ) {
		cout << "Calling process connection" << endl;
		server.accept(process_connection, (void*)&user_data);
	}
	return 0;
}

// Note: Pass core_id = -1 to unbind
int bind_to_core(int core_id, pthread_t tid) {
	// Check for valid core_id
	int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
	if (core_id < -1 || core_id >= num_cores) {
		return -1;
	}
	// Create core mask
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	if( core_id >= 0 ) {
		// Set specified core
		CPU_SET(core_id, &cpuset);
	}
	else {
		// Set all valid cores (i.e., 'un-bind')
		for( int c=0; c<num_cores; ++c ) {
			CPU_SET(c, &cpuset);
		}
	}
	// Default to current thread
	if( tid == 0 ) {
		tid = pthread_self();
	}
	// Set affinity (note: non-portable)
	return pthread_setaffinity_np(tid, sizeof(cpu_set_t), &cpuset);
}

// Stripped from PSRDADA
const char* ascii_header_find(const char* header, const char* keyword) {
  const char* key = strstr (header, keyword);
  // keyword might be the very first word in header
  while (key > header)
  {
    // if preceded by a new line, return the found key
    if ((*(key-1) == '\n') || (*(key-1) == '\\'))
      break;
    // otherwise, search again, starting one byte later
    key = strstr (key+1, keyword);
  }
  return key;
}
int ascii_header_get(const char* header, const char* keyword,
                     const char* format, ...) {
  static const char* whitespace = " \t\n";
  va_list arguments;
  const char* value = 0;
  int ret = 0;
  /* find the keyword */
  const char* key = ascii_header_find (header, keyword);
  if (!key)
    return -1;
  /* find the value after the keyword */
  value = key + strcspn (key, whitespace);
  /* parse the value */
  va_start (arguments, format);
  ret = vsscanf (value, format, arguments);
  va_end (arguments);
  return ret;
}
