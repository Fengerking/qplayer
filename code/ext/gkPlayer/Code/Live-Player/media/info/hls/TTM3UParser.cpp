#include "TTM3UParser.h"
#include "GKTypedef.h"
#include "TTString.h"
#include "GKMacrodef.h"
#include <string.h>

static bool MakeURL(const char *baseURL, const char *url, TTString *out) {
	out->clear();

	if (strncmp("http://", baseURL, 7)
		&& strncmp("https://", baseURL, 8)
		&& strncmp("file://", baseURL, 7)) {
			// Base URL must be absolute
			return false;
	}

	if (!strncmp("http://", url, 7) || !strncmp("https://", url, 8)) {
		// "url" is already an absolute URL, ignore base URL.
		out->setTo(url);
		return true;
	}

	if (url[0] == '/') {
		// URL is an absolute path.
		char *protocolEnd = (char*)strstr(baseURL, "//") + 2;
		char *pathStart = strchr(protocolEnd, '/');

		if (pathStart != NULL) {
			out->setTo(baseURL, pathStart - baseURL);
		} else {
			out->setTo(baseURL);
		}

		out->append(url);
	} else {
		// URL is a relative path
		size_t n = strlen(baseURL);
		if (baseURL[n - 1] == '/') {
			out->setTo(baseURL);
			out->append(url);
		} else {
			const char *slashPos = strrchr(baseURL, '/');

			if (slashPos > &baseURL[6]) {
				out->setTo(baseURL, slashPos - baseURL);
			} else {
				out->setTo(baseURL);
			}

			out->append("/");
			out->append(url);
		}
	}

	return true;
}


M3UParser::M3UParser(const char *baseURI, const void *_data, unsigned int size)
	:mIsExtM3U(false),
	mBaseURI(baseURI),
	mIsVariantPlayList(false),
	mIsSegmentPlayList(false),
	mIsComplete(false),
	mIsEvent(false),
	mTargetDuration(0),
	mDiscontinuitySeq(0),
	mVariantNum(0),
	mSegenmtNum(0),
	mSequenceNum(0),
	mDuration(0),
	mVec(32),
	mSec(1024)
{
		mInitCheck = parse(_data,size);
}

M3UParser::~M3UParser(){
	int n;

	for(n = 0; n < mVec.Count(); n++) {
		free(mVec[n]);
	}

	for(n = 0; n < mSec.Count(); n++) {
		free(mSec[n]);
	}
}

int M3UParser::parse(const void *_data , TTUint32 size){

	TTUint32 lineNo=0;
	const char*data =(const char*)_data;
	TTUint32 offset=0;
	TTInt64 segmentRnageOffset =0;
	TTInt64 durationUs=0;
	TTInt64 totalTime = 0;
	unsigned long bandwith;
	while (offset < size){
		TTUint32 offsetLF= offset;
		while(offsetLF < size && data[offsetLF] != '\n'){
			++offsetLF;
		}

		TTString line;
		if(offsetLF > offset && data[offsetLF-1] == '\r' ){
			line.setTo(&data[offset],offsetLF- offset -1);
		}else{
			line.setTo(&data[offset],offsetLF-offset);
		}

		if(line.empty()){
			offset = offsetLF +1;
			continue;
		}

		if(lineNo == 0 && line == "#EXTM3U"){
			mIsExtM3U = true;
		}

		int discon = 0;

		if(mIsExtM3U){
			TTUint32 err = TTKErrNone;

			if(line.startsWith("#EXT-X-TARGETDURATION")){

				if(mIsVariantPlayList){
					return TTKErrNotSupported;
				}
				err = parseMetaData(line,&mTargetDuration);
			}else if(line.startsWith("#EXT-X-ENDLIST")){
				mDuration = totalTime;
				mIsComplete = true;
			}else if(line.startsWith("#EXT-X-MEDIA-SEQUENCE")){
				if(mIsVariantPlayList){
					return TTKErrNotSupported;
				}
				err= parseMetaData(line,&mSequenceNum);

			}else if(line.startsWith("#EXT-X-DISCONTINUITY")){
				if(mIsVariantPlayList){
					return TTKErrNotSupported;
				}

				discon = 1;
			} else if(line.startsWith("EXT-X-DISCONTINUITY-SEQUENCE")){
				if(mIsVariantPlayList){
					return TTKErrNotSupported;
				}
				err= parseMetaData(line,&mDiscontinuitySeq);


			} else if(line.startsWith("#EXT-X-STREAM-INF")) {

				mIsVariantPlayList = true;
				err = parseStreamInf(line, &bandwith);
			}else if(line.startsWith("#EXTINF")){

				mIsSegmentPlayList = true;
				if(mIsVariantPlayList){
					return TTKErrArgument;
				}
				err = parseMetaDataDuration(line,&durationUs);

			}

			if (err != TTKErrNone) {
				return err;
			}
		}

		if(!line.startsWith("#")){
			if(mIsVariantPlayList){
				VariantItem *mvarItem = (VariantItem*)malloc(sizeof(VariantItem));
				memset(mvarItem, 0, sizeof(VariantItem));
				mvarItem->nBandwidth = bandwith;
				TTString url;
				MakeURL(mBaseURI.c_str(),line.c_str(),&url);
				memcpy(mvarItem->url,url.c_str(),url.size());
				addVariantItem(mvarItem);				
			}else if(mIsSegmentPlayList){
				SegmentItem *mSegItem = (SegmentItem*)malloc(sizeof(SegmentItem));
				memset(mSegItem, 0, sizeof(SegmentItem));
				mSegItem->nSeqNum = mSequenceNum + mSec.Count();
				mSegItem->nDuration = durationUs;
				mSegItem->nDisflag = discon;
				TTString url;
				MakeURL(mBaseURI.c_str(),line.c_str(),&url);
				memcpy(mSegItem->url,url.c_str(),url.size());
				mSec.Append(mSegItem);

				totalTime += durationUs;
			}
		}
		offset = offsetLF + 1;
		++lineNo;
	}

	return TTKErrNone;
}

