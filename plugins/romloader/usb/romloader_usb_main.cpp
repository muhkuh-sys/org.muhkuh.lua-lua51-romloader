/***************************************************************************
 *   Copyright (C) 2007 by Christoph Thelen                                *
 *   doc_bacardi@users.sourceforge.net                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include <ctype.h>
#include <stdio.h>

#include "romloader_usb_main.h"

#ifdef _WINDOWS
	#define snprintf _snprintf
	#define ssize_t size_t
#endif

/*-------------------------------------*/

/*
 * libusb-0.1 compatibility functions
 */

#if ROMLOADER_USB_LIBUSB_VERSION==0

#ifdef _WINDOWS
#define LIBUSB_BUS_PATTERN "bus-%u"
#define LIBUSB_DEV_PATTERN "\\\\.\\libusb0-%u"
#else
#define LIBUSB_BUS_PATTERN "%u"
#define LIBUSB_DEV_PATTERN "%u"
#endif


int libusb_open(libusb_device *ptDevice, libusb_device_handle **pptDevHandle)
{
	libusb_device_handle *ptDevHandle;
	int iError;


	ptDevHandle = usb_open(ptDevice);
	if( ptDevHandle!=NULL )
	{
		*pptDevHandle = ptDevHandle;
		iError = 0;
	}
	else
	{
		*pptDevHandle = NULL;
		iError = -1;
	}

	return iError;
}


void libusb_close(libusb_device_handle *dev_handle)
{
	usb_close(dev_handle);
}


int libusb_init(libusb_context **pptContext)
{
	usb_init();

	/* use something different from NULL */
	*pptContext = (libusb_context*)1;

	return LIBUSB_SUCCESS;
}


void libusb_exit(libusb_context *ptContext)
{
}


void libusb_set_debug(libusb_context *ptContext, int iLevel)
{
}


int usb_bulk_pc_to_netx(libusb_device_handle *ptDevHandle, unsigned char ucEndPointOut, const unsigned char *pucDataOut, int iLength, int *piProcessed, unsigned int uiTimeoutMs)
{
	int iError;
#ifdef _WINDOWS
	char *pcDataOut = (char*)pucDataOut;
#else
	const char *pcDataOut = (const char*)pucDataOut;
#endif


	iError = usb_bulk_write(ptDevHandle, ucEndPointOut, pcDataOut, iLength, uiTimeoutMs);
	if( iError==iLength )
	{
		/* transfer ok! */
		iError = 0;
	}
	else
	{
		/* do not return 0 in case of an error */
		if( iError==0 )
		{
			iError = -1;
		}
		else if( iError==-110 )
		{
			iError = LIBUSB_ERROR_TIMEOUT;
		}
		/* transfer failed */
		iLength = 0;
	}

	if( piProcessed!=NULL )
	{
		*piProcessed = iLength;
	}

	return iError;
}


int usb_bulk_netx_to_pc(libusb_device_handle *ptDevHandle, unsigned char ucEndPointIn, unsigned char *pucDataIn, int iLength, int *piProcessed, unsigned int uiTimeoutMs)
{
	int iError;


	iError = usb_bulk_read(ptDevHandle, ucEndPointIn, (char*)pucDataIn, iLength, uiTimeoutMs);
	if( iError>0 )
	{
		/* transfer ok! */
		if( piProcessed!=NULL )
		{
			*piProcessed = iError;
		}
		iError = 0;
	}
	else
	{
		/* do not return 0 in case of an error */
		if( iError==0 )
		{
			iError = -1;
		}
		else if( iError==-110 )
		{
			iError = LIBUSB_ERROR_TIMEOUT;
		}
		/* transfer failed */
		if( piProcessed!=NULL )
		{
			*piProcessed = 0;
		}
	}

	return iError;
}


int libusb_get_device_descriptor(libusb_device *dev, struct libusb_device_descriptor *desc)
{
	*desc = dev->descriptor;
	return LIBUSB_SUCCESS;
}


uint8_t libusb_get_bus_number(libusb_device *dev)
{
	uint8_t ucBusNr;
	usb_bus *ptBus;
	const char *pcBusName;
	int iResult;
	unsigned int uiBusNumber;


	/* expect failure */
	ucBusNr = 0xffU;

	/* is the device valid? */
	if( dev!=NULL )
	{
		/* does the device have a bus assigned? */
		ptBus = dev->bus;
		if( ptBus!=NULL )
		{
			/* does the bus have a directory name? */
			pcBusName = ptBus->dirname;
			if( pcBusName!=NULL )
			{
				/* parse the directory name */
				iResult = sscanf(pcBusName, LIBUSB_BUS_PATTERN, &uiBusNumber);
				/* does the directory name have the expected format? */
				if( iResult==1 )
				{
					/* is the bus number in the valid range? */
					if( uiBusNumber<0x80U )
					{
						/* set the result */
						ucBusNr = (uint8_t)uiBusNumber;
					}
				}
			}
		}
	}

	return ucBusNr;
}


uint8_t libusb_get_device_address(libusb_device *dev)
{
	uint8_t ucDeviceAddress;
	const char *pcFilename;
	int iResult;
	unsigned int uiDeviceNumber;


	/* expect failure */
	ucDeviceAddress = 0xffU;

	/* is the device valid? */
	if( dev!=NULL )
	{
		/* does the bus have a directory name? */
		pcFilename = dev->filename;
		if( pcFilename!=NULL )
		{
			/* parse the directory name */
			iResult = sscanf(pcFilename, LIBUSB_DEV_PATTERN, &uiDeviceNumber);
			/* does the directory name have the expected format? */
			if( iResult==1 )
			{
				/* is the bus number in the valid range? */
				if( uiDeviceNumber<0x80U )
				{
					/* set the result */
					ucDeviceAddress = (uint8_t)uiDeviceNumber;
				}
			}
		}
	}

	return ucDeviceAddress;
}


int libusb_reset_device(libusb_device_handle *dev)
{
	return usb_reset(dev);
}


int libusb_set_configuration(libusb_device_handle *dev, int configuration)
{
	return usb_set_configuration(dev, configuration);
}


int libusb_claim_interface(libusb_device_handle *dev, int iface)
{
	return usb_claim_interface(dev, iface);
}


int libusb_release_interface(libusb_device_handle *dev, int iface)
{
	return usb_release_interface(dev, iface);
}


ssize_t libusb_get_device_list(libusb_context *ctx, libusb_device ***list)
{
	libusb_device **ptDeviceList;
	libusb_device **ptCnt;
	ssize_t ssizDeviceList;
	struct usb_bus *ptBusses;
	struct usb_bus *ptBusCnt;
	struct usb_device *ptDevCnt;


	/* detect all busses */
	usb_find_busses();
	/* detect all devices on each bus */
	usb_find_devices();

	/* init the device list */
	ptDeviceList = NULL;

	/* init the device counter */
	ssizDeviceList = 0;

	/* get the head of the bus list */
	ptBusses = usb_get_busses();

	/* loop over all bus entries and count the devices */
	ptBusCnt = ptBusses;
	while( ptBusCnt!=NULL )
	{
		/* loop over all devices on this bus */
		ptDevCnt = ptBusCnt->devices;
		while( ptDevCnt!=NULL )
		{
			/* found one more device */
			++ssizDeviceList;

			/* next device */
			ptDevCnt = ptDevCnt->next;
		}
		/* scan next bus */
		ptBusCnt = ptBusCnt->next;
	}

	/* found any devices? */
	if( ssizDeviceList>0 )
	{
		/* allocate the array */
		ptDeviceList = (libusb_device **)malloc(ssizDeviceList*sizeof(libusb_device*));
		if( ptDeviceList==NULL )
		{
			ssizDeviceList = LIBUSB_ERROR_NO_MEM;
		}
		else
		{
			ptCnt = ptDeviceList;

			/* loop over all bus entries and count the devices */
			ptBusCnt = ptBusses;
			while( ptBusCnt!=NULL )
			{
				/* loop over all devices on this bus */
				ptDevCnt = ptBusCnt->devices;
				while( ptDevCnt!=NULL )
				{
					/* add the device to the list */
					*(ptCnt++) = ptDevCnt;

					/* next device */
					ptDevCnt = ptDevCnt->next;
				}
				/* scan next bus */
				ptBusCnt = ptBusCnt->next;
			}

			*list = ptDeviceList;
		}
	}

	return ssizDeviceList;
}


void libusb_free_device_list(libusb_device **list, int unref_devices)
{
	/* free the complete list */
	free(list);
}


#endif

/*-------------------------------------*/

/*
 * common functions for all libusb versions
 */


typedef struct
{
	libusb_error eErrNo;
	const char *pcErrMsg;
} LIBUSB_STRERROR_T;


static const LIBUSB_STRERROR_T atStrError[] =
{
	{ LIBUSB_SUCCESS,		"success" },
	{ LIBUSB_ERROR_IO,		"input/output error" },
	{ LIBUSB_ERROR_INVALID_PARAM,	"invalid parameter" },
	{ LIBUSB_ERROR_ACCESS,		"access denied (insufficient permissions)" },
	{ LIBUSB_ERROR_NO_DEVICE,	"no such device (it may have been disconnected)" },
	{ LIBUSB_ERROR_NOT_FOUND,	"entity not found" },
	{ LIBUSB_ERROR_BUSY,		"resource busy" },
	{ LIBUSB_ERROR_TIMEOUT,		"operation timed out" },
	{ LIBUSB_ERROR_OVERFLOW,	"overflow" },
	{ LIBUSB_ERROR_PIPE,		"pipe error" },
	{ LIBUSB_ERROR_INTERRUPTED,	"system call interrupted (perhaps due to signal)" },
	{ LIBUSB_ERROR_NO_MEM,		"insufficient memory" },
	{ LIBUSB_ERROR_NOT_SUPPORTED,	"operation not supported or unimplemented on this platform" },
	{ LIBUSB_ERROR_OTHER,		"other error" },
};


static const char *libusb_strerror(int iError)
{
	const LIBUSB_STRERROR_T *ptCnt, *ptEnd;
	const char *pcMsg;
	const char *pcUnknownError = "unknown error";


	ptCnt = atStrError;
	ptEnd = ptCnt + (sizeof(atStrError)/sizeof(atStrError[0]));
	pcMsg = pcUnknownError;
	while( ptCnt<ptEnd )
	{
		if( ptCnt->eErrNo==iError )
		{
			pcMsg = ptCnt->pcErrMsg;
			break;
		}
		else
		{
			++ptCnt;
		}
	}

	return pcMsg;
}


