/**
 * @file test15_4_phy_tests_mac.c
 * @brief PHY Test Functions using MAC Functions for Data Reliablity
 * @author Wolfgang Bruchner
 * @date 19/07/14
 *//*
 * Copyright (C) 2016  Cascoda, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>

#include "ca821x_api.h"

#include "test15_4_phy_tests.h"

extern int (*test15_4_upstream)(
	const uint8_t *buf,
	size_t len,
	void *pDeviceRef
);

/******************************************************************************/
/****** Address/PID Definitions                                          ******/
/******************************************************************************/
#define  PHY_PANID         0xCA5C
#define  PHY_TX_SHORTADD   0xCA51
#define  PHY_RX_SHORTADD   0xCA52
#define  PHY_TX_LONGADD    {0x01, 0x00, 0x00, 0x00, 0xA0, 0x0D, 0x5C, 0xCA}
#define  PHY_RX_LONGADD    {0x02, 0x00, 0x00, 0x00, 0xA0, 0x0D, 0x5C, 0xCA}


/******************************************************************************/
/****** Global Variables from PHY Tests                                  ******/
/******************************************************************************/
extern uint8_t PHYBuffer[256];
extern uint8_t PHYLength;


/******************************************************************************/
/****** Global Variables only used in Phy Test Files                     ******/
/******************************************************************************/
struct FullAddr SrcFAdd, DstFAdd;

uint16_t   PHYPANId;
uint16_t   PHYTxShortAddress;
uint16_t   PHYRxShortAddress;
uint8_t    PHYTxLongAddress[8];
uint8_t    PHYRxLongAddress[8];

uint8_t    DSN_OLD;


/******************************************************************************/
/***************************************************************************//**
 * \brief Address Initialisation
 *******************************************************************************
 ******************************************************************************/
void PHYTestMACAddInit(void)
{
	PHYPANId           = PHY_PANID;
	PHYTxShortAddress  = PHY_TX_SHORTADD;
	PHYRxShortAddress  = PHY_RX_SHORTADD;
	memcpy(PHYTxLongAddress, (uint8_t[])PHY_TX_LONGADD, 8);
	memcpy(PHYRxLongAddress, (uint8_t[])PHY_RX_LONGADD, 8);
}


/******************************************************************************/
/***************************************************************************//**
 * \brief Sequential Initialisation for using MAC layer in EVBME (Tx)
 *******************************************************************************
 * \param pDeviceRef - Device Reference
 *******************************************************************************
 * \return Status
 *******************************************************************************
 ******************************************************************************/
uint8_t PHYTestMACTxInitialise(struct ca821x_dev *pDeviceRef)
{
	uint8_t status;

	PHYTestMACAddInit();

	if ((status = TDME_TESTMODE_request_sync(TDME_TEST_OFF, pDeviceRef)))                        // turn off test mode in order to be able to use MAC
		return status;

	if ((status = TDME_SETSFR_request_sync(0, 0xB2, 0x00, pDeviceRef)))                          // turn off AUTED
		return status;

    if ((status = MLME_RESET_request_sync(0, pDeviceRef)))
		return status;

	PHYBuffer[0] = CCAM_EDORCS;
	if ((status = HWME_SET_request_sync(HWME_CCAMODE, 1, PHYBuffer, pDeviceRef)))                // set CCA mode to ED OR CS
		return status;

	PHYBuffer[0] = PHY_TESTPAR.EDTHRESHOLD;
	if ((status = HWME_SET_request_sync(HWME_EDTHRESHOLD, 1, PHYBuffer, pDeviceRef)))            // set ED threshold to PHY_TESTPAR.EDTHRESHOLD
		return status;

	if ((status = MLME_SET_request_sync(nsIEEEAddress,   0, 8, PHYTxLongAddress, pDeviceRef)))   // set local long address
		return status;

	if ((status = MLME_SET_request_sync(macShortAddress, 0, 2, &PHYTxShortAddress, pDeviceRef))) // set local short address
		return status;

	status = MLME_START_request_sync(
		PHYPANId,                  /* PANId */
		PHY_TESTPAR.CHANNEL,       /* LogicalChannel */
		15,                        /* BeaconOrder */
		15,                        /* SuperframeOrder */
		1,                         /* PANCoordinator */
		0,                         /* BatteryLifeExtension */
		1,                         /* CoordRealignment */
		NULL,                      /* *pCoordRealignSecurity */
		NULL,                      /* *pBeaconSecurity */
		pDeviceRef);
	if (status)
		return status;

	PHYBuffer[0] = 0;
	if ((status = MLME_SET_request_sync(macRxOnWhenIdle, 0, 1, PHYBuffer, pDeviceRef)))          /* turn receiver off */
		return status;

	return status;
} // End of PHYTestMACTxInitialise()