void M3UParser::addVariantItem(VariantItem* pItem) {
	for(int n = 0; n < mVec.Count(); n++) {
		if(pItem->nBandwidth < mVec[n]->nBandwidth) {
			mVec.Insert(pItem, n);
			return;
		}
	}
	mVec.Append(pItem);
}

int M3UParser::initCheck() const{
	return mInitCheck;
}

bool M3UParser::isExtM3U() const{
	return mIsExtM3U;
}

bool M3UParser::isVariantPlaylist() const{
	return mIsVariantPlayList;
}

bool M3UParser::isComplete() const{
	return mIsComplete;
}

bool M3UParser::isEvent() const{
	return mIsEvent;
}

int M3UParser::getSequenceNum() const {
	return mSequenceNum;
}

unsigned int M3UParser::getTotalDuration() const {
	return mDuration;
}

unsigned int M3UParser::getDiscontinuitySeq() const{
	return mDiscontinuitySeq;
}

unsigned int M3UParser::getTargetDuration() const{
	return mTargetDuration*1000;
}

unsigned int  M3UParser::getVariantNum() const{
	return	mVec.Count();
}

VariantItem*  M3UParser::getVariantItem(int nIndex) const{
	if(nIndex >= mVec.Count())
		return NULL;

	return mVec[nIndex];
}

unsigned int  M3UParser::getSegmentNum() const{
	return mSec.Count();
}

SegmentItem*  M3UParser::getSegmentItem(int nIndex) const{
	if(nIndex >= mSec.Count())
		return NULL;

	return mSec[nIndex];
}

MediaItem*  M3UParser::getMediaAudioItem(int nStream, int nIndex){
	return NULL;
}

MediaItem*  M3UParser::getMediaVideoItem(int nStream, int nIndex){
	return NULL;
}

MediaItem*  M3UParser::getMediaSubtitleItem(int nStream, int nIndex){
	return NULL;
}

//static
int M3UParser::parseMetaData(
	const TTString &line, TTInt32 *meta) {
		TTInt32 colonPos = line.find(":");

		if (colonPos < 0) {
			return TTKErrNotFound;
		}

		TTInt32 x;
		int err = ParseInt32(line.c_str() + colonPos + 1, &x);

		if (err != TTKErrNone) {
			return err;
		}
		*meta = x;
		return TTKErrNone;
}

unsigned int M3UParser::parseStreamInf(
	const TTString &line, unsigned long *bandwith) {
		TTInt32 colonPos = line.find(":");

		if (colonPos < 0) {
			return TTKErrNotFound;
		}

		TTUint32 offset = colonPos + 1;

		while (offset < line.size()) {
			TTInt32 end = line.find(",", offset);
			if (end < 0) {
				end = line.size();
			}

			TTString attr(line, offset, end - offset);
			attr.trim();
			offset = end + 1;

			TTInt32 equalPos = attr.find("=");
			if (equalPos < 0) {
				continue;
			}

			TTString key(attr, 0, equalPos);
			key.trim();

			TTString val(attr, equalPos + 1, attr.size() - equalPos - 1);
			val.trim();

			if (!strncmp("BANDWIDTH", key.c_str(),key.size())) {
				const char *s = val.c_str();
				char *end;
				unsigned long x = strtoul(s, &end, 10);				
				if (end == s || *end != '\0') {
					// malformed
					continue;
				}

				if(x < 32*1024) {
					x *= 1024;
				}
				*bandwith = x;
			}
		}

		return TTKErrNone;
}



// static
int M3UParser::parseMetaDataDuration(
	const TTString &line, TTInt64*duration) {
		TTInt32 colonPos = line.find(":");

		if (colonPos < 0) {
			return TTKErrNotFound;
		}

		double x;
		int err = ParseDouble(line.c_str() + colonPos + 1, &x);

		if (err != TTKErrNone) {
			return err;
		}

		*duration = (TTInt64)(x * 1E3);

		return TTKErrNone;
}

// static
int M3UParser::ParseInt32(const char *s, TTInt32 *x) {
	char *end;
	long lval = strtol(s, &end, 10);

	if (end == s || (*end != '\0' && *end != ',')) {
		return TTKErrNotFound;
	}

	*x = (TTInt32)lval;

	return TTKErrNone;
}
// static
int M3UParser::ParseDouble(const char *s, double *x) {
	char *end;
	double dval = strtod(s, &end);

	if (end == s || (*end != '\0' && *end != ',')) {
		return TTKErrNotFound;
	}

	*x = dval;

	return TTKErrNone;
}