bool isDeviceNetx(libusb_device *ptDev)
{
	bool fDeviceIsNetx;
	int iResult;
	struct libusb_device_descriptor sDevDesc;


	fDeviceIsNetx = false;
	if( ptDev!=NULL )
	{
		iResult = libusb_get_device_descriptor(ptDev, &sDevDesc);
		if( iResult==LIBUSB_SUCCESS )
		{
			if(
				sDevDesc.bDeviceClass==0x00 &&
				sDevDesc.bDeviceSubClass==0x00 &&
				sDevDesc.bDeviceProtocol==0x00 &&
				sDevDesc.idVendor==0x0cc4 &&
				sDevDesc.idProduct==0x0815 &&
				sDevDesc.bcdDevice==0x0100
			  )
			{
				/* This seems to be a netX500 ABoot device. */
				fDeviceIsNetx  = true;
			}
			else if(
				sDevDesc.bDeviceClass==0xff &&
				sDevDesc.bDeviceSubClass==0x00 &&
				sDevDesc.bDeviceProtocol==0xff &&
				sDevDesc.idVendor==0x1939 &&
				sDevDesc.idProduct==0x000c &&
				sDevDesc.bcdDevice==0x0001
			       )
			{
				/* This seems to be a netX10 HBoot device. */
				fDeviceIsNetx  = true;
			}
		}
	}

	return fDeviceIsNetx;
}



/*-------------------------------------*/

const char *romloader_usb_provider::m_pcPluginNamePattern = "romloader_usb_%02x_%02x";

romloader_usb_provider::romloader_usb_provider(swig_type_info *p_romloader_usb, swig_type_info *p_romloader_usb_reference)
 : muhkuh_plugin_provider("romloader_usb")
 , m_ptLibUsbContext(NULL)
{
	int iResult;


	printf("%s(%p): provider create\n", m_pcPluginId, this);

	/* get the romloader_baka lua type */
	m_ptPluginTypeInfo = p_romloader_usb;
	m_ptReferenceTypeInfo = p_romloader_usb_reference;

	/* create a new libusb context */
	iResult = libusb_init(&m_ptLibUsbContext);
	if( iResult!=LIBUSB_SUCCESS )
	{
		/* failed to create the context */
		printf("%s(%p): Failed to create libusb context: %d:%s\n", m_pcPluginId, this, iResult, libusb_strerror(iResult));
	}
}


romloader_usb_provider::~romloader_usb_provider(void)
{
	printf("%s(%p): provider delete\n", m_pcPluginId, this);

	if( m_ptLibUsbContext!=NULL )
	{
		/* free the libusb context */
		libusb_exit(m_ptLibUsbContext);
	}
}


int romloader_usb_provider::DetectInterfaces(lua_State *ptLuaStateForTableAccess)
{
	int iResult;
	ssize_t ssizDevList;
	libusb_device **ptDeviceList;
	libusb_device **ptDevCnt, **ptDevEnd;
	libusb_device *ptDev;
	libusb_device_handle *ptDevHandle;
	unsigned int uiBusNr;
	unsigned int uiDevAdr;
	bool fDeviceIsBusy;
	int iInterfaces;
	bool fDeviceIsNetx;
	romloader_usb_reference *ptRef;
	const size_t sizMaxName = 32;
	char acName[sizMaxName];


	iInterfaces = 0;

	/* check the libusb context */
	if( m_ptLibUsbContext==NULL )
	{
		/* libusb was not initialized */
		printf("%s(%p): libusb was not initialized!\n", m_pcPluginId, this);
	}
	else
	{
		/* detect devices */
		ptDeviceList = NULL;
		ssizDevList = libusb_get_device_list(m_ptLibUsbContext, &ptDeviceList);
		if( ssizDevList<0 )
		{
			/* failed to detect devices */
			printf("%s(%p): failed to detect usb devices: %d:%s\n", m_pcPluginId, this, ssizDevList, libusb_strerror(ssizDevList));
		}
		else
		{
			/* loop over all devices */
			ptDevCnt = ptDeviceList;
			ptDevEnd = ptDevCnt + ssizDevList;
			while( ptDevCnt<ptDevEnd )
			{
				ptDev = *ptDevCnt;
				fDeviceIsNetx = isDeviceNetx(ptDev);
				if( fDeviceIsNetx==true )
				{
					/* construct the name */
					uiBusNr = libusb_get_bus_number(ptDev);
					uiDevAdr = libusb_get_device_address(ptDev);
					snprintf(acName, sizMaxName-1, m_pcPluginNamePattern, uiBusNr, uiDevAdr);

					/* open the device */
					iResult = libusb_open(ptDev, &ptDevHandle);
					if( iResult!=LIBUSB_SUCCESS )
					{
						/* failed to open the interface, do not add it to the list */
						printf("%s(%p): failed to open device %s: %d:%s\n", m_pcPluginId, this, acName, iResult, libusb_strerror(iResult));
					}
					else
					{
						/* set the configuration */
						iResult = libusb_set_configuration(ptDevHandle, 1);
						if( iResult!=LIBUSB_SUCCESS )
						{
							/* failed to set the configuration */
							printf("%s(%p): failed to set the configuration of device %s: %d:%s\n", m_pcPluginId, this, acName, iResult, libusb_strerror(iResult));
						}
						else
						{
							/* claim the interface, 0 is the interface number */
							iResult = libusb_claim_interface(ptDevHandle, 0);
							if( iResult!=LIBUSB_SUCCESS && iResult!=LIBUSB_ERROR_BUSY )
							{
								/* failed to claim the interface */
								printf("%s(%p): failed to claim the interface of device %s: %d:%s\n", m_pcPluginId, this, acName, iResult, libusb_strerror(iResult));
							}
							else
							{
								if( iResult!=LIBUSB_SUCCESS )
								{
									/* the interface is busy! */
									fDeviceIsBusy = true;
								}
								else
								{
									/* ok, claimed the interface -> the device is not busy */
									fDeviceIsBusy = false;
									/* release the interface */
									/* NOTE: The 'busy' information only represents the device state at detection time.
									 * This function _must_not_ keep the claim on the device or other applications will
									 * not be able to use it.
									 */
									iResult = libusb_release_interface(ptDevHandle, 0);
									if( iResult!=LIBUSB_SUCCESS )
									{
										/* failed to release the interface */
										printf("%s(%p): failed to release the interface of device %s after a successful claim: %d:%s\n", m_pcPluginId, this, acName, iResult, libusb_strerror(iResult));
									}
								}

								/* create the new instance */
								ptRef = new romloader_usb_reference(acName, m_pcPluginId, fDeviceIsBusy, this);
								add_reference_to_table(ptLuaStateForTableAccess, ptRef);
								/* count instances */
								++iInterfaces;
							}
						}

						/* close the device */
						libusb_close(ptDevHandle);
					}
				}
				/* next list item */
				++ptDevCnt;
			}
			/* free the device list */
			if( ptDeviceList!=NULL )
			{
				libusb_free_device_list(ptDeviceList, 1);
			}
		}
	}

	return iInterfaces;
}


romloader_usb *romloader_usb_provider::ClaimInterface(const muhkuh_plugin_reference *ptReference)
{
	romloader_usb *ptPlugin;
	const char *pcName;
	unsigned int uiBusNr;
	unsigned int uiDevAdr;


	/* expect error */
	ptPlugin = NULL;


	if( ptReference==NULL )
	{
		fprintf(stderr, "%s(%p): claim_interface(): missing reference!\n", m_pcPluginId, this);
	}
	else
	{
		pcName = ptReference->GetName();
		if( pcName==NULL )
		{
			fprintf(stderr, "%s(%p): claim_interface(): missing name!\n", m_pcPluginId, this);
		}
		else if( sscanf(pcName, m_pcPluginNamePattern, &uiBusNr, &uiDevAdr)!=2 )
		{
			fprintf(stderr, "%s(%p): claim_interface(): invalid name: %s\n", m_pcPluginId, this, pcName);
		}
		else if( m_ptLibUsbContext==NULL )
		{
			/* libusb was not initialized */
			printf("%s(%p): libusb was not initialized!\n", m_pcPluginId, this);
		}
		else
		{
			ptPlugin = new romloader_usb(pcName, m_pcPluginId, this, uiBusNr, uiDevAdr);
			printf("%s(%p): claim_interface(): claimed interface %s.\n", m_pcPluginId, this, pcName);
		}
	}

	return ptPlugin;
}


bool romloader_usb_provider::ReleaseInterface(muhkuh_plugin *ptPlugin)
{
	bool fOk;
	const char *pcName;
	unsigned int uiBusNr;
	unsigned int uiDevAdr;


	/* expect error */
	fOk = false;


	if( ptPlugin==NULL )
	{
		fprintf(stderr, "%s(%p): release_interface(): missing plugin!\n", m_pcPluginId, this);
	}
	else
	{
		pcName = ptPlugin->GetName();
		if( pcName==NULL )
		{
			fprintf(stderr, "%s(%p): release_interface(): missing name!\n", m_pcPluginId, this);
		}
		else if( sscanf(pcName, m_pcPluginNamePattern, &uiBusNr, &uiDevAdr)!=2 )
		{
			fprintf(stderr, "%s(%p): release_interface(): invalid name: %s\n", m_pcPluginId, this, pcName);
		}
		else
		{
			printf("%s(%p): released interface %s.\n", m_pcPluginId, this, pcName);
			fOk = true;
		}
	}

	return fOk;
}


/*-------------------------------------*/

romloader_usb::romloader_usb(const char *pcName, const char *pcTyp, romloader_usb_provider *ptProvider, unsigned int uiBusNr, unsigned int uiDeviceAdr)
 : romloader(pcName, pcTyp, ptProvider)
 , m_ptUsbProvider(ptProvider)
 , m_uiBusNr(uiBusNr)
 , m_uiDeviceAdr(uiDeviceAdr)
 , m_ptLibUsbContext(NULL)
 , m_ptUsbDev(NULL)
 , m_ptUsbDevHandle(NULL)
{
	int iResult;


	printf("%s(%p): created in romloader_usb\n", m_pcName, this);

	/* create a new libusb context */
	iResult = libusb_init(&m_ptLibUsbContext);
	if( iResult!=LIBUSB_SUCCESS )
	{
		/* failed to create the context */
		printf("%s(%p): Failed to create libusb context: %d:%s\n", m_pcName, this, iResult, libusb_strerror(iResult));
	}
}


romloader_usb::~romloader_usb(void)
{
	printf("%s(%p): deleted in romloader_usb\n", m_pcName, this);

	if( m_ptLibUsbContext!=NULL )
	{
		/* free the libusb context */
		libusb_exit(m_ptLibUsbContext);
	}
}


