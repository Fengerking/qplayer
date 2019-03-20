#ifndef __AMF_H__
#define __AMF_H__
/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *      Copyright (C) 2008-2009 Andrej Stepanchuk
 *      Copyright (C) 2009-2010 Howard Chu
 *
 *  This file is part of librtmp.
 *
 *  librtmp is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 2.1,
 *  or (at your option) any later version.
 *
 *  librtmp is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with librtmp see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 *  http://www.gnu.org/copyleft/lgpl.html
 */

#include <stdint.h>

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

#ifdef __cplusplus
extern "C"
{
#endif

  typedef enum
  { qcAMF_NUMBER = 0, qcAMF_BOOLEAN, qcAMF_STRING, qcAMF_OBJECT,
    qcAMF_MOVIECLIP,		/* reserved, not used */
    qcAMF_NULL, qcAMF_UNDEFINED, qcAMF_REFERENCE, qcAMF_ECMA_ARRAY, qcAMF_OBJECT_END,
    qcAMF_STRICT_ARRAY, qcAMF_DATE, qcAMF_LONG_STRING, qcAMF_UNSUPPORTED,
    qcAMF_RECORDSET,		/* reserved, not used */
    qcAMF_XML_DOC, qcAMF_TYPED_OBJECT,
    qcAMF_AVMPLUS,		/* switch to AMF3 */
    qcAMF_INVALID = 0xff
  } qcAMFDataType;

  typedef enum
  { qcAMF3_UNDEFINED = 0, qcAMF3_NULL, qcAMF3_FALSE, qcAMF3_TRUE,
    qcAMF3_INTEGER, qcAMF3_DOUBLE, qcAMF3_STRING, qcAMF3_XML_DOC, qcAMF3_DATE,
    qcAMF3_ARRAY, qcAMF3_OBJECT, qcAMF3_XML, qcAMF3_BYTE_ARRAY
  } qcAMF3DataType;

  typedef struct AVal
  {
    char *av_val;
    int av_len;
  } AVal;
#define AVC(str)	{str,sizeof(str)-1}
#define AVMATCH(a1,a2)	((a1)->av_len == (a2)->av_len && !memcmp((a1)->av_val,(a2)->av_val,(a1)->av_len))

  struct qcAMFObjectProperty;

  typedef struct qcAMFObject
  {
    int o_num;
    struct qcAMFObjectProperty *o_props;
  } qcAMFObject;

  typedef struct qcAMFObjectProperty
  {
    AVal p_name;
    qcAMFDataType p_type;
    union
    {
      double p_number;
      AVal p_aval;
      qcAMFObject p_object;
    } p_vu;
    int16_t p_UTCoffset;
  } qcAMFObjectProperty;

  char *qcAMF_EncodeString(char *output, char *outend, const AVal * str);
  char *qcAMF_EncodeNumber(char *output, char *outend, double dVal);
  char *qcAMF_EncodeInt16(char *output, char *outend, short nVal);
  char *qcAMF_EncodeInt24(char *output, char *outend, int nVal);
  char *qcAMF_EncodeInt32(char *output, char *outend, int nVal);
  char *qcAMF_EncodeBoolean(char *output, char *outend, int bVal);

  /* Shortcuts for AMFProp_Encode */
  char *qcAMF_EncodeNamedString(char *output, char *outend, const AVal * name, const AVal * value);
  char *qcAMF_EncodeNamedNumber(char *output, char *outend, const AVal * name, double dVal);
  char *qcAMF_EncodeNamedBoolean(char *output, char *outend, const AVal * name, int bVal);

  unsigned short qcAMF_DecodeInt16(const char *data);
  unsigned int qcAMF_DecodeInt24(const char *data);
  unsigned int qcAMF_DecodeInt32(const char *data);
  void qcAMF_DecodeString(const char *data, AVal * str);
  void qcAMF_DecodeLongString(const char *data, AVal * str);
  int qcAMF_DecodeBoolean(const char *data);
  double qcAMF_DecodeNumber(const char *data);

  char *qcAMF_Encode(qcAMFObject * obj, char *pBuffer, char *pBufEnd);
  char *qcAMF_EncodeEcmaArray(qcAMFObject *obj, char *pBuffer, char *pBufEnd);
  char *qcAMF_EncodeArray(qcAMFObject *obj, char *pBuffer, char *pBufEnd);

  int qcAMF_Decode(qcAMFObject * obj, const char *pBuffer, int nSize,
		 int bDecodeName);
  int qcAMF_DecodeArray(qcAMFObject * obj, const char *pBuffer, int nSize,
		      int nArrayLen, int bDecodeName);
  int qcAMF3_Decode(qcAMFObject * obj, const char *pBuffer, int nSize,
		  int bDecodeName);
  void qcAMF_Dump(qcAMFObject * obj);
  void qcAMF_Reset(qcAMFObject * obj);

  void qcAMF_AddProp(qcAMFObject * obj, const qcAMFObjectProperty * prop);
  int qcAMF_CountProp(qcAMFObject * obj);
  qcAMFObjectProperty *qcAMF_GetProp(qcAMFObject * obj, const AVal * name,
				 int nIndex);

  qcAMFDataType qcAMFProp_GetType(qcAMFObjectProperty * prop);
  void qcAMFProp_SetNumber(qcAMFObjectProperty * prop, double dval);
  void qcAMFProp_SetBoolean(qcAMFObjectProperty * prop, int bflag);
  void qcAMFProp_SetString(qcAMFObjectProperty * prop, AVal * str);
  void qcAMFProp_SetObject(qcAMFObjectProperty * prop, qcAMFObject * obj);

  void qcAMFProp_GetName(qcAMFObjectProperty * prop, AVal * name);
  void qcAMFProp_SetName(qcAMFObjectProperty * prop, AVal * name);
  double qcAMFProp_GetNumber(qcAMFObjectProperty * prop);
  int qcAMFProp_GetBoolean(qcAMFObjectProperty * prop);
  void qcAMFProp_GetString(qcAMFObjectProperty * prop, AVal * str);
  void qcAMFProp_GetObject(qcAMFObjectProperty * prop, qcAMFObject * obj);

  int qcAMFProp_IsValid(qcAMFObjectProperty * prop);

  char *qcAMFProp_Encode(qcAMFObjectProperty * prop, char *pBuffer, char *pBufEnd);
  int qcAMF3Prop_Decode(qcAMFObjectProperty * prop, const char *pBuffer,
		      int nSize, int bDecodeName);
  int qcAMFProp_Decode(qcAMFObjectProperty * prop, const char *pBuffer,
		     int nSize, int bDecodeName);

  void qcAMFProp_Dump(qcAMFObjectProperty * prop);
  void qcAMFProp_Reset(qcAMFObjectProperty * prop);

  typedef struct qcAMF3ClassDef
  {
    AVal cd_name;
    char cd_externalizable;
    char cd_dynamic;
    int cd_num;
    AVal *cd_props;
  } qcAMF3ClassDef;

  void qcAMF3CD_AddProp(qcAMF3ClassDef * cd, AVal * prop);
  AVal *qcAMF3CD_GetProp(qcAMF3ClassDef * cd, int idx);

#ifdef __cplusplus
}
#endif

#endif				/* __AMF_H__ */