/******************************************************************************/
/***************************************************************************//**
 * \brief Sequential Initialisation for using MAC layer in EVBME (Rx)
 *******************************************************************************
 * \param pDeviceRef - Device Reference
 *******************************************************************************
 * \return Status
 *******************************************************************************
 ******************************************************************************/
uint8_t PHYTestMACRxInitialise(struct ca821x_dev *pDeviceRef)
{
	uint8_t status;

	PHYTestMACAddInit();

	if ((status = TDME_TESTMODE_request_sync(TDME_TEST_OFF, pDeviceRef)))                        // turn off test mode in order to be able to use MAC
		return status;

	if ((status = TDME_SETSFR_request_sync(0, 0xB2, 0x08, pDeviceRef)))                          // turn on AUTED
		return status;

    if ((status = MLME_RESET_request_sync(0, pDeviceRef)))
		return status;

	if ((status = MLME_SET_request_sync(macPANId,        0, 2, &PHYPANId, pDeviceRef)))          // set local PANId
		return status;

	if ((status = MLME_SET_request_sync(nsIEEEAddress,   0, 8, PHYRxLongAddress, pDeviceRef)))   // set local long address
		return status;

	if ((status = MLME_SET_request_sync(macShortAddress, 0, 2, &PHYRxShortAddress, pDeviceRef))) // set local short address
		return status;

	PHYBuffer[0] = 1;
	if ((status = MLME_SET_request_sync(macRxOnWhenIdle, 0, 1, PHYBuffer, pDeviceRef)))          // turn receiver on
		return status;

	DSN_OLD = 0;

	return status;
} // End of PHYTestMACRxInitialise()