bool romloader_usb::chip_init(lua_State *ptClientData)
{
	bool fResult;


	switch( m_tChiptyp )
	{
	case ROMLOADER_CHIPTYP_NETX500:
	case ROMLOADER_CHIPTYP_NETX100:
		switch( m_tRomcode )
		{
		case ROMLOADER_ROMCODE_ABOOT:
			// aboot does not set the serial vectors
			write_data32(ptClientData, 0x10001ff0, 0x00200582|1);		// get: usb_receiveChar
			write_data32(ptClientData, 0x10001ff4, 0x0020054e|1);		// put: usb_sendChar
			write_data32(ptClientData, 0x10001ff8, 0);			// peek: none
			write_data32(ptClientData, 0x10001ffc, 0x00200566|1);		// flush: usb_sendFinish
			fResult = true;
			break;
		case ROMLOADER_ROMCODE_HBOOT:
			// hboot needs no special init
			fResult = true;
			break;
		case ROMLOADER_ROMCODE_UNKNOWN:
			fResult = false;
			break;
		}
		break;

	case ROMLOADER_CHIPTYP_NETX50:
		switch( m_tRomcode )
		{
		case ROMLOADER_ROMCODE_ABOOT:
			// this is an unknown combination
			fResult = false;
			break;
		case ROMLOADER_ROMCODE_HBOOT:
			// hboot needs no special init
			fResult = true;
			break;
		case ROMLOADER_ROMCODE_UNKNOWN:
			fResult = false;
			break;
		}
		break;

	case ROMLOADER_CHIPTYP_NETX10:
		switch( m_tRomcode )
		{
		case ROMLOADER_ROMCODE_ABOOT:
			// this is an unknown combination
			fResult = false;
			break;
		case ROMLOADER_ROMCODE_HBOOT:
			// hboot needs no special init
			fResult = true;
			break;
		case ROMLOADER_ROMCODE_UNKNOWN:
			fResult = false;
			break;
		}
		break;

	case ROMLOADER_CHIPTYP_UNKNOWN:
		fResult = false;
		break;
	}

	return fResult;
}


/* open the connection to the device */
void romloader_usb::Connect(lua_State *ptClientData)
{
	int iResult;
	DATA_BUFFER_T tBuffer;
	SWIGLUA_REF tRef;
	ssize_t ssizDevList;
	libusb_device **ptDeviceList;
	libusb_device **ptDevCnt, **ptDevEnd;
	libusb_device *ptDev;
	libusb_device_handle *ptUsbDevHandle;
	struct libusb_device_descriptor sDevDesc;


	tRef.L = NULL;
	tRef.ref = 0;

	if( m_fIsConnected!=false )
	{
		printf("%s(%p): already connected, ignoring new connect request\n", m_pcName, this);
	}
	else
	{
		m_ptUsbDev = NULL;
		m_ptUsbDevHandle = NULL;

		ptDeviceList = NULL;

		/* search device with bus and address */
		ssizDevList = libusb_get_device_list(m_ptLibUsbContext, &ptDeviceList);
		if( ssizDevList<0 )
		{
			/* failed to detect devices */
			MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): failed to detect usb devices: %d:%s", m_pcName, this, ssizDevList, libusb_strerror(ssizDevList));
			iResult = (int)ssizDevList;
		}
		else
		{
			/* loop over all devices */
			ptDevCnt = ptDeviceList;
			ptDevEnd = ptDevCnt + ssizDevList;
			while( ptDevCnt<ptDevEnd )
			{
				ptDev = *ptDevCnt;
				if( isDeviceNetx(ptDev)==true && libusb_get_bus_number(ptDev)==m_uiBusNr && libusb_get_device_address(ptDev)==m_uiDeviceAdr )
				{
					/* Get the vendor and product id. */
					iResult = libusb_get_device_descriptor(ptDev, &sDevDesc);
					if( iResult==LIBUSB_SUCCESS )
					{
						if( sDevDesc.idVendor==0x1939 && sDevDesc.idProduct==0x000c )
						{
							printf("VID=0x$04x, PID=0x%04x -> assumed netx10\n", sDevDesc.idVendor, sDevDesc.idProduct);
							m_tChiptyp = ROMLOADER_CHIPTYP_NETX10;
							m_tRomcode = ROMLOADER_ROMCODE_HBOOT;
						}

						m_ptUsbDev = ptDev;
						break;
					}
				}

				++ptDevCnt;
			}

			/* found the requested device? */
			if( m_ptUsbDev==NULL )
			{
				MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): interface not found. Maybe it was plugged out.\n", m_pcName, this);
				iResult = LIBUSB_ERROR_NOT_FOUND;
			}
			else
			{
				iResult = libusb_open(m_ptUsbDev, &ptUsbDevHandle);
				if( iResult!=LIBUSB_SUCCESS )
				{
					MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): failed to open the device: %d:%s", m_pcName, this, iResult, libusb_strerror(iResult));
				}
				else
				{
					/* set the configuration */
					iResult = libusb_set_configuration(ptUsbDevHandle, 1);
					if( iResult!=LIBUSB_SUCCESS )
					{
						/* failed to set the configuration */
						MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): failed to set the configuration of device: %d:%s\n", m_pcName, this, iResult, libusb_strerror(iResult));
					}
					else
					{
						/* claim the interface, 0 is the interface number */
						iResult = libusb_claim_interface(ptUsbDevHandle, 0);
						if( iResult!=LIBUSB_SUCCESS )
						{
							/* failed to claim the interface */
							MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): failed to claim the device: %d:%s\n", m_pcName, this, iResult, libusb_strerror(iResult));
						}
						else
						{
							m_ptUsbDevHandle = ptUsbDevHandle;

							// get netx welcome message
							iResult = usb_getNetxData(&tBuffer, &tRef, 0);
							if( iResult!=LIBUSB_SUCCESS )
							{
								printf("%s(%p): failed to receive netx response, trying to reset netx: %d:%s\n", m_pcName, this, iResult, libusb_strerror(iResult));

								// try to reset the device and try again
								iResult = libusb_resetDevice();
								if( iResult!=LIBUSB_SUCCESS )
								{
									MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): failed to reset the netx, giving up: %d:%s", m_pcName, this, iResult, libusb_strerror(iResult));
									libusb_closeDevice();
								}
								else
								{
									printf("%s(%p): reset ok!\n", m_pcName, this);

									libusb_closeDevice();
									m_ptUsbDevHandle = NULL;
									iResult = libusb_open(m_ptUsbDev, &m_ptUsbDevHandle);
									if( iResult==LIBUSB_SUCCESS )
									{
										if( m_ptUsbDevHandle==NULL )
										{
											MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): lost device after reset!");
											iResult = LIBUSB_ERROR_OTHER;
										}
										else
										{
											// get netx welcome message
											iResult = usb_getNetxData(&tBuffer, &tRef, 0);
											if( iResult!=LIBUSB_SUCCESS )
											{
												MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): failed to receive netx response: %d:%s", m_pcName, this, iResult, libusb_strerror(iResult));
												libusb_closeDevice();
											}
										}
									}
								}
							}

							if( iResult==LIBUSB_SUCCESS )
							{
								printf("%s(%p): netx response: %s\n", m_pcName, this, tBuffer.pucData);
								free(tBuffer.pucData);

								/* NOTE: set m_fIsConnected to true here or detect_chiptyp and chip_init will fail! */
								m_fIsConnected = true;

								if( detect_chiptyp(ptClientData)!=true )
								{
									MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): failed to detect chiptyp!", m_pcName, this);
									m_fIsConnected = false;
									libusb_closeDevice();
								}
								else if( chip_init(ptClientData)!=true )
								{
									MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): failed to init chip!", m_pcName, this);
									m_fIsConnected = false;
									libusb_closeDevice();
								}
							}
						}
					}
				}
			}

			/* free the device list */
			libusb_free_device_list(ptDeviceList, 1);
		}

		if( iResult!=LIBUSB_SUCCESS )
		{
			MUHKUH_PLUGIN_EXIT_ERROR(ptClientData);
		}
	}
}


/* close the connection to the device */
void romloader_usb::Disconnect(lua_State *ptClientData)
{
	printf("%s(%p): disconnect()\n", m_pcName, this);

	/* NOTE: allow disconnects even if the plugin was already disconnected. */

	if( m_ptUsbDevHandle!=NULL )
	{
		libusb_close(m_ptUsbDevHandle);
	}

	m_fIsConnected = false;
	m_ptUsbDevHandle = NULL;
}


bool romloader_usb::parse_hex_digit(DATA_BUFFER_T *ptBuffer, size_t sizDigits, unsigned long *pulResult)
{
	const unsigned char *pucData;
	unsigned long ulResult;
	bool fOk;
	unsigned char uc;
	unsigned int uiDigit;
	size_t sizCnt;

	if( ptBuffer->sizPos+sizDigits>ptBuffer->sizData )
	{
		// not enough chars left in the buffer -> this can not work!
		fOk = false;
	}
	else
	{
		pucData = ptBuffer->pucData + ptBuffer->sizPos;
		ulResult = 0;
		fOk = true;
		sizCnt = 0;
		while( sizCnt<sizDigits )
		{
			uc = *(pucData++);
			uc = tolower(uc);
			if( uc>='0' && uc<='9' )
			{
				uiDigit = uc - '0';
			}
			else if( uc>='a' && uc<='f' )
			{
				uiDigit = uc - 'a' + 10;
			}
			else
			{
				fOk = false;
				break;
			}
			ulResult <<= 4;
			ulResult |= uiDigit;

			++sizCnt;
		}

		if( fOk==true )
		{
			if( pulResult!=NULL )
			{
				*pulResult = ulResult;
			}
			ptBuffer->sizPos += sizDigits;
		}
	}

	return fOk;
}


bool romloader_usb::expect_string(DATA_BUFFER_T *ptBuffer, const char *pcMatch)
{
	size_t sizMatch;
	const unsigned char *pucMatch;
	const unsigned char *pucData;
	bool fOk;
	unsigned char uc;
	size_t sizCnt;


	sizMatch = strlen(pcMatch);
	pucMatch = (unsigned char*)pcMatch;

	if( ptBuffer->sizPos+sizMatch>ptBuffer->sizData )
	{
		fOk = false;
	}
	else
	{
		pucData = ptBuffer->pucData + ptBuffer->sizPos;
		fOk = true;
		sizCnt = 0;
		do
		{
			uc = *(pucMatch++);
			fOk = ( uc==*(pucData++) );
			++sizCnt;
		} while( fOk==true && sizCnt<sizMatch );
	}

	if( fOk==true )
	{
		ptBuffer->sizPos += sizMatch;
	}

	return fOk;
}


bool romloader_usb::skip_line(DATA_BUFFER_T *ptBuffer)
{
	size_t sizCnt;
	unsigned char ucData;
	bool fFoundEol;


	/* No eol found yet. */
	fFoundEol = false;

	sizCnt = ptBuffer->sizPos;

	/* Skip until the end of the line. */
	do
	{
		ucData = ptBuffer->pucData[sizCnt++];
		if( ucData=='\n' || ucData=='\r' )
		{
			ptBuffer->sizPos = sizCnt;
			fFoundEol = true;
			break;
		}
	} while( sizCnt<ptBuffer->sizData );

	return fFoundEol;
}


