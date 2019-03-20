/*******************************************************************************
	File:		UUrlParser.h

	Contains:	URL parser header file.

	Written by:	Bangfei Jin

	Change History (most recent first):
	2017-01-06		Bangfei			Create file

*******************************************************************************/
/**
* \fn                       void ParseProtocal(const char* aUrl, char* aProtocal);
* \brief                    ����Urlǰ׺������Э������
* \param[in]	aUrl		Url
* \param[out]	aProtocal	Э����(û��Э����ʱ�����ؿ��ַ�)
*/
void					qcUrlParseProtocal(const char* aUrl, char* aProtocal);

/**
* \fn                       void ParseExtension(const char* aUrl, char* aExtension);
* \brief                    ����Url��׺����
* \param[in]	aUrl		Url
* \param[out]	aExtension	��׺��(û�к�׺ʱ�����ؿ��ַ�)
*/
void					qcUrlParseExtension(const char* aUrl, char* aExtension, int nExtensionLen);

/**
* \fn                       void ParseShortName(const char* aUrl, char* aShortName);
* \brief                    ����Url·���е�short name������: ����http://www.google.com/download/1.mp3 --> ���1.mp3
* \param[in]	aUrl		Url
* \param[out]	aShortName	Short name.
*/
void					qcUrlParseShortName(const char* aUrl, char* aShortName);

/**
* \fn                       void ParseUrl(const char* aUrl, char* aHost, char* aPath, int& aPort);
* \brief                    ��Url�н�������������·���Ͷ˿ںš�
* \param[in]	aUrl		Url
* \param[out]	aHost		������
* \param[out]	aPath		·��
* \param[out]	aPort		�˿ں�
*/
void					qcUrlParseUrl(const char* aUrl, char* aHost, char* aPath, int& aPort, char * aDomain);

int						qcUrlConvert (const char * pURL, char * pDest, int nSize);

