

#ifndef __GK_INTERFACE_H__
#define __GK_INTERFACE_H__

// INCLUDES

// FORWARD DECLARATIONS
#include "GKMacrodef.h"
#include "GKTypedef.h"

// CLASS DECLARATION


class IGKInterface
{

public: // Constructors and destructor

	IGKInterface(): iRefCount(1){};

	virtual ~IGKInterface(){};

	/**
	* \fn							TTInt AddRef()
	* \brief						增加接口的引用计数
	*								接口对象创建时(Constructor)，或任何对象引用此接口时(QueryInterface)，都需要增加引用计数
	* \return						当前的引用计数
	*/
	virtual	TTInt					AddRef()
	{
		++ iRefCount;
		return iRefCount;
	}

	/**
	* \fn							TTInt Release()
	* \brief						释放接口
	*								首先减少引用计数，然后判断计数是否为0，为0时，表示此接口不再被任何对象引用，调用delete this销毁此接口对象
	* \return						当前的引用计数
	*/
	virtual	TTInt					Release()
	{
		--iRefCount;
		if (0 == iRefCount)
		{
			delete this;
			return 0;
		}
		else
		{
			return iRefCount;
		}
	}

	/**
	* \fn							QueryInterface(TTUint32 aInterfaceID, void** aInterfacePtr)
	* \brief						请求接口
	* \param	aInterfaceID[in]	接口ID
	* \param	aInterfacePtr[in]	接口指针
	* \return						TTKErrNone: 成功
	*								TTKErrNotSupport: 不支持此接口
	*/
	virtual	TTInt					QueryInterface(TTUint32 /*aInterfaceID*/, void** /*aInterfacePtr*/)
	{
		return TTKErrNotSupported;
	}


private:

	TTInt							iRefCount;	/**< 引用计数 */
};

#endif // __GK_INTERFACE_H__
// End of File