size_t romloader_usb::get_line_length(DATA_BUFFER_T *ptBuffer)
{
	size_t sizLine;
	unsigned char ucData;
	const unsigned char *pucCnt;
	const unsigned char *pucEnd;


	/* Get the line length. */
	sizLine = 0;
	pucCnt = ptBuffer->pucData + ptBuffer->sizPos;
	pucEnd = ptBuffer->pucData + ptBuffer->sizData;

	/* Search for lineend. */
	while( pucCnt<pucEnd )
	{
		ucData = *(pucCnt++);
		if( ucData!='\n' && ucData!='\r' )
		{
			++sizLine;
		}
		else
		{
			break;
		}
	}

	return sizLine;
}


/* read a byte (8bit) from the netx to the pc */
unsigned char romloader_usb::read_data08(lua_State *ptClientData, unsigned long ulNetxAddress)
{
	int iResult;
	char acCommand[19];
	DATA_BUFFER_T tBuffer;
	unsigned long ulResponseAddress;
	unsigned long ulResponseValue;
	bool fOk;


	ulResponseValue = 0;

	/* Expect failure. */
	fOk = false;

	/* Construct the command. */
	if( m_tChiptyp==ROMLOADER_CHIPTYP_NETX10 )
	{
		snprintf(acCommand, sizeof(acCommand), "db %08lX ++1", ulNetxAddress);
	}
	else
	{
		snprintf(acCommand, sizeof(acCommand), "DUMP %08lX BYTE", ulNetxAddress);
	}

	if( m_fIsConnected==false )
	{
		MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): not connected!", m_pcName, this);
	}
	else
	{
		/* Send the command. */
		iResult = usb_executeCommand(acCommand, &tBuffer);
		if( iResult!=LIBUSB_SUCCESS )
		{
			MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): failed to send command: %d:%s", m_pcName, this, iResult, libusb_strerror(iResult));
		}
		else
		{
			if( parse_hex_digit(&tBuffer, 8, &ulResponseAddress)!=true )
			{
				MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): strange response from device: %s", m_pcName, this, tBuffer.pucData);
			}
			else if( ulResponseAddress!=ulNetxAddress )
			{
				MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): address does not match request: %s", m_pcName, this, tBuffer.pucData);
			}
			else if( expect_string(&tBuffer, ": ")!=true )
			{
				MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): strange response from device: %s", m_pcName, this, tBuffer.pucData);
			}
			else if( parse_hex_digit(&tBuffer, 2, &ulResponseValue)!=true )
			{
				MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): strange response from device: %s", m_pcName, this, tBuffer.pucData);
			}
			else
			{
				printf("%s(%p): read_data08: 0x%08lx = 0x%02lx\n", m_pcName, this, ulNetxAddress, ulResponseValue);
				fOk = true;
			}

			free(tBuffer.pucData);
		}
	}

	if( fOk!=true )
	{
		MUHKUH_PLUGIN_EXIT_ERROR(ptClientData);
	}

	return (unsigned char)ulResponseValue;
}


/* read a word (16bit) from the netx to the pc */
unsigned short romloader_usb::read_data16(lua_State *ptClientData, unsigned long ulNetxAddress)
{
	int iResult;
	char acCommand[19];
	DATA_BUFFER_T tBuffer;
	unsigned long ulResponseAddress;
	unsigned long ulResponseValue;
	bool fOk;


	ulResponseValue = 0;

	/* Expect failure. */
	fOk = false;

	/* Construct the command. */
	if( m_tChiptyp==ROMLOADER_CHIPTYP_NETX10 )
	{
		snprintf(acCommand, sizeof(acCommand), "dw %08lX ++2", ulNetxAddress);
	}
	else
	{
		snprintf(acCommand, sizeof(acCommand), "DUMP %08lX WORD", ulNetxAddress);
	}

	if( m_fIsConnected==false )
	{
		MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): not connected!", m_pcName, this);
	}
	else
	{
		/* Send the command. */
		iResult = usb_executeCommand(acCommand, &tBuffer);
		if( iResult!=LIBUSB_SUCCESS )
		{
			MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): failed to send command: %d:%s", m_pcName, this, iResult, libusb_strerror(iResult));
		}
		else
		{
			if( parse_hex_digit(&tBuffer, 8, &ulResponseAddress)!=true )
			{
				MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): strange response from device: %s", m_pcName, this, tBuffer.pucData);
			}
			else if( ulResponseAddress!=ulNetxAddress )
			{
				MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): address does not match request: %s", m_pcName, this, tBuffer.pucData);
			}
			else if( expect_string(&tBuffer, ": ")!=true )
			{
				MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): strange response from device: %s", m_pcName, this, tBuffer.pucData);
			}
			else if( parse_hex_digit(&tBuffer, 4, &ulResponseValue)!=true )
			{
				MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): strange response from device: %s", m_pcName, this, tBuffer.pucData);
			}
			else
			{
				printf("%s(%p): read_data16: 0x%08lx = 0x%04lx\n", m_pcName, this, ulNetxAddress, ulResponseValue);
				fOk = true;
			}

			free(tBuffer.pucData);
		}
	}

	if( fOk!=true )
	{
		MUHKUH_PLUGIN_EXIT_ERROR(ptClientData);
	}

	return (unsigned short)ulResponseValue;
}


/* read a long (32bit) from the netx to the pc */
unsigned long romloader_usb::read_data32(lua_State *ptClientData, unsigned long ulNetxAddress)
{
	int iResult;
	char acCommand[19];
	DATA_BUFFER_T tBuffer;
	unsigned long ulResponseAddress;
	unsigned long ulResponseValue;
	bool fOk;


	ulResponseValue = 0;

	/* Expect failure. */
	fOk = false;

	/* Construct the command. */
	if( m_tChiptyp==ROMLOADER_CHIPTYP_NETX10 )
	{
		snprintf(acCommand, sizeof(acCommand), "d %08lX ++4\r", ulNetxAddress);
	}
	else
	{
		snprintf(acCommand, sizeof(acCommand), "DUMP %08lX LONG", ulNetxAddress);
	}

	if( m_fIsConnected==false )
	{
		MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): not connected!", m_pcName, this);
	}
	else
	{
		/* Send the command. */
		iResult = usb_executeCommand(acCommand, &tBuffer);
		if( iResult!=LIBUSB_SUCCESS )
		{
			MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): failed to send command: %d:%s", m_pcName, this, iResult, libusb_strerror(iResult));
		}
		else
		{
			if( m_tChiptyp==ROMLOADER_CHIPTYP_NETX10 && expect_string(&tBuffer, acCommand)!=true )
			{
				MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): strange response 1 from device: %s", m_pcName, this, tBuffer.pucData);
			}
			else if( parse_hex_digit(&tBuffer, 8, &ulResponseAddress)!=true )
			{
				MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): strange response 2 from device: %s", m_pcName, this, tBuffer.pucData);
			}
			else if( ulResponseAddress!=ulNetxAddress )
			{
				MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): address does not match request: %s", m_pcName, this, tBuffer.pucData);
			}
			else if( expect_string(&tBuffer, ": ")!=true )
			{
				MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): strange response 3 from device: %s", m_pcName, this, tBuffer.pucData);
			}
			else if( parse_hex_digit(&tBuffer, 8, &ulResponseValue)!=true )
			{
				MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): strange response 4 from device: %s", m_pcName, this, tBuffer.pucData);
			}
			else
			{
				printf("%s(%p): read_data32: 0x%08lx = 0x%08lx\n", m_pcName, this, ulNetxAddress, ulResponseValue);
				fOk = true;
			}

			free(tBuffer.pucData);
		}
	}

	if( fOk!=true )
	{
		MUHKUH_PLUGIN_EXIT_ERROR(ptClientData);
	}

	return ulResponseValue;
}


bool romloader_usb::parseDumpLine(DATA_BUFFER_T *ptBuffer, unsigned long ulAddress, unsigned long ulElements, unsigned char *pucBuffer)
{
	bool fResult;
	unsigned long ulResponseAddress;
	unsigned long ulChunkCnt;
	unsigned long ulByte;
	size_t sizBufferLeft;
	size_t sizPos;
	size_t sizMax;
	char c;


	// expect success
	fResult = true;

	// get remaining size of buffer
	sizBufferLeft = ptBuffer->sizData - ptBuffer->sizPos;
	// is enough input data left?
	if( sizBufferLeft<(10+ulElements*3) )
	{
		fResult = false;
	}
	else if( parse_hex_digit(ptBuffer, 8, &ulResponseAddress)!=true )
	{
		fResult = false;
	}
	else if( ulResponseAddress!=ulAddress )
	{
		fResult = false;
	}
	else if( expect_string(ptBuffer, ": ")!=true )
	{
		fResult = false;
	}
	else
	{
		// get all bytes
		ulChunkCnt = ulElements;
		while( ulChunkCnt!=0 )
		{
			// get one hex digit
			if( parse_hex_digit(ptBuffer, 2, &ulByte)!=true )
			{
				fResult = false;
				break;
			}
			else if( expect_string(ptBuffer, " ")!=true )
			{
				fResult = false;
				break;
			}
			else
			{
				*(pucBuffer++) = (char)ulByte;
				// one number processed
				--ulChunkCnt;
			}
		}
	}

	if( fResult==true )
	{
		/* skip until the end of the line */
		sizPos = ptBuffer->sizPos;
		sizMax = ptBuffer->sizData;
		/* wait for eol */
		while( sizPos<sizMax )
		{
			c = ptBuffer->pucData[sizPos];
			if( c!='\r' && c!='\n' )
			{
				++sizPos;
			}
			else
			{
				break;
			}
		}
		/* wait for eol gone */
		while( sizPos<sizMax )
		{
			c = ptBuffer->pucData[sizPos];
			if( c!='\r' && c!='\n' )
			{
				break;
			}
			else
			{
				++sizPos;
			}
		}
		ptBuffer->sizPos = sizPos;
	}

	return fResult;
}


