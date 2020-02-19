#ifndef DADAHEADER_H_
#define DADAHEADER_H_

#include <map>
#include <string>
#include <stdexcept>
#include <istream>

namespace dada {

class DadaHeader
{
public:
    DadaHeader(const char *dadaFilename);
    DadaHeader(char *base, std::size_t size);
    int nAnt() const {return mNAnt;}
    int nTime() const {return mNTime;}
    int nFreq() const {return mNFreq;}
    int nPol() const {return mNPol;}
    int nCorr() const {return mNCorr;}
    double intTime() const {return mIntTime;}
    double cFreq() const {return mCFreq;}
    double bandwidth() const {return mBandwidth;}
    double channelBandwidth() const {return mChannelBandwidth;}
    double channelFreq(int channelNum);
    double startTime() const {return mStartTime;}
    double finishTime() const {return mFinishTime;}
    double startTimeMJD() const {return unix2mjd(mStartTime);}
    double finishTimeMJD() const {return unix2mjd(mFinishTime);}
    int nBaseline() const {return mNBaseline;};
    int headerSize() const {return mHeaderSize;};
    std::map<std::string,std::string>& rawValues() {return mHeaderMap;};
    void writeMap(char *base, size_t size);
    static double str2epoch(const char *time, double offsetSec);
    static double unix2mjd(double time);
private:
    int mNAnt, mNTime, mNFreq, mNPol, mNCorr, mNAvg, mHeaderSize;
    long mFileSize, mObsOffset, mBytesPerAvg;
    double mIntTime, mCFreq, mBandwidth, mChannelBandwidth, mTSamp, mObsOffsetSec;
    double mStartTime, mFinishTime;
    long mNBaseline;
    std::map<std::string,std::string> mHeaderMap;
    void header2map(std::istream &s);
    void loadMap();
};

} // namespace dada

#endif // DADAHEADER_H_
