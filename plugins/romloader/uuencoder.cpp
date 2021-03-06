/***************************************************************************
 *   Copyright (C) 2010 by Christoph Thelen                                *
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


#include "uuencoder.h"

#include <stdio.h>

#if defined(_MSC_VER)
#	define snprintf _snprintf
#endif

uuencoder::uuencoder(void)
{
	m_pucStart = NULL;
	m_pucCnt   = NULL;
	m_pucEnd   = NULL;
	m_tState   = UUENCODE_STATE_Finished;
}


void uuencoder::set_data(const uint8_t *pucData, size_t sizData)
{
	m_pucStart = pucData;
	m_pucCnt   = pucData;
	m_pucEnd   = pucData + sizData;
	m_tState   = UUENCODE_STATE_Begin;
}


size_t uuencoder::process(char *pcLine, size_t sizMaxLine)
{
	size_t sizLine;
	size_t sizChunk;
	const uint8_t *pucInputCnt;
	char *pcOutputCnt;
	uint32_t ulUueBuf;
	int iCnt;


	/* Default is no output. */
	sizLine = 0;

	switch( m_tState )
	{
	case UUENCODE_STATE_Begin:
		sizLine = snprintf(pcLine, sizMaxLine, "begin 666 -\n");
		m_tState = UUENCODE_STATE_Data;
		break;

	case UUENCODE_STATE_Data:
		/* Get the size of the next chunk in raw bytes. */
		sizChunk = m_pucEnd - m_pucCnt;
		/* Limit the chunk size to the maximum line size for UUCODE. */
		if( sizChunk>45 )
		{
			sizChunk = 45;
		}

		/* Get a pointer to the output buffer. */
		pcOutputCnt = pcLine;

		/* Print the length character for the line. */
		*(pcOutputCnt++) = (char)(0x20 + sizChunk);

		/* Convert the raw bytes to UUCODE. */
		pucInputCnt = m_pucCnt;
		do
		{
			/* Clear UUENCODE buffer. */
			ulUueBuf = 0;

			/* Get max 3 chars into the buffer. */
			iCnt = 3;
			do
			{
				/* Still bytes left? */
				if( sizChunk>0 )
				{
					ulUueBuf |= *(pucInputCnt++);
					--sizChunk;
				}
				/* NOTE: The shift operation must be executed after the new data is masked in, the result must be in 8..31 . */
				ulUueBuf <<= 8;
			} while( --iCnt>0 );

			/* Encode the buffer. */
			iCnt = 4;
			do
			{
				*(pcOutputCnt++) = (char)(0x20U + ((ulUueBuf>>26U)&0x3fU));
				ulUueBuf <<= 6U;
			} while( --iCnt!=0 );
		} while( sizChunk!=0 );

		/* End the line. */
		*(pcOutputCnt++) = '`';
		*(pcOutputCnt++) = '\n';

		/* Get the line size. */
		sizLine = pcOutputCnt - pcLine;

		/* Check if all data is processed. */
		m_pucCnt = pucInputCnt;
		if( pucInputCnt>=m_pucEnd )
		{
			m_tState = UUENCODE_STATE_Last1;
		}
		break;

	case UUENCODE_STATE_Last1:
		sizLine = snprintf(pcLine, sizMaxLine, "`\n");
		m_tState = UUENCODE_STATE_Last2;
		break;

	case UUENCODE_STATE_Last2:
		sizLine = snprintf(pcLine, sizMaxLine, "end\n");
		m_tState = UUENCODE_STATE_Finished;
		break;

	case UUENCODE_STATE_Finished:
		sizLine = 0;
		break;
	}

	return sizLine;
}


void uuencoder::get_progress_info(UUENCODER_PROGRESS_INFO_T *ptProgressInfo)
{
	size_t sizTotal;
	size_t sizProcessed;
	unsigned int uiPercent;


	switch( m_tState )
	{
	case UUENCODE_STATE_Begin:
	case UUENCODE_STATE_Data:
	case UUENCODE_STATE_Last1:
	case UUENCODE_STATE_Last2:
		sizTotal = m_pucEnd - m_pucStart;
		sizProcessed = m_pucCnt - m_pucStart;
		if( sizTotal!=0 )
		{
			uiPercent = sizProcessed*100 / sizTotal;
		}
		else
		{
			uiPercent = 0;
		}
		break;

	case UUENCODE_STATE_Finished:
		sizTotal = 0;
		sizProcessed = 0;
		uiPercent = 0;
		break;
	}

	ptProgressInfo->sizTotal = sizTotal;
	ptProgressInfo->sizProcessed = sizProcessed;
	ptProgressInfo->uiPercent = uiPercent;
}


bool uuencoder::isFinished(void) const
{
	bool fResult;


	fResult = (m_tState==UUENCODE_STATE_Finished);

	return fResult;
}