int romloader_usb::parse_uue(DATA_BUFFER_T *ptBuffer, unsigned long ulStart, size_t sizLength, unsigned char **ppucData)
{
	int iResult;
	const unsigned char *pucLine;
	int iMatch;
	size_t sizLine;
	size_t sizUueBytesInLine;
	size_t sizCharCnt;
	size_t sizByteCnt;
	const unsigned char *pucCnt;
	unsigned long ulResult;
	unsigned int uiMaxLineSize;
	unsigned char ucData;
	unsigned long ulValue;
	unsigned char *pucData;
	unsigned char *pucDataCnt;


	/*
	 * Skip lines until 'begin'.
	 */

	iResult = 0;
	pucData = NULL;

	/* Are at least 6 chars left (length of "begin" and one more). */
	while( (ptBuffer->sizData-ptBuffer->sizPos)>5 )
	{
		/* Compare with "begin". */
		if( memcmp(ptBuffer->pucData+ptBuffer->sizPos, "begin", 5)==0 )
		{
			break;
		}

		/* Skip until the end of the line. */
		skip_line(ptBuffer);
	}

	if( expect_string(ptBuffer, "begin 666 ")!=true )
	{
		iResult = -1;
	}
	else if( parse_hex_digit(ptBuffer, 8, &ulValue)!=true )
	{
		iResult = -1;
	}
	else if( ulValue!=ulStart )
	{
		iResult = -1;
	}
	else if( expect_string(ptBuffer, "_")!=true )
	{
		iResult = -1;
	}
	else if( parse_hex_digit(ptBuffer, 8, &ulValue)!=true )
	{
		iResult = -1;
	}
	else if( ulValue!=ulStart+sizLength )
	{
		iResult = -1;
	}
	else if( expect_string(ptBuffer, ".bin")!=true )
	{
		iResult = -1;
	}
	else
	{
		skip_line(ptBuffer);

		/* Found begin, now parse all lines until 'end' is reached. */

		pucData = (unsigned char*)malloc(sizLength);
		if( pucData==NULL )
		{
			iResult = -1;
		}
		else
		{
			pucDataCnt = pucData;
			do
			{
				/* Test for "end". */
				pucCnt = ptBuffer->pucData + ptBuffer->sizPos;
				if( (ptBuffer->sizData-ptBuffer->sizPos)>3 && memcmp(pucCnt, "end", 3)==0 )
				{
					/* found 'end' */
					break;
				}
				else
				{
					sizLine = get_line_length(ptBuffer);
					if( sizLine>0 )
					{
						/* no end or empty line -> must be uencoded line */

						/* Get the linelength character.
						 * NOTE: This will warp for numbers smaller than 0x20, and that's what we want.
						 */
						sizUueBytesInLine = *(pucCnt++) - 0x20U;
						/* The length must be between 0x40 and 0x40.
						 * NOTE: 0x40 is special as it's used for a 'end of line' marker. it equals 0.
						 */
						if( sizUueBytesInLine>0x40 )
						{
							/* Illegal line number, break */
							iResult = -1;
							break;
						}
						else
						{
							/* 0x40 must equal 0x00 */
							sizUueBytesInLine &= 0x3f;
							if( sizUueBytesInLine>0 )
							{
								/* check if the real line length matches the uee length 	*/
								sizCharCnt = 1 + (sizUueBytesInLine*4)/3;
								if( sizCharCnt>sizLine )
								{
									/* line is not long enough for the expected data, seems to be cut off */
									iResult = -1;
									break;
								}
								else
								{
									/* ok, line matches, decode all data */
									do
									{
										/* grab the next 4 chars */
										sizCharCnt = 4;
										ulResult = 0;
										do
										{
											ulResult <<= 6;
											ulResult |= (*(pucCnt++)-0x20) & 0x3f;
										} while( --sizCharCnt!=0 );

										/* write 3 decoded chars */
										sizByteCnt = 3;
										if( sizByteCnt>sizUueBytesInLine )
										{
											sizByteCnt = sizUueBytesInLine;
										}
										do
										{
											/* NOTE: this must be before the data write line, the bits must be in 31..8 */
											ulResult <<= 8;
											*(pucDataCnt++) = (unsigned char)(ulResult>>24U);
											--sizUueBytesInLine;
										} while( --sizByteCnt!=0 );
									} while( sizUueBytesInLine!=0 );
								}
							}
						}
					}
					skip_line(ptBuffer);
				}
			} while( iResult==0 );

			if( iResult!=0 && pucData!=NULL )
			{
				free(pucData);
			}
		}
	}

	*ppucData = pucData;

	return iResult;
}


int romloader_usb::uue_generate(const unsigned char *pucData, size_t sizData, char **ppcUueData, size_t *psizUueData)
{
	size_t sizUueData;
	size_t sizMaxUueData;
	char *pcUueData;
	char *pcUueDataCnt;
	const unsigned char *pucDataCnt;
	const unsigned char *pucDataEnd;
	int iResult;
	size_t sizChunk;
	unsigned long ulUueBuf;
	int iCnt;


	/* Expect error. */
	iResult = -1;

	/* Allocate the output buffer. */
	sizUueData = 0;
	sizMaxUueData = 76 + ((sizData+44)/45)*63;
	pcUueData = (char*)malloc(sizMaxUueData);
	if( pcUueData!=NULL )
	{
		pcUueDataCnt = pcUueData;

		/* Generate the header. */
		iCnt = sprintf(pcUueDataCnt, "begin 666 data.bin\n");
		pcUueDataCnt += iCnt;

		/* Dump all memory. */
		pucDataCnt = pucData;
		pucDataEnd = pucData + sizData;
		while( pucDataCnt<pucDataEnd )
		{
			/* get the rest of the bytes to dump */
			sizChunk = pucDataEnd - pucDataCnt;
			/* limit to max uuencode line size */
			if( sizChunk>45 )
			{
				sizChunk = 45;
			}

			/* print the length character for the line */
			*(pcUueDataCnt++) = (unsigned char)(0x20 + sizChunk);

			/* print one line */
			do
			{
				/* clear uuencode buffer */
				ulUueBuf = 0;

				/* get max 3 chars into the buffer */
				iCnt = 3;
				do
				{
					/* still bytes left? */
					if( sizChunk>0 )
					{
						ulUueBuf |= *(pucDataCnt++);
						--sizChunk;
					}
					/* NOTE: the shift operation must be executed after the new data is masked in, the result must be in 8..31 */
					ulUueBuf <<= 8;
				} while( --iCnt>0 );

				/* encode the buffer */
				iCnt = 4;
				do
				{
					*(pcUueDataCnt++) = (unsigned char)(0x20 + (ulUueBuf>>26));
					ulUueBuf <<= 6;
				} while( --iCnt!=0 );
			} while( sizChunk!=0 );

			/* end the line */
			*(pcUueDataCnt++) = '`';
			*(pcUueDataCnt++) = '\n';
		}
	
		/* print last line */
		iCnt = sprintf(pcUueDataCnt, "`\nend\n");
		pcUueDataCnt += iCnt+1;

		/* Get the size of the UUencoded data. */
		sizUueData = pcUueDataCnt - pcUueData;

		iResult = 0;
	}

	*ppcUueData = pcUueData;
	*psizUueData = sizUueData;

	return iResult;
}


/* read a byte array from the netx to the pc */
void romloader_usb::read_image(unsigned long ulNetxAddress, unsigned long ulSize, char **ppcBUFFER_OUT, size_t *psizBUFFER_OUT, SWIGLUA_REF tLuaFn, long lCallbackUserData)
{
	int iResult;
	char acCommand[28];
	bool fOk;
	DATA_BUFFER_T tBuffer;
	size_t sizDataExpected;
	unsigned char *pucData;
	unsigned char *pucDataCnt;
	unsigned long ulBytesLeft;
	unsigned long ulExpectedAddress;
	unsigned long ulChunkSize;


	/* Expect error. */
	fOk = false;

	/* Init the buffer. */
	tBuffer.pucData = NULL;
	tBuffer.sizData = 0;
	tBuffer.sizPos = 0;

	if( m_fIsConnected==false )
	{
		MUHKUH_PLUGIN_PUSH_ERROR(tLuaFn.L, "%s(%p): not connected!", m_pcName, this);
	}
	else
	{
		/* Construct the command. */
		if( m_tChiptyp==ROMLOADER_CHIPTYP_NETX10 )
		{
			snprintf(acCommand, sizeof(acCommand), "s %08lX ++%08lX\r", ulNetxAddress, ulSize);

			/* Send the command. */
			iResult = usb_sendCommand(acCommand);
			if( iResult!=LIBUSB_SUCCESS )
			{
				MUHKUH_PLUGIN_PUSH_ERROR(tLuaFn.L, "%s(%p): failed to send command: %d:%s", m_pcName, this, iResult, libusb_strerror(iResult));
			}
			else
			{
				/* Init the buffer. */
				sizDataExpected = 76 + ((ulSize+44)/45)*63;
				/* Rounding up to a multiple of 4096. */
				if( (sizDataExpected & 4095)!=0 )
				{
					sizDataExpected += 4096 - (sizDataExpected & 4095);
				}
				iResult = usb_getNetxData(&tBuffer, &tLuaFn, lCallbackUserData, sizDataExpected);
				if( iResult!=LIBUSB_SUCCESS )
				{
					MUHKUH_PLUGIN_PUSH_ERROR(tLuaFn.L, "%s(%p): failed to send command: %d:%s", m_pcName, this, iResult, libusb_strerror(iResult));
				}
				else if( expect_string(&tBuffer, acCommand)!=true )
				{
					MUHKUH_PLUGIN_PUSH_ERROR(tLuaFn.L, "%s(%p): strange response from device: %s", m_pcName, this, tBuffer.pucData);
				}
				else
				{
					iResult = parse_uue(&tBuffer, ulNetxAddress, ulSize, &pucData);
					if( iResult!=0 )
					{
						MUHKUH_PLUGIN_PUSH_ERROR(tLuaFn.L, "%s(%p): strange response from device: %s", m_pcName, this, tBuffer.pucData);
					}
					else
					{
						*ppcBUFFER_OUT = (char*)pucData;
						*psizBUFFER_OUT = (size_t)ulSize;

						fOk = true;
					}
				}
			}
		}
		else
		{
			snprintf(acCommand, sizeof(acCommand), "DUMP %08lX %08lX BYTE", ulNetxAddress, ulSize);

			/* Send the command. */
			iResult = usb_sendCommand(acCommand);
			if( iResult!=LIBUSB_SUCCESS )
			{
				MUHKUH_PLUGIN_PUSH_ERROR(tLuaFn.L, "%s(%p): failed to send command: %d:%s", m_pcName, this, iResult, libusb_strerror(iResult));
			}
			else
			{
/*
00000010: 09 CF 9F 09 D5 52 65 6F 86 BA F1 A4 10 98 C4 83   .....Reo........
10 + 3*16 + 2 + 16 + 1 = 77
*/
				/* Init the buffer. */
				sizDataExpected = ((ulSize+15)/16)*77;
				printf("%s(%p): expecting %d bytes\n", m_pcName, this, sizDataExpected);
				/* Rounding up to a multiple of 4096. */
				if( (sizDataExpected & 4095)!=0 )
				{
					sizDataExpected += 4096 - (sizDataExpected & 4095);
				}
				iResult = usb_getNetxData(&tBuffer, &tLuaFn, lCallbackUserData, sizDataExpected);
				if( iResult!=LIBUSB_SUCCESS )
				{
					MUHKUH_PLUGIN_PUSH_ERROR(tLuaFn.L, "%s(%p): failed to send command: %d:%s", m_pcName, this, iResult, libusb_strerror(iResult));
				}
				else
				{
					sizBufLen = 0;
					sizBufPos = 0;
					fEof = false;

					// alloc buffer
					pucData = (unsigned char*)malloc(ulSize);
					if( pucData==NULL )
					{
						MUHKUH_PLUGIN_PUSH_ERROR(tLuaFn.L, "%s(%p): failed to alloc %d bytes of input buffer!", m_pcName, this, ulSize);
						iResult = LIBUSB_ERROR_NO_MEM;
					}
					else
					{
						pucDataCnt = pucData;
						// parse the result
						ulBytesLeft = ulSize;
						ulExpectedAddress = ulNetxAddress;
						while( ulBytesLeft>0 )
						{
							// get the number of expected bytes in the next row
							ulChunkSize = 16;
							if( ulChunkSize>ulBytesLeft )
							{
								ulChunkSize = ulBytesLeft;
							}
							fOk = parseDumpLine(&tBuffer, ulExpectedAddress, ulChunkSize, pucDataCnt);
							if( fOk!=true )
							{
								MUHKUH_PLUGIN_PUSH_ERROR(tLuaFn.L, "%s(%p): failed to parse response!", m_pcName, this);
								iResult = LIBUSB_ERROR_OTHER;
								break;
							}
							else
							{
								ulBytesLeft -= ulChunkSize;
								// inc address
								ulExpectedAddress += ulChunkSize;
								// inc buffer ptr
								pucDataCnt += ulChunkSize;
							}
						}
					}

					if( fOk!=true )
					{
						free(pucData);
						pucData = NULL;
						ulSize = 0;
					}

					*ppcBUFFER_OUT = (char*)pucData;
					*psizBUFFER_OUT = (size_t)ulSize;
				}
			}
		}
	}

	if( tBuffer.pucData!=NULL )
	{
		free(tBuffer.pucData);
	}

	if( fOk!=true )
	{
		MUHKUH_PLUGIN_EXIT_ERROR(tLuaFn.L);
	}
}