/******************************************************************************/
/****** PHY_TXPKT_MAC_request()                                          ******/
/******************************************************************************/
/****** Brief:  PHY Test Wrapper for MCPS_DATA_request()                 ******/
/******************************************************************************/
/****** Param:  -                                                        ******/
/******************************************************************************/
/****** Return: Status                                                   ******/
/******************************************************************************/
/******************************************************************************/
uint8_t PHY_TXPKT_MAC_request(struct ca821x_dev *pDeviceRef)
{
	uint8_t i;
	uint8_t status;
	uint8_t randnum[2];
	uint8_t attlen;
	struct MAC_Message rx_msg;

	PHYLength    =  PHY_TESTPAR.PACKETLENGTH;
	PHYBuffer[1] =  PHY_TESTRES.SEQUENCENUMBER;

	if (PHY_TESTPAR.PACKETDATATYPE == TDME_TXD_APPENDED) {
		for (i = 0; i < PHY_TESTPAR.PACKETLENGTH; ++i) {
			PHYBuffer[i+2] = 0x00; // currently filled with 0's
		}
	} else if (PHY_TESTPAR.PACKETDATATYPE == TDME_TXD_RANDOM) {
		for (i = 0; i < PHY_TESTPAR.PACKETLENGTH; ++i) {
			if ((status = HWME_GET_request_sync(HWME_RANDOMNUM, &attlen, randnum, pDeviceRef)))
				return status;
			else
				PHYBuffer[i+2] = randnum[0];
		}
	} else if (PHY_TESTPAR.PACKETDATATYPE == TDME_TXD_SEQRANDOM) {
		PHYBuffer[2] = PHY_TESTRES.SEQUENCENUMBER;
		for (i = 1; i < PHY_TESTPAR.PACKETLENGTH; ++i) {
			if ((status = HWME_GET_request_sync(HWME_RANDOMNUM, &attlen, randnum, pDeviceRef)))
				return(status);
			else
				PHYBuffer[i+2] = randnum[0];
		}
	} else { /* PHY_TESTPAR.PACKETDATATYPE == TDME_TXD_COUNT) */
		for (i = 0; i < PHY_TESTPAR.PACKETLENGTH; ++i) {
			PHYBuffer[i+2] = i+1;
		}
	}

	DstFAdd.AddressMode = MAC_MODE_SHORT_ADDR;
	PUTLE16(PHYPANId, DstFAdd.PANId);
	PUTLE16(PHYRxShortAddress, DstFAdd.Address);

	MCPS_DATA_request(
		MAC_MODE_SHORT_ADDR,                  /* SrcAddrMode */
		DstFAdd,                              /* DstAddr */
		PHYLength,                            /* MsduLength */
		PHYBuffer+2,                          /* *pMsdu */
		LS_BYTE(PHY_TESTRES.SEQUENCENUMBER),  /* MsduHandle */
		TXOPT_ACKREQ,                         /* TxOptions */
		NULL,                                /* *pSecurity */
		pDeviceRef
	);

	if((status = ca821x_wait_for_message(SPI_MCPS_DATA_CONFIRM, 5000, &rx_msg.CommandId, pDeviceRef)))
     return(status);

	status = rx_msg.PData.DataCnf.Status;

	if(status == MAC_NO_ACK)
	{
		++PHY_TESTRES.SHRERR_COUNT; // SHRERR_COUNT used for counting NO_ACK
		status = MAC_SUCCESS;
	}
	else if(status == MAC_CHANNEL_ACCESS_FAILURE)
	{
		++PHY_TESTRES.PHRERR_COUNT; // PHRERR_COUNT used for counting CHANNEL_ACCESS_FAILURE
		status = MAC_SUCCESS;
	}

	++PHY_TESTRES.SEQUENCENUMBER;

	return(status);
} // End of PHY_TXPKT_MAC_request()


/******************************************************************************/
/***************************************************************************//**
 * \brief PHY Test Wrapper for MCPS_DATA_indication()
 *******************************************************************************
 * \param data_ind - MCPS data indication buffer
 * \param pDeviceRef - Device Reference
 *******************************************************************************
 * \return Status
 *******************************************************************************
 ******************************************************************************/
uint8_t PHY_RXPKT_MAC_indication(struct MAC_Message *data_ind, struct ca821x_dev *pDeviceRef)
{
	uint8_t status = 0;
	uint8_t MsduLength, MpduLinkQuality, DSN;
	uint8_t edvallp, freqoffs;
	uint8_t len;

	DSN = data_ind->PData.DataInd.DSN;
	MpduLinkQuality = data_ind->PData.DataInd.MpduLinkQuality;
	MsduLength = data_ind->PData.DataInd.MsduLength;

	if (test15_4_upstream) {
		test15_4_upstream(&data_ind->CommandId, data_ind->Length + 2, pDeviceRef);
	}

	if (!status) {
		if (DSN == DSN_OLD) { /* check if same sequence number - discard if this is the case */
			status = MAC_INVALID_HANDLE;
		} else {
			PHY_TESTRES.PACKET_RECEIVED = 1; /* Flag indication */
		}
	}

	DSN_OLD = DSN;

	if (!status)
		status = HWME_GET_request_sync(HWME_EDVALLP,  &len, &edvallp, pDeviceRef);

	if (!status)
		status = HWME_GET_request_sync(HWME_FREQOFFS, &len, &freqoffs, pDeviceRef);

	PHYBuffer[0] = status;
	PHYBuffer[1] = edvallp;
	PHYBuffer[2] = MpduLinkQuality;
	PHYBuffer[3] = freqoffs;
	PHYBuffer[4] = MsduLength;
	PHYLength    = MsduLength;

	memcpy(
		&PHYBuffer[4],
		data_ind->PData.DataInd.Msdu,
		MsduLength);

	return status;
} // End of PHY_RXPKT_MAC_indication()
