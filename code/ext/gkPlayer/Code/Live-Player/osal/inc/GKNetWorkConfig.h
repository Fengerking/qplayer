#ifndef __GK_NETWORK_CONFIG__
#define __GK_NETWORK_CONFIG__

#include "GKNetWorkType.h"

class GKNetWorkConfig
{
public:
	static GKNetWorkConfig* getInstance();
	static void				release();

	GKActiveNetWorkType getActiveNetWorkType();

	void SetActiveNetWorkType(GKActiveNetWorkType aNetWorkType);

private:
	GKNetWorkConfig();

private:
	GKActiveNetWorkType			iNetWorkType;
	static GKNetWorkConfig*		iNetWorkConfig;
};

#endif