/* write a byte (8bit) from the pc to the netx */
void romloader_usb::write_data08(lua_State *ptClientData, unsigned long ulNetxAddress, unsigned char ucData)
{
	int iResult;
	char acCommand[22];
	DATA_BUFFER_T tBuffer;
	bool fOk;


	// assume failure
	fOk = false;

	/* Construct the command. */
	if( m_tChiptyp==ROMLOADER_CHIPTYP_NETX10 )
	{
		snprintf(acCommand, sizeof(acCommand), "mb %08lX %02X\r", ulNetxAddress, ucData);
	}
	else
	{
		snprintf(acCommand, sizeof(acCommand), "FILL %08lX %02X BYTE", ulNetxAddress, ucData);
	}

	if( m_fIsConnected==false )
	{
		MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): not connected!", m_pcName, this);
	}
	else
	{
		// send the command
		iResult = usb_executeCommand(acCommand, &tBuffer);
		if( iResult!=LIBUSB_SUCCESS )
		{
			MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): failed to send command: %d:%s", m_pcName, this, iResult, libusb_strerror(iResult));
		}
		else
		{
			if( m_tChiptyp==ROMLOADER_CHIPTYP_NETX10 && expect_string(&tBuffer, acCommand)!=true )
			{
				MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): strange response from netx: %s", m_pcName, this, tBuffer.pucData);
			}
			else if( m_tChiptyp!=ROMLOADER_CHIPTYP_NETX10 && expect_string(&tBuffer, "\n>")!=true )
			{
				MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): strange response from netx: %s", m_pcName, this, tBuffer.pucData);
			}
			else
			{
				printf("%s(%p): write_data08: 0x%08lx = 0x%02x\n", m_pcName, this, ulNetxAddress, ucData);
				fOk = true;
			}

			free(tBuffer.pucData);
		}
	}

	if( fOk!=true )
	{
		MUHKUH_PLUGIN_EXIT_ERROR(ptClientData);
	}
}


/* write a word (16bit) from the pc to the netx */
void romloader_usb::write_data16(lua_State *ptClientData, unsigned long ulNetxAddress, unsigned short usData)
{
	int iResult;
	char acCommand[24];
	DATA_BUFFER_T tBuffer;
	bool fOk;


	// assume failure
	fOk = false;

	/* Construct the command. */
	if( m_tChiptyp==ROMLOADER_CHIPTYP_NETX10 )
	{
		snprintf(acCommand, sizeof(acCommand), "mw %08lX %04X\r", ulNetxAddress, usData);
	}
	else
	{
		snprintf(acCommand, sizeof(acCommand), "FILL %08lX %04X WORD", ulNetxAddress, usData);
	}

	if( m_fIsConnected==false )
	{
		MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): not connected!", m_pcName, this);
	}
	else
	{
		// send the command
		iResult = usb_executeCommand(acCommand, &tBuffer);
		if( iResult!=LIBUSB_SUCCESS )
		{
			MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): failed to send command: %d:%s", m_pcName, this, iResult, libusb_strerror(iResult));
		}
		else
		{
			if( m_tChiptyp==ROMLOADER_CHIPTYP_NETX10 && expect_string(&tBuffer, acCommand)!=true )
			{
				MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): strange response from netx: %s", m_pcName, this, tBuffer.pucData);
			}
			else if( m_tChiptyp!=ROMLOADER_CHIPTYP_NETX10 && expect_string(&tBuffer, "\n>")!=true )
			{
				MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): strange response from netx: %s", m_pcName, this, tBuffer.pucData);
			}
			else
			{
				printf("%s(%p): write_data16: 0x%08lx = 0x%04x\n", m_pcName, this, ulNetxAddress, usData);
				fOk = true;
			}

			free(tBuffer.pucData);
		}
	}

	if( fOk!=true )
	{
		MUHKUH_PLUGIN_EXIT_ERROR(ptClientData);
	}
}


/* write a long (32bit) from the pc to the netx */
void romloader_usb::write_data32(lua_State *ptClientData, unsigned long ulNetxAddress, unsigned long ulData)
{
	int iResult;
	char acCommand[28];
	DATA_BUFFER_T tBuffer;
	bool fOk;


	// assume failure
	fOk = false;

	/* Construct the command. */
	if( m_tChiptyp==ROMLOADER_CHIPTYP_NETX10 )
	{
		snprintf(acCommand, sizeof(acCommand), "m %08lX %08X\r", ulNetxAddress, ulData);
	}
	else
	{
		snprintf(acCommand, sizeof(acCommand), "FILL %08lX %08X LONG", ulNetxAddress, ulData);
	}

	if( m_fIsConnected==false )
	{
		MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): not connected!", m_pcName, this);
	}
	else
	{
		// send the command
		iResult = usb_executeCommand(acCommand, &tBuffer);
		if( iResult!=LIBUSB_SUCCESS )
		{
			MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): failed to send command: %d:%s", m_pcName, this, iResult, libusb_strerror(iResult));
		}
		else
		{
			if( m_tChiptyp==ROMLOADER_CHIPTYP_NETX10 && expect_string(&tBuffer, acCommand)!=true )
			{
				MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): strange response from netx: %s", m_pcName, this, tBuffer.pucData);
			}
			else if( m_tChiptyp!=ROMLOADER_CHIPTYP_NETX10 && expect_string(&tBuffer, "\n>")!=true )
			{
				MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%s(%p): strange response from netx: %s", m_pcName, this, tBuffer.pucData);
			}
			else
			{
			
			
				printf("want:\n");
				hexdump((const unsigned char*)acCommand, strlen(acCommand), 0);
				printf("got:\n");
				hexdump(tBuffer.pucData, tBuffer.sizData, 0);

			
				printf("%s(%p): write_data32: 0x%08lx = 0x%08x\n", m_pcName, this, ulNetxAddress, ulData);
				fOk = true;
			}

			free(tBuffer.pucData);
		}
	}

	if( fOk!=true )
	{
		MUHKUH_PLUGIN_EXIT_ERROR(ptClientData);
	}
}


/* write a byte array from the pc to the netx */
void romloader_usb::write_image(unsigned long ulNetxAddress, const char *pcBUFFER_IN, size_t sizBUFFER_IN, SWIGLUA_REF tLuaFn, long lCallbackUserData)
{
	int iResult;
	bool fOk;
	DATA_BUFFER_T tBuffer;
	const unsigned char *pucInputData;
	char *pcUueData;
	const unsigned char *pucUueDataCnt;
	const unsigned char *pucUueDataEnd;
	size_t sizUueData;
	size_t sizChunk;
	char acCommand[16];
	int iProcessed;


	/* Expect error. */
	fOk = false;

	pucInputData = (const unsigned char*)pcBUFFER_IN;

	if( m_fIsConnected==false )
	{
		MUHKUH_PLUGIN_PUSH_ERROR(tLuaFn.L, "%s(%p): not connected!", m_pcName, this);
	}
	else
	{
		if( m_tChiptyp==ROMLOADER_CHIPTYP_NETX10 )
		{
			printf("hihi 1\n");
			/* Generate the command. */
			snprintf(acCommand, sizeof(acCommand), "l %08lX\r", ulNetxAddress);
			/* Send the command. */
			iResult = usb_executeCommand(acCommand, &tBuffer);
			if( iResult!=LIBUSB_SUCCESS )
			{
				printf("err 1\n");
				MUHKUH_PLUGIN_PUSH_ERROR(tLuaFn.L, "%s(%p): failed to send command: %d:%s", m_pcName, this, iResult, libusb_strerror(iResult));
			}
			else if( expect_string(&tBuffer, acCommand)!=true )
			{
				printf("err 2\n");
				MUHKUH_PLUGIN_PUSH_ERROR(tLuaFn.L, "%s(%p): strange response from netx: %s", m_pcName, this, tBuffer.pucData);
			}
			else
			{
				printf("hihi 2\n");
				pcUueData = NULL;
				sizUueData = 0;
				iResult = uue_generate(pucInputData, sizBUFFER_IN, &pcUueData, &sizUueData);
				printf("uue data: '%s'\n", pcUueData);
				printf("result: %d\n", iResult);
				if( iResult==0 )
				{
					/* Send the complete data block. */
					pucUueDataCnt = (const unsigned char*)pcUueData;
					pucUueDataEnd = pucUueDataCnt + sizUueData;
					while( pucUueDataCnt<pucUueDataEnd )
					{
						sizChunk = pucUueDataEnd - pucUueDataCnt;
						if( sizChunk>64 )
						{
							sizChunk = 64;
						}
						iResult = usb_bulk_pc_to_netx(m_ptUsbDevHandle, 0x04, pucUueDataCnt, sizChunk, &iProcessed, 500);
						if( iResult==LIBUSB_SUCCESS )
						{
							printf("send\n");
							pucUueDataCnt += sizChunk;
						}
						else if( iResult==LIBUSB_ERROR_TIMEOUT )
						{
							/* Just retry. */
							printf("retry\n");
							continue;
						}
						else
						{
							break;
						}
					}
					printf("done: %d\n", iResult);
					free(pcUueData);

					if( iResult==0 )
					{
						/* Get the response. */
						iResult = usb_getNetxData(&tBuffer, &tLuaFn, lCallbackUserData);
						if( iResult!=LIBUSB_SUCCESS )
						{
							printf("err 3\n");
							MUHKUH_PLUGIN_PUSH_ERROR(tLuaFn.L, "%s(%p): failed to get response from netx", m_pcName, this);
						}
						else if( expect_string(&tBuffer, "Result: 0\r\r>")!=true )
						{
							printf("err 4\n");
							hexdump(tBuffer.pucData, tBuffer.sizData, 0);
							MUHKUH_PLUGIN_PUSH_ERROR(tLuaFn.L, "%s(%p): strange response from netx: %s", m_pcName, this, tBuffer.pucData);
						}
						else
						{
							printf("Yay!\n");
							printf("want:\n");
							hexdump((const unsigned char*)"Result: 0\r\r>", sizeof("Result: 0\r\r>"), 0);
							printf("got:\n");
							hexdump(tBuffer.pucData, tBuffer.sizData, 0);
							
							fOk = true;
						}
					}
				}
			}
		}
		else
		{
			/* Send the command. */
			iResult = usb_load(pucInputData, sizBUFFER_IN, ulNetxAddress, &tLuaFn, lCallbackUserData);
			if( iResult!=LIBUSB_SUCCESS )
			{
				MUHKUH_PLUGIN_PUSH_ERROR(tLuaFn.L, "%s(%p): failed to send command: %d:%s", m_pcName, this, iResult, libusb_strerror(iResult));
			}
			else
			{
				/* Get the response. */
				iResult = usb_getNetxData(&tBuffer, &tLuaFn, lCallbackUserData);
				if( iResult!=LIBUSB_SUCCESS )
				{
					MUHKUH_PLUGIN_PUSH_ERROR(tLuaFn.L, "%s(%p): failed to get response from netx", m_pcName, this);
				}
				else
				{
					if( expect_string(&tBuffer, "\n>")!=true )
					{
						MUHKUH_PLUGIN_PUSH_ERROR(tLuaFn.L, "%s(%p): strange response from netx: %s", m_pcName, this, tBuffer.pucData);
					}
					else
					{
						fOk = true;
					}

					free(tBuffer.pucData);
				}
			}
		}
	}

	if( fOk!=true )
	{
		MUHKUH_PLUGIN_EXIT_ERROR(tLuaFn.L);
	}
}


