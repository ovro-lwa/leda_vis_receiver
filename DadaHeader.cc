#include "DadaHeader.h"
#include <cstdlib>
#include <fstream>
#include <istream>
#include <stdexcept>
#include <ctime>
#include <iostream>
#include "membuf.h"

namespace dada {

DadaHeader::DadaHeader(const char *dadaFilename)
{
    std::ifstream s(dadaFilename);
    header2map(s);
    loadMap();
}

DadaHeader::DadaHeader(char *base, std::size_t size)
{
    membuf sbuf(base, size);
    std::istream s(&sbuf);
    header2map(s);
    loadMap();
}

void DadaHeader::header2map(std::istream &s)
{
    // Read DADA header into map
    if (s.fail())
	    throw std::invalid_argument("Error with input stream in DadaHeader::header2map()");
    std::string key, value;
    while (s >> key >> value) {
	if (mHeaderMap.count("HDR_SIZE") > 0 && s.tellg() > atol(mHeaderMap["HDR_SIZE"].c_str()))
		break;
        mHeaderMap[key] = value;
    }
}

void DadaHeader::loadMap()
{
    mNAvg = atoi(mHeaderMap.at("NAVG").c_str());
    mTSamp = atof(mHeaderMap.at("TSAMP").c_str());
    mIntTime = mNAvg * mTSamp / 1e6;
    mNAnt = atoi(mHeaderMap["NSTATION"].c_str());
    mNBaseline = (mNAnt + 1) * mNAnt / 2;
    mNFreq = atoi(mHeaderMap["NCHAN"].c_str());
    mNPol = atoi(mHeaderMap["NPOL"].c_str());
    mNCorr = mNPol * mNPol;
    mCFreq = atof(mHeaderMap["CFREQ"].c_str()) * 1e6;
    mBandwidth = atof(mHeaderMap["BW"].c_str()) * 1e6;
    mChannelBandwidth = mBandwidth / mNFreq;
    mHeaderSize = atoi(mHeaderMap["HDR_SIZE"].c_str());
    mFileSize = atol(mHeaderMap["FILE_SIZE"].c_str());
    mBytesPerAvg = atol(mHeaderMap["BYTES_PER_AVG"].c_str());
    mNTime = mFileSize / mBytesPerAvg;
    mObsOffset = atol(mHeaderMap["OBS_OFFSET"].c_str()); // OBS_OFFSET is in bytes
    mObsOffsetSec = static_cast<double>(mObsOffset) / mBytesPerAvg * mIntTime;
    mStartTime = str2epoch(mHeaderMap["UTC_START"].c_str(), mObsOffsetSec);
    mFinishTime = mStartTime + mNTime * mIntTime;
}

void DadaHeader::writeMap(char *base, size_t size)
{
	membuf mb(base, size);
	std::ostream outs(&mb);

	for (std::map<std::string, std::string>::iterator iter = mHeaderMap.begin(); iter != mHeaderMap.end(); ++iter) {
		outs << iter->first << " " << iter->second << std::endl;
	}
	for (size_t i = outs.tellp(); i < size; ++i) {
		outs << '\0';
	}
}

double DadaHeader::channelFreq(int channelNum)
{
    return mCFreq;
}

double DadaHeader::str2epoch(const char *time, double offsetSec)
{
    struct tm t;
    if (sscanf(time, "%d-%d-%d-%d:%d:%d", &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec) != 6)
        throw std::runtime_error("Invalid String enountered in str2MEpoch()");
    --t.tm_mon;
    t.tm_year -= 1900;
    t.tm_isdst = 0;

    time_t t0 = mktime(&t) - timezone;
    return static_cast<double>(t0) + offsetSec;
}

double DadaHeader::unix2mjd(double time)
{
	// Difference between 17.11.1858 and 1.1.1970 in seconds
	return time + 3506716800.0;
}


} // namespace dada