/* call routine */
void romloader_usb::call(unsigned long ulNetxAddress, unsigned long ulParameterR0, SWIGLUA_REF tLuaFn, long lCallbackUserData)
{
	int iResult;
	bool fOk;


	fOk = false;

	if( m_fIsConnected==false )
	{
		MUHKUH_PLUGIN_PUSH_ERROR(tLuaFn.L, "%s(%p): not connected!", m_pcName, this);
	}
	else
	{
		if( m_tChiptyp==ROMLOADER_CHIPTYP_NETX10 )
		{
			iResult = usb_call_netx10(ulNetxAddress, ulParameterR0, &tLuaFn, lCallbackUserData);
		}
		else
		{
			iResult = usb_call(ulNetxAddress, ulParameterR0, &tLuaFn, lCallbackUserData);
		}

		if( iResult!=LIBUSB_SUCCESS )
		{
			MUHKUH_PLUGIN_PUSH_ERROR(tLuaFn.L, "%s(%p): failed to execute call: %d:%s", m_pcName, this, iResult, libusb_strerror(iResult));
		}
		else
		{
			fOk = true;
		}
	}

	if( fOk!=true )
	{
		MUHKUH_PLUGIN_EXIT_ERROR(tLuaFn.L);
	}
}


/*-------------------------------------*/


int romloader_usb::usb_load(const unsigned char *pucData, size_t sizDataLen, unsigned long ulLoadAdr, SWIGLUA_REF *ptLuaFn, long lCallbackUserData)
{
	const unsigned char *pucDataCnt, *pucDataEnd;
	int iResult;
	unsigned int uiCrc;
	char acCommand[28];
	unsigned char aucBufSend[64];
	unsigned char aucBufRec[64];
	size_t sizChunkSize;
	bool fIsRunning;
	long lBytesProcessed;


	pucDataCnt = pucData;
	pucDataEnd = pucDataCnt + sizDataLen;
	// generate crc checksum
	uiCrc = 0xffff;
	// loop over all bytes
	while( pucDataCnt<pucDataEnd )
	{
		uiCrc = crc16(uiCrc, *(pucDataCnt++));
	}

	// construct the command
	snprintf(acCommand, sizeof(acCommand), "LOAD %08lX %08X %04X", ulLoadAdr, sizDataLen, uiCrc);

	// send the command
	iResult = usb_sendCommand(acCommand);
	if( iResult==LIBUSB_SUCCESS )
	{
		// now send the data part
		pucDataCnt = pucData;
		lBytesProcessed = 0;
		while( pucDataCnt<pucDataEnd )
		{
			// get the size of the next data chunk
			sizChunkSize = pucDataEnd - pucDataCnt;
			if( sizChunkSize>63 )
			{
				sizChunkSize = 63;
			}
			// copy data to the packet
			memcpy(aucBufSend+1, pucDataCnt, sizChunkSize);
			aucBufSend[0] = sizChunkSize+1;

			fIsRunning = callback_long(ptLuaFn, lBytesProcessed, lCallbackUserData);
			if( fIsRunning!=true )
			{
				printf("***********************************\n");
				printf("*** operation canceled by user! ***\n");
				printf("***********************************\n");
				iResult = LIBUSB_ERROR_INTERRUPTED;
				break;
			}

			iResult = libusb_exchange(aucBufSend, aucBufRec);
			if( iResult!=LIBUSB_SUCCESS )
			{
				break;
			}
			pucDataCnt += sizChunkSize;
			lBytesProcessed += sizChunkSize;
		}

		if( pucDataCnt==pucDataEnd )
		{
			iResult = LIBUSB_SUCCESS;
		}
	}

	return iResult;
}


int romloader_usb::usb_call(unsigned long ulNetxAddress, unsigned long ulParameterR0, SWIGLUA_REF *ptLuaFn, long lCallbackUserData)
{
	int iResult;
	char acCommand[23];
	unsigned char aucSend[64];
	unsigned char aucRec[64];
	bool fIsRunning;
	size_t sizProgressData;
	unsigned char aucSbuf[2] = { 0, 0 };
	char *pcCallbackData;


	/* Construct the command. */
	snprintf(acCommand, sizeof(acCommand), "CALL %08lX %08X", ulNetxAddress, ulParameterR0);

	/* Send the command. */
	iResult = usb_sendCommand(acCommand);
	if( iResult==LIBUSB_SUCCESS )
	{
		aucRec[0] = 0x00;
		pcCallbackData = (char*)(aucRec+1);

		// wait for the call to finish
		do
		{
			// send handshake
			aucSend[0] = 0x00;
			iResult = libusb_writeBlock(aucSend, 64, 200);
			if( iResult==LIBUSB_SUCCESS )
			{
				do
				{
					// execute callback
					sizProgressData = aucRec[0];
//					printf("raw data len: %d\n", sizProgressData);
					if( sizProgressData>1 && sizProgressData<=64 )
					{
						--sizProgressData;
					}
					else
					{
						sizProgressData = 0;
					}
//					printf("run callback with %d bytes of data\n", sizProgressData);
					fIsRunning = callback_string(ptLuaFn, pcCallbackData, sizProgressData, lCallbackUserData);
					aucRec[0] = 0x00;
					if( fIsRunning!=true )
					{
						iResult = LIBUSB_ERROR_INTERRUPTED;
					}
					else
					{
						// look for data from netx
						iResult = libusb_readBlock(aucRec, 64, 200);
						if( iResult==LIBUSB_SUCCESS )
						{
//							printf("received a packet!\n");
//							hexdump(aucRec, 64, 0);

							iResult = LIBUSB_ERROR_TIMEOUT;

							// received netx data, check for prompt
							sizProgressData = aucRec[0];
							if( sizProgressData>1 && sizProgressData<=64 )
							{
								// last packet has '\n>' at the end
								if( sizProgressData>2 && aucRec[sizProgressData-2]=='\n' && aucRec[sizProgressData-1]=='>' )
								{
									// send the rest of the data
									callback_string(ptLuaFn, pcCallbackData, sizProgressData-1, lCallbackUserData);
									iResult = LIBUSB_SUCCESS;
								}
							}
							break;
						}
					}
				} while( iResult==LIBUSB_ERROR_TIMEOUT );
			}
		} while( iResult==LIBUSB_ERROR_TIMEOUT );
	}

	if( iResult==LIBUSB_SUCCESS )
	{
		aucSend[0] = 0x00;
		iResult = libusb_exchange(aucSend, aucRec);
	}

	return iResult;
}


int romloader_usb::usb_call_netx10(unsigned long ulNetxAddress, unsigned long ulParameterR0, SWIGLUA_REF *ptLuaFn, long lCallbackUserData)
{
	int iResult;
	char acCommand[23];
	unsigned char aucRec[64];
	bool fIsRunning;
	size_t sizProgressData;
	unsigned char aucSbuf[2] = { 0, 0 };
	const char *pcCallbackData;
	int iProcessed;


	/* Construct the command. */
	snprintf(acCommand, sizeof(acCommand), "g %08lX %08X\r", ulNetxAddress, ulParameterR0);

	/* Send the command. */
	iResult = usb_sendCommand(acCommand);
	if( iResult==LIBUSB_SUCCESS )
	{
		aucRec[0] = 0x00;
		pcCallbackData = (const char*)aucRec;

		/* Wait for the call to finish. */
		do
		{
			iResult = usb_bulk_netx_to_pc(m_ptUsbDevHandle, 0x83, aucRec, sizeof(aucRec), &iProcessed, 200);
			if( iResult==LIBUSB_SUCCESS )
			{
				/* Execute callback. */
//				printf("raw data len: %d\n", sizProgressData);
				if( iProcessed>0 && iProcessed<=64 )
				{
					printf("run callback with %d bytes of data\n", iProcessed);
					fIsRunning = callback_string(ptLuaFn, pcCallbackData, sizProgressData, lCallbackUserData);
					if( fIsRunning!=true )
					{
						iResult = LIBUSB_ERROR_INTERRUPTED;
					}
					/* Last packet has '\n>' at the end. */
					else if( iProcessed>2 && aucRec[iProcessed-2]=='\n' && aucRec[iProcessed-1]=='>' )
					{
						iResult = LIBUSB_SUCCESS;
						break;
					}
				}
			}
		} while( iResult!=LIBUSB_SUCCESS && iResult!=LIBUSB_ERROR_TIMEOUT );
	}

	return iResult;
}


int romloader_usb::usb_sendCommand(const char *pcCommand)
{
	int iResult;
	size_t sizCmdLen;
	unsigned char abSend[64];
	unsigned char abRec[64];
	unsigned char *pucData;
	int iProcessed;


	if( m_tChiptyp==ROMLOADER_CHIPTYP_NETX10 )
	{
		sizCmdLen = strlen(pcCommand);
		if( sizCmdLen>64 )
		{
			printf("%s(%p): command exceeds maximum length of 64 chars: %s\n", m_pcName, this, pcCommand);
			iResult = LIBUSB_ERROR_OVERFLOW;
		}
		else
		{
			pucData = (unsigned char*)pcCommand;
			iResult = usb_bulk_pc_to_netx(m_ptUsbDevHandle, 0x04, pucData, sizCmdLen, &iProcessed, 100);
		}
	}
	else
	{
		/* Check the command size.
		 * Commands must fit into one usb packet of 64 bytes, the first byte
		 * is the length and the last byte must be 0x0a. This means the max
		 * command size is 62 bytes.
		 */
		sizCmdLen = strlen(pcCommand);
		if( sizCmdLen>62 )
		{
			printf("%s(%p): command exceeds maximum length of 62 chars: %s\n", m_pcName, this, pcCommand);
			iResult = LIBUSB_ERROR_OVERFLOW;
		}
		else
		{
			/* Construct the command. */
			memcpy(abSend+1, pcCommand, sizCmdLen);
			abSend[0] = sizCmdLen+2;
			abSend[sizCmdLen+1] = 0x0a;

			/* Send the command. */
			iResult = libusb_exchange(abSend, abRec);
			if( iResult==LIBUSB_SUCCESS )
			{
				/* Terminate command. */
				abSend[0] = 0x00;
				iResult = libusb_exchange(abSend, abRec);
			}
		}
	}

	return iResult;
}


int romloader_usb::usb_getNetxData(DATA_BUFFER_T *ptBuffer, SWIGLUA_REF *ptLuaFn, long lCallbackUserData, size_t sizInitialSize)
{
	int iResult;
	unsigned char aucSendBuf[64];
	unsigned char aucRecBuf[64];
	size_t sizChunk;
	size_t sizBuffer;
	size_t sizNewBuffer;
	size_t sizBufferPos;
	unsigned char *pucBuffer;
	unsigned char *pucNewBuffer;
	int iTransfered;
	unsigned char *pucInputData;


	/* The buffer is empty. */
	sizBufferPos = 0;

	/* Clear the send buffer. */
	memset(aucSendBuf, 0, sizeof(aucSendBuf));

	/* Init buffer with the initial size. */
	sizBuffer = sizInitialSize;
	pucBuffer = (unsigned char*)malloc(sizBuffer);
	if( pucBuffer==NULL )
	{
		iResult = LIBUSB_ERROR_NO_MEM;
	}
	else
	{
		/* Receive netx data. */
		do
		{
			if( m_tChiptyp==ROMLOADER_CHIPTYP_NETX10 )
			{
				iResult = usb_bulk_netx_to_pc(m_ptUsbDevHandle, 0x83, aucRecBuf, 64, &iTransfered, 500);
				pucInputData = aucRecBuf;
				sizChunk = iTransfered;
				if( iResult==LIBUSB_ERROR_TIMEOUT )
				{
					iResult = LIBUSB_SUCCESS;
					sizChunk = 0;
				}
			}
			else
			{
				iResult = libusb_exchange(aucSendBuf, aucRecBuf);
				pucInputData = aucRecBuf + 1;
				sizChunk = aucRecBuf[0];
			}
			if( iResult!=LIBUSB_SUCCESS )
			{
				break;
			}
			else if( sizChunk!=0 )
			{
				/* Remove the size byte from the data length. */
				--sizChunk;

				/* Is still enough space in the buffer left? */
				if( sizBufferPos+sizChunk>sizBuffer )
				{
					/* No -> double the buffer. */
					sizNewBuffer = sizBuffer * 2;
					if( sizBuffer>sizNewBuffer )
					{
						iResult = LIBUSB_ERROR_NO_MEM;
						break;
					}
					else
					{
						pucNewBuffer = (unsigned char*)realloc(pucBuffer, sizNewBuffer);
						if( pucNewBuffer==NULL )
						{
							/* Out of memory, */
							iResult = LIBUSB_ERROR_NO_MEM;
							break;
						}
						else
						{
							pucBuffer = pucNewBuffer;
							sizBuffer = sizNewBuffer;
						}
					}
				}

				/* Copy the new data chunk to the buffer. */
				memcpy(pucBuffer+sizBufferPos, pucInputData, sizChunk);
				sizBufferPos += sizChunk;
			}
		} while( sizChunk!=0 );

		if( iResult==LIBUSB_SUCCESS )
		{
			/* Shrink buffer to optimal size. */
			if( sizBufferPos<sizBuffer )
			{
				/* Do not resize with length=0, this would free the memory! */
				if( sizBufferPos==0 )
				{
					sizBufferPos = 1;
				}

				pucNewBuffer = (unsigned char*)realloc(pucBuffer, sizBufferPos);
				if( pucNewBuffer!=NULL )
				{
					pucBuffer = pucNewBuffer;
					sizBuffer = sizBufferPos;
				}
			}
		}
		else
		{
			if( pucBuffer!=NULL )
			{
				free(pucBuffer);
				pucBuffer = NULL;
				sizBufferPos = 0;
			}
		}
	}

	ptBuffer->pucData = pucBuffer;
	ptBuffer->sizData = sizBufferPos;
	ptBuffer->sizPos = 0;

	return iResult;
}


int romloader_usb::usb_executeCommand(const char *pcCommand, DATA_BUFFER_T *ptBuffer)
{
	int iResult;
	SWIGLUA_REF tRef;


	tRef.L = NULL;
	tRef.ref = 0;

	/* send the command */
	iResult = usb_sendCommand(pcCommand);
	if( iResult==LIBUSB_SUCCESS )
	{
	
		usleep(10000);
	
	
		/* get the response */
		iResult = usb_getNetxData(ptBuffer, &tRef, 0);
	}

	return iResult;
}


/*-------------------------------------*/


int romloader_usb::libusb_closeDevice(void)
{
	int iResult;


	/* release the interface */
	iResult = libusb_release_interface(m_ptUsbDevHandle, 0);
	if( iResult!=LIBUSB_SUCCESS )
	{
		/* failed to release interface */
		printf("%s(%p): failed to release the usb interface: %d:%s\n", m_pcName, this, iResult, libusb_strerror(iResult));
	}
	else
	{
		/* close the netx device */
		libusb_close(m_ptUsbDevHandle);
	}

	return iResult;
}


int romloader_usb::libusb_resetDevice(void)
{
	int iResult;


	iResult = libusb_reset_device(m_ptUsbDevHandle);
	if( iResult==LIBUSB_SUCCESS )
	{
		libusb_close(m_ptUsbDevHandle);
	}
	else if( iResult==LIBUSB_ERROR_NOT_FOUND )
	{
		// the old device is already gone -> that's good, ignore the error
		libusb_close(m_ptUsbDevHandle);
		iResult = LIBUSB_SUCCESS;
	}

	return iResult;
}


int romloader_usb::libusb_readBlock(unsigned char *pucReceiveBuffer, unsigned int uiSize, int iTimeoutMs)
{
	int iRet;
	int iSize;
	int iTransfered;
	unsigned char ucEndPoint;


	iSize = (int)uiSize;

	if( m_tChiptyp==ROMLOADER_CHIPTYP_NETX10 )
	{
		ucEndPoint = 0x83;
	}
	else
	{
		ucEndPoint = 0x81;
	}
	iRet = usb_bulk_netx_to_pc(m_ptUsbDevHandle, ucEndPoint, pucReceiveBuffer, iSize, &iTransfered, iTimeoutMs);
	return iRet;
}


int romloader_usb::libusb_writeBlock(unsigned char *pucSendBuffer, unsigned int uiSize, int iTimeoutMs)
{
	int iRet;
	int iSize;
	int iTransfered;
	unsigned char ucEndPoint;


	iSize = (int)uiSize;

	if( m_tChiptyp==ROMLOADER_CHIPTYP_NETX10 )
	{
		ucEndPoint = 0x04;
	}
	else
	{
		ucEndPoint = 0x01;
	}
	iRet = usb_bulk_pc_to_netx(m_ptUsbDevHandle, ucEndPoint, pucSendBuffer, iSize, &iTransfered, iTimeoutMs);
	return iRet;
}


int romloader_usb::libusb_exchange(unsigned char *pucSendBuffer, unsigned char *pucReceiveBuffer)
{
	int iResult;


	iResult = libusb_writeBlock(pucSendBuffer, 64, 200);
	if( iResult==LIBUSB_SUCCESS )
	{
		iResult = libusb_readBlock(pucReceiveBuffer, 64, 200);
	}
	return iResult;
}


void romloader_usb::hexdump(const unsigned char *pucData, unsigned long ulSize, unsigned long ulNetxAddress)
{
	const unsigned char *pucDumpCnt, *pucDumpEnd;
	unsigned long ulAddressCnt;
	unsigned long ulSkipOffset;
	size_t sizBytesLeft;
	size_t sizChunkSize;
	size_t sizChunkCnt;


	// show a hexdump of the data
	pucDumpCnt = pucData;
	pucDumpEnd = pucData + ulSize;
	ulAddressCnt = ulNetxAddress;
	while( pucDumpCnt<pucDumpEnd )
	{
		// get number of bytes for the next line
		sizChunkSize = 16;
		sizBytesLeft = pucDumpEnd - pucDumpCnt;
		if( sizChunkSize>sizBytesLeft )
		{
			sizChunkSize = sizBytesLeft;
		}

		// start a line in the dump with the address
		printf("%08lX: ", ulAddressCnt);
		// append the data bytes
		sizChunkCnt = sizChunkSize;
		while( sizChunkCnt!=0 )
		{
			printf("%02X ", *(pucDumpCnt++));
			--sizChunkCnt;
		}
		// next line
		printf("\n");
		ulAddressCnt += sizChunkSize;
	}
}


/*-------------------------------------*/


romloader_usb_reference::romloader_usb_reference(void)
 : muhkuh_plugin_reference()
{
}


romloader_usb_reference::romloader_usb_reference(const char *pcName, const char *pcTyp, bool fIsUsed, romloader_usb_provider *ptProvider)
 : muhkuh_plugin_reference(pcName, pcTyp, fIsUsed, ptProvider)
{
}


romloader_usb_reference::romloader_usb_reference(const romloader_usb_reference *ptCloneMe)
 : muhkuh_plugin_reference(ptCloneMe)
{
}


/*-------------------------------------*/





