////////////////////////////////////////////////////////////////////////////////
////                                                                        ////
////                         KSE DIM API for Ubuntu                         ////
////                                                                        ////
////          Copyright (C) 2017,2019 Keypair All rights reserved.          ////
////                                                                        ////
////////////////////////////////////////////////////////////////////////////////
//// Module Name    : kse_ubuntu.c                                          ////
//// Description    : KSE DIM API for Windows                               ////
//// Programmer     : Jung-Youp Lee                                         ////
//// OS             : Ubuntu 18.04.2 LTS                                    ////
//// Compiler       : gcc (Ubuntu 7.4.0-1ubuntu1~18.04.1) 7.4.0             ////
////                  with libusb-1.0-0(2:1.0.21-2)                         ////
//// Last Update    : 2019.11.21                                            ////
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//// Define Conditions /////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// Select or not.
#define ENABLE_DEBUG_PRINT
//#define USB_INTERRUPT_TRANSFER


////////////////////////////////////////////////////////////////////////////////
//// Include Header Files //////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <libusb-1.0/libusb.h>

#include "kse_ubuntu.h"


////////////////////////////////////////////////////////////////////////////////
//// Declare Global Constants //////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//// Debug /////////////////////////////////////////////////////////////////////

//// Network Communication /////////////////////////////////////////////////////

//// USB Communication /////////////////////////////////////////////////////////

#define INTERFACE_NUMBER  0
#define TIMEOUT_MS        5000
#define IN_ENDPOINT       0x81
#define OUT_ENDPOINT      0x01
#define REPORT_SIZE       64
#define VENDOR_ID         0x25F8  // VID : Keypair
#define PRODUCT_ID        0x1010  // PID : ETRI DIM
#define REP_ONE_BLOCK     0xA5
#define REP_FIRST_BLOCK   0xA1
#define REP_MIDDLE_BLOCK  0x11
#define REP_LAST_BLOCK    0x15

#define MAX_IO_CTRL_LEN     128
#define MAX_IO_BUFFER_SIZE  (MAX_IO_DATA_SIZE + MAX_IO_CTRL_LEN)  // 3072.

//// KSE DIM ///////////////////////////////////////////////////////////////////

#define KSE_POWER_OFF  0
#define KSE_POWER_ON   1

//// KSE ///////////////////////////////////////////////////////////////////////

//// KCMVP /////////////////////////////////////////////////////////////////////

#define KCMVP_AES         0x40
#define KCMVP_ARIA        0x50
#define KCMVP_SHA         0x60
#define KCMVP_HMAC_GEN    0x70
#define KCMVP_HMAC_VERI   0x78
#define KCMVP_ECDSA_SIGN  0x80
#define KCMVP_ECDSA_VERI  0x88
#define KCMVP_ECDH        0x90

//// Cert //////////////////////////////////////////////////////////////////////

//// kseTLS ////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//// Define Data Types /////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//// Declare Global Variables //////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//// Debug /////////////////////////////////////////////////////////////////////

//// Network Communication /////////////////////////////////////////////////////

//// USB Communication /////////////////////////////////////////////////////////

struct libusb_device_handle *ghDev = NULL;
uint8_t gabTxRxData[MAX_IO_BUFFER_SIZE];

//// KSE DIM ///////////////////////////////////////////////////////////////////

uint16_t gusKsePower = KSE_POWER_OFF;

//// KSE ///////////////////////////////////////////////////////////////////////

//// KCMVP /////////////////////////////////////////////////////////////////////

//// Cert //////////////////////////////////////////////////////////////////////

//// kseTLS ////////////////////////////////////////////////////////////////////

uint16_t gusTxRxDataOffset;
uint16_t gusTxRxDataLength;


////////////////////////////////////////////////////////////////////////////////
//// Declare Functions /////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//// Define Functions //////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//// Function Name : _cpy
//// Parameters    : (Output) pbC
////                 (Input)  pbA
////                 (Input)  uSize
//// Return Value  : void
//// Operation     : C = A.
//// Description   : -  None.
static void _cpy(uint8_t *pbC, uint8_t *pbA, int iSize)
{
    if (pbC < pbA)
    {
        while (iSize--)
            *pbC++ = *pbA++;
    }
    else if (pbC > pbA)
    {
        pbC += iSize;
        pbA += iSize;
        while (iSize--)
            *(--pbC) = *(--pbA);
    }
}

//// Debug /////////////////////////////////////////////////////////////////////

#ifdef ENABLE_DEBUG_PRINT // ---------------------------------------------------
//// Function Name : _debugPrintNetTxRxData
//// Parameters    : (Input) bMode
////                 (Input) pbData
////                 (Input) sLen
//// Return Value  : void
//// Operation     : Print network transceive data for debug.
//// Description   : - 'bMode' should be one of belows.
////                    < SENT(0) / RECV(1) >
void _debugPrintNetTxRxData(uint8_t bMode, uint8_t *pbData, int16_t sLen)
{
    int16_t i;

    if (bMode == SENT)
        printf("    + Sent Data:\r\n");
    else
        printf("    + Received Data:\r\n");

    for (i = 0; i < sLen; i++)
    {
        if ((i % 16) == 0)
            printf("      ");
        printf("%02X ", pbData[i]);
        if ((i % 16) == 15)
            printf("\n");
    }
    if ((i % 16) != 0)
        printf("\n");

    if (bMode == SENT)
        printf("    + Sent Data Length: %d\r\n", sLen);
    else
        printf("    + Received Data Length: %d\r\n", sLen);
}

//// Function Name : _debugPrintErrStr
//// Parameters    : (Input) strErrFunc
////                 (Input) sKseErrCode
////                 (Input) iUsbErrCode
//// Return Value  : void
//// Operation     : Print error string for debug.
//// Description   : - 'sKseErrCode' should be positive '6Fxx', negative, or
////                    zero(no error).
////                 - 'iUsbErrCode' should be negative or zero(no error).
void _debugPrintErrStr(const char *strErrFunc, int16_t sKseErrCode,
                       int iUsbErrCode)
{
    char *strKseErr;

    if (sKseErrCode == KSE_FAIL)
        strKseErr = (char *)"Fail";

    else if (sKseErrCode == KSE_FAIL_WRONG_INPUT)
        strKseErr = (char *)"KSE wrong input";
    else if (sKseErrCode == KSE_FAIL_NOT_SUPPORTED)
        strKseErr = (char *)"KSE not supported";
    else if (sKseErrCode == KSE_FAIL_NOT_POWERED_ON)
        strKseErr = (char *)"KSE not powered on";
    else if (sKseErrCode == KSE_FAIL_ALREADY_POWERED_ON)
        strKseErr = (char *)"KSE already powered on";
    else if (sKseErrCode == KSE_FAIL_ATR)
        strKseErr = (char *)"KSE ATR error";
    else if (sKseErrCode == KSE_FAIL_PPS)
        strKseErr = (char *)"KSE PPS error";
    else if (sKseErrCode == KSE_FAIL_TX)
        strKseErr = (char *)"KSE Tx error";
    else if (sKseErrCode == KSE_FAIL_RX)
        strKseErr = (char *)"KSE Rx error";
    else if (sKseErrCode == KSE_FAIL_CHAINING)
        strKseErr = (char *)"KSE chaining error";
    else if (sKseErrCode == KSE_FAIL_APDU_FORMAT)
        strKseErr = (char *)"KSE wrong APDU format";
    else if (sKseErrCode == KSE_FAIL_UNKNOWN_CMD)
        strKseErr = (char *)"KSE unknown command";
    else if (sKseErrCode == KSE_FAIL_STATE)
        strKseErr = (char *)"KSE state error";
    else if (sKseErrCode == KSE_FAIL_PIN_VERIFY)
        strKseErr = (char *)"KSE PIN verification error";
    else if (sKseErrCode == KSE_FAIL_CRYPTO_VERIFY)
        strKseErr = (char *)"KSE crypto verification error";
    else if (sKseErrCode == KSE_FAIL_CERT_VERIFY)
        strKseErr = (char *)"KSE certificate verification error";
    else if ((sKseErrCode & 0xFF00) == 0x6F00)
    {
        printf("%s : [0x%04X] CLIB error.\r\n", strErrFunc, sKseErrCode);
        return;
    }

    else if (sKseErrCode == KSE_FAIL_USB_INIT)
        strKseErr = (char *)"USB initialization error";
    else if (sKseErrCode == KSE_FAIL_USB_NO_DEVICES)
        strKseErr = (char *)"No USB devices";
    else if (sKseErrCode == KSE_FAIL_USB_DEVICE_OPEN)
        strKseErr = (char *)"USB device open error";
    else if (sKseErrCode == KSE_FAIL_USB_DETACH_KERNEL_DRIVER)
        strKseErr = (char *)"USB detach kernel driver error";
    else if (sKseErrCode == KSE_FAIL_USB_CLAIM_INTERFACE)
        strKseErr = (char *)"USB claim interface driver error";
    else if (sKseErrCode == KSE_FAIL_USB_SEND_REPORT)
        strKseErr = (char *)"USB send report error";
    else if (sKseErrCode == KSE_FAIL_USB_RECV_REPORT)
        strKseErr = (char *)"USB receive report error";
    else if (sKseErrCode == KSE_FAIL_NOT_FOUND)
        strKseErr = (char *)"KSE not found";
    else if (sKseErrCode == KSE_FAIL_UNEXPECTED_RESP)
        strKseErr = (char *)"USB unexpected response";
    else if (sKseErrCode == KSE_FAIL_UNEXPECTED_RESP_LEN)
        strKseErr = (char *)"USB unexpected response Length";
    else if (sKseErrCode == KSE_FAIL_RECV_BUF_OVERFLOW)
        strKseErr = (char *)"USB receive buffer overflow";

    else if (sKseErrCode == KSETLS_ERR_NET_CONFIG)
        strKseErr = (char *)"Failed to get ip address! Please check your "
                    "network configuration";
    else if (sKseErrCode == KSETLS_ERR_NET_SOCKET_FAILED)
        strKseErr = (char *)"Failed to open a socket";
    else if (sKseErrCode == KSETLS_ERR_NET_CONNECT_FAILED)
        strKseErr = (char *)"The connection to the given server:port failed";
    else if (sKseErrCode == KSETLS_ERR_NET_BIND_FAILED)
        strKseErr = (char *)"Binding of the socket failed";
    else if (sKseErrCode == KSETLS_ERR_NET_LISTEN_FAILED)
        strKseErr = (char *)"Could not listen on the socket";
    else if (sKseErrCode == KSETLS_ERR_NET_ACCEPT_FAILED)
        strKseErr = (char *)"Could not accept the incoming connection";
    else if (sKseErrCode == KSETLS_ERR_NET_RECV_FAILED)
        strKseErr = (char *)"Reading information from the socket failed";
    else if (sKseErrCode == KSETLS_ERR_NET_SEND_FAILED)
        strKseErr = (char *)"Sending information through the socket failed";
    else if (sKseErrCode == KSETLS_ERR_NET_CONN_RESET)
        strKseErr = (char *)"Connection was reset by peer";
    else if (sKseErrCode == KSETLS_ERR_NET_UNKNOWN_HOST)
        strKseErr = (char *)"Failed to get an IP address for the given "
                    "hostname";
    else if (sKseErrCode == KSETLS_ERR_NET_BUFFER_TOO_SMALL)
        strKseErr = (char *)"Buffer is too small to hold the data";
    else if (sKseErrCode == KSETLS_ERR_NET_INVALID_CONTEXT)
        strKseErr = (char *)"The context is invalid, eg because it was "
                    "free()ed";
    else if (sKseErrCode == KSETLS_ERR_NET_POLL_FAILED)
        strKseErr = (char *)"Polling the net context failed";
    else if (sKseErrCode == KSETLS_ERR_NET_BAD_INPUT_DATA)
        strKseErr = (char *)"Input invalid";

    else if (sKseErrCode == KSETLS_ERR_TLS_FEATURE_UNAVAILABLE)
        strKseErr = (char *)"The requested feature is not available";
    else if (sKseErrCode == KSETLS_ERR_TLS_BAD_INPUT_DATA)
        strKseErr = (char *)"Bad input parameters to function";
    else if (sKseErrCode == KSETLS_ERR_TLS_INVALID_MAC)
        strKseErr = (char *)"Verification of the message MAC failed";
    else if (sKseErrCode == KSETLS_ERR_TLS_INVALID_RECORD)
        strKseErr = (char *)"An invalid SSL record was received";
    else if (sKseErrCode == KSETLS_ERR_TLS_CONN_EOF)
        strKseErr = (char *)"The connection indicated an EOF";
    else if (sKseErrCode == KSETLS_ERR_TLS_UNKNOWN_CIPHER)
        strKseErr = (char *)"An unknown cipher was received";
    else if (sKseErrCode == KSETLS_ERR_TLS_NO_CIPHER_CHOSEN)
        strKseErr = (char *)"The server has no ciphersuites in common with the "
                    "client";
    else if (sKseErrCode == KSETLS_ERR_TLS_NO_RNG)
        strKseErr = (char *)"No RNG was provided to the SSL module";
    else if (sKseErrCode == KSETLS_ERR_TLS_NO_CLIENT_CERTIFICATE)
        strKseErr = (char *)"No client certification received from the client, "
                    "but required by the authentication mode";
    else if (sKseErrCode == KSETLS_ERR_TLS_CERTIFICATE_TOO_LARGE)
        strKseErr = (char *)"Our own certificate(s) is/are too large to send "
                    "in an SSL message";
    else if (sKseErrCode == KSETLS_ERR_TLS_CERTIFICATE_REQUIRED)
        strKseErr = (char *)"The own certificate is not set, but needed by the "
                    "server";
    else if (sKseErrCode == KSETLS_ERR_TLS_PRIVATE_KEY_REQUIRED)
        strKseErr = (char *)"The own private key or pre-shared key is not set, "
                    "but needed";
    else if (sKseErrCode == KSETLS_ERR_TLS_CA_CHAIN_REQUIRED)
        strKseErr = (char *)"No CA Chain is set, but required to operate";
    else if (sKseErrCode == KSETLS_ERR_TLS_UNEXPECTED_MESSAGE)
        strKseErr = (char *)"An unexpected message was received from our peer";
    else if (sKseErrCode == KSETLS_ERR_TLS_FATAL_ALERT_MESSAGE)
        strKseErr = (char *)"A fatal alert message was received from our peer";
    else if (sKseErrCode == KSETLS_ERR_TLS_PEER_VERIFY_FAILED)
        strKseErr = (char *)"Verification of our peer failed";
    else if (sKseErrCode == KSETLS_ERR_TLS_PEER_CLOSE_NOTIFY)
        strKseErr = (char *)"The peer notified us that the connection is going "
                    "to be closed";
    else if (sKseErrCode == KSETLS_ERR_TLS_BAD_HS_CLIENT_HELLO)
        strKseErr = (char *)"Processing of the ClientHello handshake message "
                    "failed";
    else if (sKseErrCode == KSETLS_ERR_TLS_BAD_HS_SERVER_HELLO)
        strKseErr = (char *)"Processing of the ServerHello handshake message "
                    "failed";
    else if (sKseErrCode == KSETLS_ERR_TLS_BAD_HS_CERTIFICATE)
        strKseErr = (char *)"Processing of the Certificate handshake message "
                    "failed";
    else if (sKseErrCode == KSETLS_ERR_TLS_BAD_HS_CERTIFICATE_REQUEST)
        strKseErr = (char *)"Processing of the CertificateRequest handshake "
                    "message failed";
    else if (sKseErrCode == KSETLS_ERR_TLS_BAD_HS_SERVER_KEY_EXCHANGE)
        strKseErr = (char *)"Processing of the ServerKeyExchange handshake "
                    "message failed";
    else if (sKseErrCode == KSETLS_ERR_TLS_BAD_HS_SERVER_HELLO_DONE)
        strKseErr = (char *)"Processing of the ServerHelloDone handshake "
                    "message failed";
    else if (sKseErrCode == KSETLS_ERR_TLS_BAD_HS_CLIENT_KEY_EXCHANGE)
        strKseErr = (char *)"Processing of the ClientKeyExchange handshake "
                    "message failed";
    else if (sKseErrCode == KSETLS_ERR_TLS_BAD_HS_CLIENT_KEY_EXCHANGE_RP)
        strKseErr = (char *)"Processing of the ClientKeyExchange handshake "
                    "message failed in DHM / ECDH Read Public";
    else if (sKseErrCode == KSETLS_ERR_TLS_BAD_HS_CLIENT_KEY_EXCHANGE_CS)
        strKseErr = (char *)"Processing of the ClientKeyExchange handshake "
                    "message failed in DHM / ECDH Calculate Secret";
    else if (sKseErrCode == KSETLS_ERR_TLS_BAD_HS_CERTIFICATE_VERIFY)
        strKseErr = (char *)"Processing of the CertificateVerify handshake "
                    "message failed";
    else if (sKseErrCode == KSETLS_ERR_TLS_BAD_HS_CHANGE_CIPHER_SPEC)
        strKseErr = (char *)"Processing of the ChangeCipherSpec handshake "
                    "message failed";
    else if (sKseErrCode == KSETLS_ERR_TLS_BAD_HS_FINISHED)
        strKseErr = (char *)"Processing of the Finished handshake message "
                    "failed";
    else if (sKseErrCode == KSETLS_ERR_TLS_ALLOC_FAILED)
        strKseErr = (char *)"Memory allocation failed";
    else if (sKseErrCode == KSETLS_ERR_TLS_HW_ACCEL_FAILED)
        strKseErr = (char *)"Hardware acceleration function returned with "
                    "error";
    else if (sKseErrCode == KSETLS_ERR_TLS_HW_ACCEL_FALLTHROUGH)
        strKseErr = (char *)"Hardware acceleration function skipped / left "
                    "alone data";
    else if (sKseErrCode == KSETLS_ERR_TLS_COMPRESSION_FAILED)
        strKseErr = (char *)"Processing of the compression / decompression "
                    "failed";
    else if (sKseErrCode == KSETLS_ERR_TLS_BAD_HS_PROTOCOL_VERSION)
        strKseErr = (char *)"Handshake protocol not within min/max boundaries";
    else if (sKseErrCode == KSETLS_ERR_TLS_BAD_HS_NEW_SESSION_TICKET)
        strKseErr = (char *)"Processing of the NewSessionTicket handshake "
                    "message failed";
    else if (sKseErrCode == KSETLS_ERR_TLS_SESSION_TICKET_EXPIRED)
        strKseErr = (char *)"Session ticket has expired";
    else if (sKseErrCode == KSETLS_ERR_TLS_PK_TYPE_MISMATCH)
        strKseErr = (char *)"Public key type mismatch (eg, asked for RSA key "
                    "exchange and presented EC key)";
    else if (sKseErrCode == KSETLS_ERR_TLS_UNKNOWN_IDENTITY)
        strKseErr = (char *)"Unknown identity received (eg, PSK identity)";
    else if (sKseErrCode == KSETLS_ERR_TLS_INTERNAL_ERROR)
        strKseErr = (char *)"Internal error (eg, unexpected failure in "
                    "lower-level module)";
    else if (sKseErrCode == KSETLS_ERR_TLS_COUNTER_WRAPPING)
        strKseErr = (char *)"A counter would wrap (eg, too many messages "
                    "exchanged)";
    else if (sKseErrCode == KSETLS_ERR_TLS_WAITING_SERVER_HELLO_RENEGO)
        strKseErr = (char *)"Unexpected message at ServerHello in "
                    "renegotiation";
    else if (sKseErrCode == KSETLS_ERR_TLS_HELLO_VERIFY_REQUIRED)
        strKseErr = (char *)"DTLS client must retry for hello verification";
    else if (sKseErrCode == KSETLS_ERR_TLS_BUFFER_TOO_SMALL)
        strKseErr = (char *)"A buffer is too small to receive or write a "
                    "message";
    else if (sKseErrCode == KSETLS_ERR_TLS_NO_USABLE_CIPHERSUITE)
        strKseErr = (char *)"None of the common ciphersuites is usable (eg, no "
                    "suitable certificate, see debug messages)";
    else if (sKseErrCode == KSETLS_ERR_TLS_WANT_READ)
        strKseErr = (char *)"No data of requested type currently available on "
                    "underlying transport";
    else if (sKseErrCode == KSETLS_ERR_TLS_WANT_WRITE)
        strKseErr = (char *)"Connection requires a write call";
    else if (sKseErrCode == KSETLS_ERR_TLS_TIMEOUT)
        strKseErr = (char *)"The operation timed out";
    else if (sKseErrCode == KSETLS_ERR_TLS_CLIENT_RECONNECT)
        strKseErr = (char *)"The client initiated a reconnect from the same "
                    "port";
    else if (sKseErrCode == KSETLS_ERR_TLS_UNEXPECTED_RECORD)
        strKseErr = (char *)"Record header looks valid but is not expected";
    else if (sKseErrCode == KSETLS_ERR_TLS_NON_FATAL)
        strKseErr = (char *)"The alert message received indicates a non-fatal "
                    "error";
    else if (sKseErrCode == KSETLS_ERR_TLS_INVALID_VERIFY_HASH)
        strKseErr = (char *)"Couldn't set the hash for verifying "
                    "CertificateVerify";
    else if (sKseErrCode == KSETLS_ERR_TLS_CONTINUE_PROCESSING)
        strKseErr = (char *)"Internal-only message signaling that further "
                    "message-processing should be done";
    else if (sKseErrCode == KSETLS_ERR_TLS_ASYNC_IN_PROGRESS)
        strKseErr = (char *)"The asynchronous operation is not completed yet";
    else if (sKseErrCode == KSETLS_ERR_TLS_EARLY_MESSAGE)
        strKseErr = (char *)"Internal-only message signaling that a message "
                    "arrived early";
    else if (sKseErrCode == KSETLS_ERR_TLS_CRYPTO_IN_PROGRESS)
        strKseErr = (char *)"A cryptographic operation failure in progress.";

    else
    {
        printf("%s : [-0x%04X] KSE unknown error.\r\n", strErrFunc,
               -sKseErrCode);
        return;
    }

    if ((sKseErrCode < 0) && (iUsbErrCode < 0))
        printf("%s : [-0x%04X] %s([-0x%04X] %s).\r\n", strErrFunc, -sKseErrCode,
               strKseErr, -iUsbErrCode, libusb_strerror(iUsbErrCode));
    else if ((sKseErrCode < 0) && (iUsbErrCode >= 0))
        printf("%s : [-0x%04X] %s.\r\n", strErrFunc, -sKseErrCode, strKseErr);
    else if ((sKseErrCode >= 0) && (iUsbErrCode < 0))
        printf("%s : ([-0x%04X] %s).\r\n", strErrFunc, -iUsbErrCode,
               libusb_strerror(iUsbErrCode));
}
#endif // ENABLE_DEBUG_PRINT ---------------------------------------------------

//// Network Communication /////////////////////////////////////////////////////

//// Function Name : _tlsRecv
//// Parameters    : (Output) pbData
////                 (Input)  sDataBufferLen
////                 (Input)  socketDesc
//// Return Value  : Number of received bytes(>=0) / KSETLS_ERR_NET_...(<0)
//// Operation     : Receive data for TLS.
//// Description   : -  This function can be modified by the user.
int16_t _tlsRecv(uint8_t *pbData, int16_t sDataBufferLen, SOCKET socketDesc)
{
// User Code Begin -------------------------------------------------------------
    int16_t sInLen;

    sInLen = (int16_t)read(socketDesc, pbData, sDataBufferLen);
#ifdef ENABLE_DEBUG_PRINT
    if (sInLen >= 0)
        _debugPrintNetTxRxData(RECV, pbData, sInLen);
#endif

    return sInLen;
// User Code End ---------------------------------------------------------------
}

//// Function Name : _tlsSend
//// Parameters    : (Input) socketDesc
////                 (Input) pbData
////                 (Input) sDataBufferLen
//// Return Value  : Number of sent bytes(>=0) / KSETLS_ERR_NET_...(<0)
//// Operation     : Send data for TLS.
//// Description   : -  This function can be modified by the user.
int16_t _tlsSend(SOCKET socketDesc, uint8_t *pbData, int16_t sDataBufferLen)
{
// User Code Begin -------------------------------------------------------------
    int16_t sInLen;

    sInLen = (int16_t)write(socketDesc, pbData, sDataBufferLen);
#ifdef ENABLE_DEBUG_PRINT
    if (sInLen >= 0)
        _debugPrintNetTxRxData(SENT, pbData, sInLen);
#endif

    return sInLen;
// User Code End ---------------------------------------------------------------
}

//// USB Communication /////////////////////////////////////////////////////////

//// Function Name : _kseTransceive
//// Parameters    : (Output) pbRecvData
////                 (Output) pusRecvLen
////                 (Input)  pbSendData
////                 (Input)  usSendLen
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : Transceive data to KSE through USB.
//// Description   : - 'ghDev' would be referrenced.
int16_t _kseTransceive(uint8_t *pbRecvData, uint16_t *pusRecvLen,
                       uint8_t *pbSendData, uint16_t usSendLen)
{
    int iRv, iTransferred;
    uint8_t abInReport[64] = {0,};
    uint8_t abOutReport[64] = {0,};
    uint16_t usOffset, usLen, usRecvLen;

    if (ghDev == NULL)
    {
#ifdef ENABLE_DEBUG_PRINT
        _debugPrintErrStr("_kseTransceive[0]", KSE_FAIL_NOT_FOUND, 0);
#endif
        return KSE_FAIL_NOT_FOUND;
    }

    // Send Data.
    usOffset = 0;
    while (usSendLen > 0)
    {
        if ((abOutReport[0] == 0x00) && (usSendLen <= 60))
            abOutReport[0] = REP_ONE_BLOCK;
        else if ((abOutReport[0] == 0x00) && (usSendLen > 60))
            abOutReport[0] = REP_FIRST_BLOCK;
        else if ((abOutReport[0] != 0x00) && (usSendLen > 60))
            abOutReport[0] = REP_MIDDLE_BLOCK;
        else
            abOutReport[0] = REP_LAST_BLOCK;

        usLen = (usSendLen > 60) ? 60 : usSendLen;
        abOutReport[1] = 0x05;
        abOutReport[2] = (uint8_t)usOffset;
        abOutReport[3] = (uint8_t)usLen;
        _cpy(abOutReport + 4, pbSendData + usOffset * 60, usLen);
        usOffset++;
        usSendLen -= usLen;

        // Transceive Report.
#ifdef USB_INTERRUPT_TRANSFER
        iRv = libusb_interrupt_transfer(ghDev, OUT_ENDPOINT, abOutReport,
                                        REPORT_SIZE, &iTransferred, TIMEOUT_MS);
#else
        iRv = libusb_bulk_transfer(ghDev, OUT_ENDPOINT, abOutReport,
                                   REPORT_SIZE, &iTransferred, TIMEOUT_MS);
#endif
        if ((iRv < 0) || (iTransferred != REPORT_SIZE))
        {
#ifdef ENABLE_DEBUG_PRINT
            _debugPrintErrStr("_kseTransceive[1]", KSE_FAIL_USB_SEND_REPORT,
                              iRv);
#endif
            return KSE_FAIL_USB_SEND_REPORT;
        }
#ifdef USB_INTERRUPT_TRANSFER
        iRv = libusb_interrupt_transfer(ghDev, IN_ENDPOINT, abInReport,
                                        REPORT_SIZE, &iTransferred, TIMEOUT_MS);
#else
        iRv = libusb_bulk_transfer(ghDev, IN_ENDPOINT, abInReport,
                                   REPORT_SIZE, &iTransferred, TIMEOUT_MS);
#endif
        if ((iRv < 0) || (iTransferred != REPORT_SIZE))
        {
#ifdef ENABLE_DEBUG_PRINT
            _debugPrintErrStr("_kseTransceive[2]", KSE_FAIL_USB_RECV_REPORT,
                              iRv);
#endif
            return KSE_FAIL_USB_RECV_REPORT;
        }

        if ((((abOutReport[0] == REP_ONE_BLOCK) ||
              (abOutReport[0] == REP_LAST_BLOCK)) &&
             ((abInReport[1] != 0x06) ||
              (abInReport[2] != 0x00))) ||
            (((abOutReport[0] == REP_FIRST_BLOCK) ||
              (abOutReport[0] == REP_MIDDLE_BLOCK)) &&
             ((abInReport[0] != abOutReport[0]) ||
              (abInReport[1] != 0xFE) ||
              (abInReport[2] != abOutReport[2]) ||
              (abInReport[3] != abOutReport[3]))))
        {
#ifdef ENABLE_DEBUG_PRINT
            _debugPrintErrStr("_kseTransceive[3]", KSE_FAIL_UNEXPECTED_RESP, 0);
#endif
            return KSE_FAIL_UNEXPECTED_RESP;
        }
    }

    // Receive Data.
    usOffset = 0;
    usRecvLen = abInReport[3];
    if (usRecvLen > MAX_IO_BUFFER_SIZE)
    {
#ifdef ENABLE_DEBUG_PRINT
        _debugPrintErrStr("_kseTransceive[4]", KSE_FAIL_RECV_BUF_OVERFLOW, 0);
#endif
        return KSE_FAIL_RECV_BUF_OVERFLOW;
    }
    _cpy(pbRecvData, abInReport + 4, usRecvLen);

    if (abInReport[0] == REP_FIRST_BLOCK)
    {
        do
        {
            _cpy(abOutReport, abInReport, 4);
            abOutReport[1] = 0xFE;

            // Transceive Report.
#ifdef USB_INTERRUPT_TRANSFER
            iRv = libusb_interrupt_transfer(ghDev, OUT_ENDPOINT, abOutReport,
                                            REPORT_SIZE, &iTransferred,
                                            TIMEOUT_MS);
#else
            iRv = libusb_bulk_transfer(ghDev, OUT_ENDPOINT, abOutReport,
                                       REPORT_SIZE, &iTransferred, TIMEOUT_MS);
#endif
            if ((iRv < 0) || (iTransferred != REPORT_SIZE))
            {
#ifdef ENABLE_DEBUG_PRINT
                _debugPrintErrStr("_kseTransceive[5]", KSE_FAIL_USB_SEND_REPORT,
                                  iRv);
#endif
                return KSE_FAIL_USB_SEND_REPORT;
            }
#ifdef USB_INTERRUPT_TRANSFER
            iRv = libusb_interrupt_transfer(ghDev, IN_ENDPOINT, abInReport,
                                            REPORT_SIZE, &iTransferred,
                                            TIMEOUT_MS);
#else
            iRv = libusb_bulk_transfer(ghDev, IN_ENDPOINT, abInReport,
                                       REPORT_SIZE, &iTransferred, TIMEOUT_MS);
#endif
            if ((iRv < 0) || (iTransferred != REPORT_SIZE))
            {
#ifdef ENABLE_DEBUG_PRINT
                _debugPrintErrStr("_kseTransceive[6]", KSE_FAIL_USB_RECV_REPORT,
                                  iRv);
#endif
                return KSE_FAIL_USB_RECV_REPORT;
            }

            if (abInReport[1] != 0x06)
            {
#ifdef ENABLE_DEBUG_PRINT
                _debugPrintErrStr("_kseTransceive[7]", KSE_FAIL_UNEXPECTED_RESP,
                                  0);
#endif
                return KSE_FAIL_UNEXPECTED_RESP;
            }

            usOffset = abInReport[2];
            usLen = abInReport[3];
            usRecvLen += usLen;
            if (usRecvLen > MAX_IO_BUFFER_SIZE)
            {
#ifdef ENABLE_DEBUG_PRINT
                _debugPrintErrStr("_kseTransceive[8]",
                                  KSE_FAIL_RECV_BUF_OVERFLOW, 0);
#endif
                return KSE_FAIL_RECV_BUF_OVERFLOW;
            }
            _cpy(pbRecvData + usOffset * 60, abInReport + 4, usLen);
        }
        while (abInReport[0] == REP_MIDDLE_BLOCK);
    }
    *pusRecvLen = usRecvLen;

    return KSE_SUCCESS;
}

//// Function Name : _usbClose
//// Parameters    : void
//// Return Value  : void
//// Operation     : Close USB.
//// Description   : - 'ghDev' and 'gusKsePower' would be cleared.
void _usbClose(void)
{
    libusb_release_interface(ghDev, 0);
    libusb_close(ghDev);
    libusb_exit(NULL);
    ghDev = NULL;
    gusKsePower = KSE_POWER_OFF;
}

//// KSE DIM ///////////////////////////////////////////////////////////////////

//// Function Name : _ksePowerOn
//// Parameters    : (Output) pbVer
////                 (Output) pbLifeCycle
////                 (Output) pbSystemTitle
////                 (Output) pbPinType
////                 (Output) pbMaxPinRetryCount
////                 (Output) pusMaxKcmvpKeyCount
////                 (Output) pusMaxCertKeyCount
////                 (Output) pusMaxIoDataSize
////                 (Output) pusInfoFileSize
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : Power on KSE.
//// Description   : - 'pbVer' should be NULL_PTR or 3-byte array.
////                 - 'pbLifeCycle' should be NULL_PTR or 1-byte pointer.
////                 - 'pbSystemTitle' should be NULL_PTR or 8-byte array.
////                 - 'pbPinType' should be NULL_PTR or 1-byte pointer.
////                 - 'pbMaxPinRetryCount' should be NULL_PTR or 1-byte
////                    pointer.
////                 - 'pusMaxKcmvpKeyCount' should be NULL_PTR or 1-ushort
////                    pointer.
////                 - 'pusMaxCertKeyCount' should be NULL_PTR or 1-ushort
////                    pointer.
////                 - 'pusMaxIoDataSize' should be NULL_PTR or 1-ushort
////                    pointer.
////                 - 'pusInfoFileSize' should be NULL_PTR or 1-ushort pointer.
////                 -  LifeCycle will be one of belows.
////                    < LC_MANUFACTURED / LC_ISSUED / LC_TERMINATED >
////                 -  PinType will be one of belows.
////                    < PIN_ENABLE / PIN_DISABLE >
////                 - '*pusMaxKcmvpKeyCount' would be 8.
////                   '*pusMaxCertKeyCount' would be 96.
////                   '*pusMaxIoDataSize' would be 2944.
////                   '*pusInfoFileSize' would be 8192.
int16_t _ksePowerOn(uint8_t *pbVer, uint8_t *pbLifeCycle,
                    uint8_t *pbSystemTitle, uint8_t *pbPinType,
                    uint8_t *pbMaxPinRetryCount, uint16_t *pusMaxKcmvpKeyCount,
                    uint16_t *pusMaxCertKeyCount, uint16_t *pusMaxIoDataSize,
                    uint16_t *pusInfoFileSize)
{
    int iRv;
    uint16_t usLen;
    int16_t i, sRv, sCount;
    libusb_device **ppstDevs;
    libusb_device_handle *hDev = NULL;
    struct libusb_device_descriptor stDesc;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_OFF)
    {
#ifdef ENABLE_DEBUG_PRINT
        _debugPrintErrStr("_ksePowerOn[0]", KSE_FAIL_ALREADY_POWERED_ON, 0);
#endif
        return KSE_FAIL_ALREADY_POWERED_ON;
    }

    // Find USB DIM.
    iRv = libusb_init(NULL);
    if (iRv < 0)
    {
#ifdef ENABLE_DEBUG_PRINT
        _debugPrintErrStr("_ksePowerOn[1]", KSE_FAIL_USB_INIT, iRv);
#endif
        return KSE_FAIL_USB_INIT;
    }

    sCount = (int16_t)libusb_get_device_list(NULL, &ppstDevs);
    if (sCount < 0)
    {
#ifdef ENABLE_DEBUG_PRINT
        _debugPrintErrStr("_ksePowerOn[2]", KSE_FAIL_USB_NO_DEVICES, 0);
#endif
        libusb_exit(NULL);
        return KSE_FAIL_USB_NO_DEVICES;
    }

    for (i = 0; i < sCount; i++)
    {
        iRv = libusb_get_device_descriptor(ppstDevs[i], &stDesc);
        if (iRv < 0)
            continue;
        if ((stDesc.idVendor != VENDOR_ID) || (stDesc.idProduct != PRODUCT_ID))
            continue;
        iRv = libusb_open(ppstDevs[i], &hDev);
        if (iRv == LIBUSB_SUCCESS)
            break;
#ifdef ENABLE_DEBUG_PRINT
        _debugPrintErrStr("_ksePowerOn[3]", KSE_FAIL_USB_DEVICE_OPEN, iRv);
#endif
        libusb_exit(NULL);
        return KSE_FAIL_USB_DEVICE_OPEN;
    }
    if (i == sCount)
    {
#ifdef ENABLE_DEBUG_PRINT
        _debugPrintErrStr("_ksePowerOn[4]", KSE_FAIL_NOT_FOUND, 0);
#endif
        libusb_exit(NULL);
        return KSE_FAIL_NOT_FOUND;
    }
    libusb_free_device_list(ppstDevs, 1);
    ghDev = hDev;

    // Find out if kernel driver is attached.
	if (libusb_kernel_driver_active(ghDev, INTERFACE_NUMBER) == 1)
    {
        // Detach it.
		iRv = libusb_detach_kernel_driver(ghDev, INTERFACE_NUMBER);
        if (iRv < 0)
        {
#ifdef ENABLE_DEBUG_PRINT
            _debugPrintErrStr("_ksePowerOn[5]",
                              KSE_FAIL_USB_DETACH_KERNEL_DRIVER, iRv);
#endif
            _usbClose();
            return KSE_FAIL_USB_DETACH_KERNEL_DRIVER;
        }
    }

    // Claim interface 0 of device.
    iRv = libusb_claim_interface(ghDev, INTERFACE_NUMBER);
    if (iRv < 0)
    {
#ifdef ENABLE_DEBUG_PRINT
        _debugPrintErrStr("_ksePowerOn[6]", KSE_FAIL_USB_CLAIM_INTERFACE, iRv);
#endif
        _usbClose();
        return KSE_FAIL_USB_CLAIM_INTERFACE;
    }

    // KSE Power On.
    gabTxRxData[0] = 0x00;
    gabTxRxData[1] = 0x00;
    sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, 2);
    if (sRv != KSE_SUCCESS)
    {
#ifdef ENABLE_DEBUG_PRINT
        _debugPrintErrStr("_ksePowerOn[7]", sRv, 0);
#endif
        _usbClose();
        return sRv;
    }
    sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                    ((uint16_t)gabTxRxData[1]));
    if (((sRv == KSE_SUCCESS) && (usLen != 31)) ||
        ((sRv != KSE_SUCCESS) && (usLen != 2)))
    {
#ifdef ENABLE_DEBUG_PRINT
        _debugPrintErrStr("_ksePowerOn[8]", KSE_FAIL_UNEXPECTED_RESP_LEN, 0);
#endif
        _usbClose();
        return KSE_FAIL_UNEXPECTED_RESP_LEN;
    }
    if (sRv != KSE_SUCCESS)
    {
#ifdef ENABLE_DEBUG_PRINT
        _debugPrintErrStr("_ksePowerOn[9]", sRv, 0);
#endif
        _usbClose();
        return sRv;
    }
    if (pbVer != NULL_PTR)
        _cpy(pbVer, gabTxRxData + 9, 3);
    if (pbLifeCycle != NULL_PTR)
        *pbLifeCycle = gabTxRxData[12];
    if (pbSystemTitle != NULL_PTR)
        _cpy(pbSystemTitle, gabTxRxData + 13, 8);
    if (pbPinType != NULL_PTR)
        *pbPinType = gabTxRxData[21];
    if (pbMaxPinRetryCount != NULL_PTR)
        *pbMaxPinRetryCount = gabTxRxData[22];
    if (pusMaxKcmvpKeyCount != NULL_PTR)
        *pusMaxKcmvpKeyCount = ((uint16_t)gabTxRxData[23] << 8) |
                               ((uint16_t)gabTxRxData[24]);;
    if (pusMaxCertKeyCount != NULL_PTR)
        *pusMaxCertKeyCount = ((uint16_t)gabTxRxData[25] << 8) |
                              ((uint16_t)gabTxRxData[26]);
    if (pusMaxIoDataSize != NULL_PTR)
        *pusMaxIoDataSize = ((uint16_t)gabTxRxData[27] << 8) |
                            ((uint16_t)gabTxRxData[28]);
    if (pusInfoFileSize != NULL_PTR)
        *pusInfoFileSize = ((uint16_t)gabTxRxData[29] << 8) |
                           ((uint16_t)gabTxRxData[30]);
    gusKsePower = KSE_POWER_ON;

    return sRv;
}

//// Function Name : _ksePowerOff
//// Parameters    : void
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : Power off KSE.
//// Description   : -  None.
int16_t _ksePowerOff(void)
{
    int16_t sRv;
    uint16_t usLen;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
    {
#ifdef ENABLE_DEBUG_PRINT
        _debugPrintErrStr("_ksePowerOff[0]", KSE_FAIL_NOT_POWERED_ON, 0);
#endif
        return KSE_FAIL_NOT_POWERED_ON;
    }

    // KSE Power Off.
    gabTxRxData[0] = 0x00;
    gabTxRxData[1] = 0x01;
    sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, 2);
    if (sRv != KSE_SUCCESS)
    {
#ifdef ENABLE_DEBUG_PRINT
        _debugPrintErrStr("_ksePowerOff[1]", sRv, 0);
#endif
        _usbClose();
        return sRv;
    }
    if (usLen != 2)
    {
#ifdef ENABLE_DEBUG_PRINT
        _debugPrintErrStr("_ksePowerOff[2]", KSE_FAIL_UNEXPECTED_RESP_LEN, 0);
#endif
        _usbClose();
        return KSE_FAIL_UNEXPECTED_RESP_LEN;
    }
    sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                    ((uint16_t)gabTxRxData[1]));
#ifdef ENABLE_DEBUG_PRINT
    if (sRv != KSE_SUCCESS)
        _debugPrintErrStr("_ksePowerOff[3]", sRv, 0);
#endif
    _usbClose();

    return sRv;
}

//// KSE ///////////////////////////////////////////////////////////////////////

//// Function Name : _kseReadInfoFile
//// Parameters    : (Output) pbData
////                 (Input)  usOffset
////                 (Input)  usSize
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : Read data from the KSE Info File.
//// Description   : - 'usSize' should be max 2944.
////                 -  Info File size is 8192 bytes.
int16_t _kseReadInfoFile(uint8_t *pbData, uint16_t usOffset, uint16_t usSize)
{
    int16_t sRv;
    uint16_t usLen;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if ((pbData == NULL_PTR) || (usSize > MAX_IO_DATA_SIZE) ||
        (usOffset + usSize > MAX_INFO_FILE_SIZE))
        return KSE_FAIL_WRONG_INPUT;

    // Read data from the KSE Info File.
    gabTxRxData[0] = 0x00;
    gabTxRxData[1] = 0x04;
    gabTxRxData[2] = (uint8_t)(usOffset >> 8);
    gabTxRxData[3] = (uint8_t)(usOffset);
    gabTxRxData[4] = (uint8_t)(usSize >> 8);
    gabTxRxData[5] = (uint8_t)(usSize);
    sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, 6);
    if (sRv != KSE_SUCCESS)
        return sRv;
    sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                    ((uint16_t)gabTxRxData[1]));
    if (((sRv == KSE_SUCCESS) && (usLen != usSize + 2)) ||
        ((sRv != KSE_SUCCESS) && (usLen != 2)))
        return KSE_FAIL_UNEXPECTED_RESP_LEN;
    if (sRv != KSE_SUCCESS)
        return sRv;
    _cpy(pbData, gabTxRxData + 2, usSize);

    return sRv;
}

//// Function Name : _kseWriteInfoFile
//// Parameters    : (Input) pbData
////                 (Input) usSize
////                 (Input) usOffset
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : Write data to the KSE Info File.
//// Description   : - 'usSize' should be max 2944.
////                 -  Info File size is 8192 bytes.
int16_t _kseWriteInfoFile(uint8_t *pbData, uint16_t usSize, uint16_t usOffset)
{
    int16_t sRv;
    uint16_t usLen;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if ((pbData == NULL_PTR) || (usSize > MAX_IO_DATA_SIZE) ||
        (usOffset + usSize > MAX_INFO_FILE_SIZE))
        return KSE_FAIL_WRONG_INPUT;

    // Write data to the KSE Info File.
    gabTxRxData[0] = 0x00;
    gabTxRxData[1] = 0x05;
    gabTxRxData[2] = (uint8_t)(usOffset >> 8);
    gabTxRxData[3] = (uint8_t)(usOffset);
    gabTxRxData[4] = (uint8_t)(usSize >> 8);
    gabTxRxData[5] = (uint8_t)(usSize);
    _cpy(gabTxRxData + 6, pbData, usSize);
    sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, usSize + 6);
    if (sRv != KSE_SUCCESS)
        return sRv;
    if (usLen != 2)
        return KSE_FAIL_UNEXPECTED_RESP_LEN;
    sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                    ((uint16_t)gabTxRxData[1]));

    return sRv;
}

//// KCMVP /////////////////////////////////////////////////////////////////////

//// Function Name : _kcmvpGenerateKey
//// Parameters    : (Input) bKeyType
////                 (Input) usKeyIndex
////                 (Input) usHmacKeySize
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : < AES / ARIA / HMAC-SHA2-256 / ECDSA-P256 / ECDH-P256 >
////                 Generate a key / keypair / keytoken in the KSE nonvolatile
////                 memory.
//// Description   : - 'bKeyType' should be one of belows.
////                    < KCMVP_AES128_KEY('40') / KCMVP_AES192_KEY('41') /
////                      KCMVP_AES256_KEY('42') / KCMVP_ARIA128_KEY('50') /
////                      KCMVP_ARIA192_KEY('51') / KCMVP_ARIA256_KEY('52') /
////                      KCMVP_HMAC_KEY('70') / KCMVP_ECDSA_KEYPAIR('80') /
////                      KCMVP_ECDH_KEYPAIR('90') >
////                 - 'usKeyIndex' should be 0 ~ 7.
////                 - 'usHmacKeySize' is used only if 'bKeyType' is
////                    KCMVP_HMAC_KEY. It should be 0 ~ 255.
int16_t _kcmvpGenerateKey(uint8_t bKeyType, uint16_t usKeyIndex,
                          uint16_t usHmacKeySize)
{
    int16_t sRv;
    uint16_t usLen, usCmdSize;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if (((bKeyType != KCMVP_AES128_KEY) && (bKeyType != KCMVP_AES192_KEY) &&
         (bKeyType != KCMVP_AES256_KEY) && (bKeyType != KCMVP_ARIA128_KEY) &&
         (bKeyType != KCMVP_ARIA192_KEY) && (bKeyType != KCMVP_ARIA256_KEY) &&
         (bKeyType != KCMVP_HMAC_KEY) && (bKeyType != KCMVP_ECDSA_KEYPAIR) &&
         (bKeyType != KCMVP_ECDH_KEYPAIR)) ||
        (usKeyIndex >= MAX_KCMVP_KEY_COUNT) ||
        ((bKeyType == KCMVP_HMAC_KEY) && (usHmacKeySize > 255)))
        return KSE_FAIL_WRONG_INPUT;

    // Generate a key / keypair / keytoken.
    gabTxRxData[0] = 0x02;
    gabTxRxData[1] = 0x00;
    gabTxRxData[2] = bKeyType;
    gabTxRxData[3] = (uint8_t)(usKeyIndex >> 8);
    gabTxRxData[4] = (uint8_t)(usKeyIndex);
    if (bKeyType != KCMVP_HMAC_KEY)
        usCmdSize = 5;
    else
    {
        gabTxRxData[5] = (uint8_t)(usKeyIndex >> 8);
        gabTxRxData[6] = (uint8_t)(usKeyIndex);
        usCmdSize = 7;
    }
    sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, usCmdSize);
    if (sRv != KSE_SUCCESS)
        return sRv;
    if (usLen != 2)
        return KSE_FAIL_UNEXPECTED_RESP_LEN;
    sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                    ((uint16_t)gabTxRxData[1]));

    return sRv;
}

//// Function Name : _kcmvpPutKey
//// Parameters    : (Input) bKeyType
////                 (Input) usKeyIndex
////                 (Input) pbKey
////                 (Input) usKeySize
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : < AES / ARIA / HMAC-SHA2-256 / ECDSA-P256 / ECDH-P256 > Put
////                 the key in the KSE nonvolatile memory.
//// Description   : - 'bKeyType' should be one of belows.
////                    < KCMVP_AES128_KEY('40') / KCMVP_AES192_KEY('41') /
////                      KCMVP_AES256_KEY('42') / KCMVP_ARIA128_KEY('50') /
////                      KCMVP_ARIA192_KEY('51') / KCMVP_ARIA256_KEY('52') /
////                      KCMVP_HMAC_KEY('70') / KCMVP_ECDSA_PRI_KEY('81') /
////                      KCMVP_ECDSA_PUB_KEY('82') / KCMVP_ECDH_PRI_KEY('91') /
////                      KCMVP_ECDH_PUB_KEY('92') >
////                 - 'usKeyIndex' should be 0 ~ 7.
////                 - 'pbKey' should be 'usKeySize'-byte array.
////                 - 'usKeySize' should be one of belows according to
////                   'bKeyType'.
////                    < 16 (KCMVP_AES128_KEY / KCMVP_ARIA128_KEY) /
////                      24 (KCMVP_AES192_KEY / KCMVP_ARIA192_KEY) /
////                      32 (KCMVP_AES256_KEY / KCMVP_ARIA256_KEY /
////                          KCMVP_ECDSA_PRI_KEY / KCMVP_ECDH_PRI_KEY) /
////                      64 (KCMVP_ECDSA_PUB_KEY / KCMVP_ECDH_PUB_KEY) /
////                      0 ~ 255 (KCMVP_HMAC_KEY) >
int16_t _kcmvpPutKey(uint8_t bKeyType, uint16_t usKeyIndex, uint8_t *pbKey,
                     uint16_t usKeySize)
{
    int16_t sRv;
    uint16_t usLen, usCmdSize;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if (((bKeyType != KCMVP_AES128_KEY) && (bKeyType != KCMVP_AES192_KEY) &&
         (bKeyType != KCMVP_AES256_KEY) && (bKeyType != KCMVP_ARIA128_KEY) &&
         (bKeyType != KCMVP_ARIA192_KEY) && (bKeyType != KCMVP_ARIA256_KEY) &&
         (bKeyType != KCMVP_HMAC_KEY) && (bKeyType != KCMVP_ECDSA_PRI_KEY) &&
         (bKeyType != KCMVP_ECDSA_PUB_KEY) &&
         (bKeyType != KCMVP_ECDH_PRI_KEY) &&
         (bKeyType != KCMVP_ECDH_PUB_KEY)) ||
        (usKeyIndex >= MAX_KCMVP_KEY_COUNT) ||
        (pbKey == NULL_PTR) ||
        (((bKeyType == KCMVP_AES128_KEY) || (bKeyType == KCMVP_ARIA128_KEY)) &&
         (usKeySize != 16)) ||
        (((bKeyType == KCMVP_AES192_KEY) || (bKeyType == KCMVP_ARIA192_KEY)) &&
         (usKeySize != 24)) ||
        (((bKeyType == KCMVP_AES256_KEY) || (bKeyType == KCMVP_ARIA256_KEY) ||
          (bKeyType == KCMVP_ECDSA_PRI_KEY) ||
          (bKeyType == KCMVP_ECDH_PRI_KEY)) &&
         (usKeySize != 32)) ||
        (((bKeyType == KCMVP_ECDSA_PUB_KEY) ||
          (bKeyType == KCMVP_ECDH_PUB_KEY)) &&
         (usKeySize != 64)) ||
        ((bKeyType == KCMVP_HMAC_KEY) && (usKeySize > 255)))
        return KSE_FAIL_WRONG_INPUT;

    // Put the key.
    gabTxRxData[0] = 0x02;
    gabTxRxData[1] = 0x01;
    gabTxRxData[2] = bKeyType;
    gabTxRxData[3] = (uint8_t)(usKeyIndex >> 8);
    gabTxRxData[4] = (uint8_t)(usKeyIndex);
    if (bKeyType != KCMVP_HMAC_KEY)
        usCmdSize = 5;
    else
    {
        gabTxRxData[5] = (uint8_t)(usKeySize >> 8);
        gabTxRxData[6] = (uint8_t)(usKeySize);
        usCmdSize = 7;
    }
    _cpy(gabTxRxData + usCmdSize, pbKey, usKeySize);
    sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData,
                         usCmdSize + usKeySize);
    if (sRv != KSE_SUCCESS)
        return sRv;
    if (usLen != 2)
        return KSE_FAIL_UNEXPECTED_RESP_LEN;
    sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                    ((uint16_t)gabTxRxData[1]));

    return sRv;
}

//// Function Name : _kcmvpGetKey
//// Parameters    : (Output) pbKey
////                 (In/Out) pusKeySize
////                 (Input)  bKeyType
////                 (Input)  usKeyIndex
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : < AES / ARIA / HMAC-SHA2-256 / ECDSA-P256 / ECDH-P256 > Get
////                 the key in the KSE nonvolatile memory.
//// Description   : -  If 'pbKey' is NULL_PTR, only '*pusKeySize' is output.
////                 -  Input '*pusKeySize' is 'pbKey' array size.
////                 -  Output '*pusKeySize' is key data size.
////                 - 'bKeyType' should be one of belows.
////                    < KCMVP_AES128_KEY('40') / KCMVP_AES192_KEY('41') /
////                      KCMVP_AES256_KEY('42') / KCMVP_ARIA128_KEY('50') /
////                      KCMVP_ARIA192_KEY('51') / KCMVP_ARIA256_KEY('52') /
////                      KCMVP_HMAC_KEY('70') / KCMVP_ECDSA_PRI_KEY('81') /
////                      KCMVP_ECDSA_PUB_KEY('82') / KCMVP_ECDH_PRI_KEY('91') /
////                      KCMVP_ECDH_PUB_KEY('92') >
////                 - 'usKeyIndex' should be 0 ~ 7.
int16_t _kcmvpGetKey(uint8_t *pbKey, uint16_t *pusKeySize, uint8_t bKeyType,
                     uint16_t usKeyIndex)
{
    int16_t sRv;
    uint16_t usLen, usKeySize;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if ((pusKeySize == NULL_PTR) ||
        ((bKeyType != KCMVP_AES128_KEY) && (bKeyType != KCMVP_AES192_KEY) &&
         (bKeyType != KCMVP_AES256_KEY) && (bKeyType != KCMVP_ARIA128_KEY) &&
         (bKeyType != KCMVP_ARIA192_KEY) && (bKeyType != KCMVP_ARIA256_KEY) &&
         (bKeyType != KCMVP_HMAC_KEY) && (bKeyType != KCMVP_ECDSA_PRI_KEY) &&
         (bKeyType != KCMVP_ECDSA_PUB_KEY) &&
         (bKeyType != KCMVP_ECDH_PRI_KEY) &&
         (bKeyType != KCMVP_ECDH_PUB_KEY)) ||
        (usKeyIndex >= MAX_KCMVP_KEY_COUNT))
        return KSE_FAIL_WRONG_INPUT;

    // Get the key.
    gabTxRxData[0] = 0x02;
    gabTxRxData[1] = 0x02;
    gabTxRxData[2] = bKeyType;
    gabTxRxData[3] = (uint8_t)(usKeyIndex >> 8);
    gabTxRxData[4] = (uint8_t)(usKeyIndex);
    sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, 5);
    if (sRv != KSE_SUCCESS)
        return sRv;
    sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                    ((uint16_t)gabTxRxData[1]));
    if (((sRv == KSE_SUCCESS) &&
         ((((bKeyType == KCMVP_AES128_KEY) ||
            (bKeyType == KCMVP_ARIA128_KEY)) &&
           (usLen != 18)) ||
          (((bKeyType == KCMVP_AES192_KEY) ||
            (bKeyType == KCMVP_ARIA192_KEY)) &&
           (usLen != 26)) ||
          (((bKeyType == KCMVP_AES256_KEY) || (bKeyType == KCMVP_ARIA256_KEY) ||
            (bKeyType == KCMVP_ECDSA_PRI_KEY) ||
            (bKeyType == KCMVP_ECDH_PRI_KEY)) &&
           (usLen != 34)) ||
          (((bKeyType == KCMVP_ECDSA_PUB_KEY) ||
            (bKeyType == KCMVP_ECDH_PUB_KEY)) &&
           (usLen != 66)) ||
          ((bKeyType != KCMVP_HMAC_KEY) && ((usLen < 2) || (usLen > 257))))) ||
        ((sRv != KSE_SUCCESS) && (usLen != 2)))
        return KSE_FAIL_UNEXPECTED_RESP_LEN;
    if (sRv != KSE_SUCCESS)
        return sRv;
    usKeySize = usLen - 2;
    usLen = (usKeySize < *pusKeySize) ? usKeySize : *pusKeySize;
    if (pbKey != NULL_PTR)
        _cpy(pbKey, gabTxRxData + 2, usLen);
    *pusKeySize = usKeySize;

    return sRv;
}

//// Function Name : _kcmvpEraseKey
//// Parameters    : (Input) bKeyType
////                 (Input) usKeyIndex
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : < AES / ARIA / HMAC-SHA2-256 / ECDSA-P256 / ECDH-P256 >
////                 Erase the key in the KSE nonvolatile memory.
//// Description   : - 'bKeyType' should be one of belows.
////                    < KCMVP_AES128_KEY('40') / KCMVP_AES192_KEY('41') /
////                      KCMVP_AES256_KEY('42') / KCMVP_ARIA128_KEY('50') /
////                      KCMVP_ARIA192_KEY('51') / KCMVP_ARIA256_KEY('52') /
////                      KCMVP_HMAC_KEY('70') / KCMVP_ECDSA_PRI_KEY('81') /
////                      KCMVP_ECDSA_PUB_KEY('82') / KCMVP_ECDH_PRI_KEY('91') /
////                      KCMVP_ECDH_PUB_KEY('92') >
////                 - 'usKeyIndex' should be 0 ~ 7.
int16_t _kcmvpEraseKey(uint8_t bKeyType, uint16_t usKeyIndex)
{
    int16_t sRv;
    uint16_t usLen;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if (((bKeyType != KCMVP_AES128_KEY) && (bKeyType != KCMVP_AES192_KEY) &&
         (bKeyType != KCMVP_AES256_KEY) && (bKeyType != KCMVP_ARIA128_KEY) &&
         (bKeyType != KCMVP_ARIA192_KEY) && (bKeyType != KCMVP_ARIA256_KEY) &&
         (bKeyType != KCMVP_HMAC_KEY) && (bKeyType != KCMVP_ECDSA_PRI_KEY) &&
         (bKeyType != KCMVP_ECDSA_PUB_KEY) &&
         (bKeyType != KCMVP_ECDH_PRI_KEY) &&
         (bKeyType != KCMVP_ECDH_PUB_KEY)) ||
        (usKeyIndex >= MAX_KCMVP_KEY_COUNT))
        return KSE_FAIL_WRONG_INPUT;

    // Erase the key.
    gabTxRxData[0] = 0x02;
    gabTxRxData[1] = 0x03;
    gabTxRxData[2] = bKeyType;
    gabTxRxData[3] = (uint8_t)(usKeyIndex >> 8);
    gabTxRxData[4] = (uint8_t)(usKeyIndex);
    sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, 5);
    if (sRv != KSE_SUCCESS)
        return sRv;
    if (usLen != 2)
        return KSE_FAIL_UNEXPECTED_RESP_LEN;
    sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                    ((uint16_t)gabTxRxData[1]));

    return sRv;
}

//// Function Name : _kcmvpDrbg
//// Parameters    : (Output) pbRandom
////                 (Input)  usRandomSize
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : < DRBG > Generate random number.
//// Description   : - 'usRandomSize' should be 1 ~ 256.
int16_t _kcmvpDrbg(uint8_t *pbRandom, uint16_t usRandomSize)
{
    int16_t sRv;
    uint16_t usLen;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if ((pbRandom == NULL_PTR) || (usRandomSize == 0) || (usRandomSize > 256))
        return KSE_FAIL_WRONG_INPUT;

    // Generate random number.
    gabTxRxData[0] = 0x02;
    gabTxRxData[1] = 0x10;
    gabTxRxData[2] = (uint8_t)(usRandomSize >> 8);
    gabTxRxData[3] = (uint8_t)(usRandomSize);
    sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, 4);
    if (sRv != KSE_SUCCESS)
        return sRv;
    sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                    ((uint16_t)gabTxRxData[1]));
    if (((sRv == KSE_SUCCESS) && (usLen != usRandomSize + 2)) ||
        ((sRv != KSE_SUCCESS) && (usLen != 2)))
        return KSE_FAIL_UNEXPECTED_RESP_LEN;
    if (sRv != KSE_SUCCESS)
        return sRv;
    _cpy(pbRandom, gabTxRxData + 2, usRandomSize);

    return sRv;
}

//// Function Name : _kcmvpEcb
//// Parameters    : (Output) pbOutput
////                 (Input)  pbInput
////                 (Input)  usInputSize
////                 (Input)  bKeyType
////                 (Input)  usKeyIndex
////                 (Input)  bEnDe
////                 (Input)  bAlg
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : < AES / ARIA > ECB Mode Encryption / Decryption.
//// Description   : -  This function is internal function for AES / ARIA.
////                 -  Output size is 'usInputSize'.
////                 - 'usInputSize' should be non-zero, multiples of block
////                    size.
////                 - 'bKeyType' should be one of belows.
////                    If 'bAlg' is KCMVP_AES,
////                    < KCMVP_AES128_KEY('40') / KCMVP_AES192_KEY('41') /
////                      KCMVP_AES256_KEY('42') >
////                    If 'bAlg' is KCMVP_ARIA,
////                    < KCMVP_ARIA128_KEY('50') / KCMVP_ARIA192_KEY('51') /
////                      KCMVP_ARIA256_KEY('52') >
////                 - 'usKeyIndex' should be 0 ~ 7.
////                 - 'bEnDe' should be one of belows.
////                    < ENCRYPT(0) / DECRYPT(1) >
////                 - 'bAlg' should be one of belows.
////                    < KCMVP_AES('40') / KCMVP_ARIA('50') >
int16_t _kcmvpEcb(uint8_t *pbOutput, uint8_t *pbInput, uint16_t usInputSize,
                  uint8_t bKeyType, uint16_t usKeyIndex, uint8_t bEnDe,
                  uint8_t bAlg)
{
    int16_t sRv;
    uint16_t usLen, usSize;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if ((pbOutput == NULL_PTR) || (pbInput == NULL_PTR) || (usInputSize == 0) ||
        (usKeyIndex >= MAX_KCMVP_KEY_COUNT) ||
        ((bEnDe != ENCRYPT) && (bEnDe != DECRYPT)) ||
        (((bAlg & 0xF0) ^ bKeyType) > 0x02) ||
        ((usInputSize % 16) != 0))
        return KSE_FAIL_WRONG_INPUT;

    // ECB.
    do
    {
        gabTxRxData[0] = 0x02;
        gabTxRxData[1] = (uint8_t)(bAlg | 0x04);
        gabTxRxData[2] = bKeyType;
        gabTxRxData[3] = (uint8_t)(usKeyIndex >> 8);
        gabTxRxData[4] = (uint8_t)(usKeyIndex);
        gabTxRxData[5] = bEnDe;
        usSize = (usInputSize < MAX_IO_DATA_SIZE) ?
                  usInputSize : MAX_IO_DATA_SIZE;
        gabTxRxData[6] = (uint8_t)(usSize >> 8);
        gabTxRxData[7] = (uint8_t)(usSize);
        _cpy(gabTxRxData + 8, pbInput, usSize);
        sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, usSize + 8);
        if (sRv != KSE_SUCCESS)
            return sRv;
        sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                        ((uint16_t)gabTxRxData[1]));
        if (((sRv == KSE_SUCCESS) && (usLen != usSize + 2)) ||
            ((sRv != KSE_SUCCESS) && (usLen != 2)))
            return KSE_FAIL_UNEXPECTED_RESP_LEN;
        if (sRv != KSE_SUCCESS)
            return sRv;
        _cpy(pbOutput, gabTxRxData + 2, usSize);
        pbInput += usSize;
        pbOutput += usSize;
        usInputSize -= usSize;
    }
    while (usInputSize > 0);

    return sRv;
}

//// Function Name : _kcmvpCbc
//// Parameters    : (Output) pbOutput
////                 (Input)  pbInput
////                 (Input)  usInputSize
////                 (Input)  bKeyType
////                 (Input)  usKeyIndex
////                 (Input)  pbIv
////                 (Input)  bEnDe
////                 (Input)  bAlg
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : < AES / ARIA > CBC Mode Encryption / Decryption.
//// Description   : -  This function is internal function for AES / ARIA.
////                 -  Output size is 'usInputSize'.
////                 - 'usInputSize' should be non-zero, multiples of block
////                    size.
////                 - 'bKeyType' should be one of belows.
////                    If 'bAlg' is KCMVP_AES,
////                    < KCMVP_AES128_KEY('40') / KCMVP_AES192_KEY('41') /
////                      KCMVP_AES256_KEY('42') >
////                    If 'bAlg' is KCMVP_ARIA,
////                    < KCMVP_ARIA128_KEY('50') / KCMVP_ARIA192_KEY('51') /
////                      KCMVP_ARIA256_KEY('52') >
////                 - 'usKeyIndex' should be 0 ~ 7.
////                 - 'pbIv' should be block size byte array.
////                 - 'bEnDe' should be one of belows.
////                    < ENCRYPT(0) / DECRYPT(1) >
////                 - 'bAlg' should be one of belows.
////                    < KCMVP_AES('40') / KCMVP_ARIA('50') >
int16_t _kcmvpCbc(uint8_t *pbOutput, uint8_t *pbInput, uint16_t usInputSize,
                  uint8_t bKeyType, uint16_t usKeyIndex, uint8_t *pbIv,
                  uint8_t bEnDe, uint8_t bAlg)
{
    int16_t sRv;
    uint8_t abBuffer[16];
    uint16_t usLen, usSize;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if ((pbOutput == NULL_PTR) || (pbInput == NULL_PTR) || (pbIv == NULL_PTR) ||
        (usInputSize == 0) || (usKeyIndex >= MAX_KCMVP_KEY_COUNT) ||
        ((bEnDe != ENCRYPT) && (bEnDe != DECRYPT)) ||
        (((bAlg & 0xF0) ^ bKeyType) > 0x02) ||
        ((usInputSize % 16) != 0))
        return KSE_FAIL_WRONG_INPUT;

    // CBC.
    do
    {
        gabTxRxData[0] = 0x02;
        gabTxRxData[1] = (uint8_t)(bAlg | 0x05);
        gabTxRxData[2] = bKeyType;
        gabTxRxData[3] = (uint8_t)(usKeyIndex >> 8);
        gabTxRxData[4] = (uint8_t)(usKeyIndex);
        gabTxRxData[5] = bEnDe;
        usSize = (usInputSize + 16 < MAX_IO_DATA_SIZE) ?
                  usInputSize : MAX_IO_DATA_SIZE - 16;
        gabTxRxData[6] = (uint8_t)(usSize >> 8);
        gabTxRxData[7] = (uint8_t)(usSize);
        _cpy(gabTxRxData + 8, pbIv, 16);
        _cpy(gabTxRxData + 24, pbInput, usSize);
        if (bEnDe == DECRYPT)
            _cpy(abBuffer, pbInput + usSize - 16, 16);
        sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, usSize + 24);
        if (sRv != KSE_SUCCESS)
            return sRv;
        sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                        ((uint16_t)gabTxRxData[1]));
        if (((sRv == KSE_SUCCESS) && (usLen != usSize + 2)) ||
            ((sRv != KSE_SUCCESS) && (usLen != 2)))
            return KSE_FAIL_UNEXPECTED_RESP_LEN;
        if (sRv != KSE_SUCCESS)
            return sRv;
        _cpy(pbOutput, gabTxRxData + 2, usSize);
        pbInput += usSize;
        pbOutput += usSize;
        usInputSize -= usSize;
        if (bEnDe == ENCRYPT)
            pbIv = pbOutput - 16;
        else
            pbIv = abBuffer;
    }
    while (usInputSize > 0);

    return sRv;
}

//// Function Name : _kcmvpCtr
//// Parameters    : (Output) pbOutput
////                 (Input)  pbInput
////                 (Input)  usInputSize
////                 (Input)  bKeyType
////                 (Input)  usKeyIndex
////                 (Input)  pbCtr
////                 (Input)  bAlg
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : < AES / ARIA > CTR Mode Encryption / Decryption.
//// Description   : -  This function is internal function for AES / ARIA.
////                 -  Output size is 'usInputSize'.
////                 - 'usInputSize' should be non-zero.
////                 - 'bKeyType' should be one of belows.
////                    If 'bAlg' is KCMVP_AES,
////                    < KCMVP_AES128_KEY('40') / KCMVP_AES192_KEY('41') /
////                      KCMVP_AES256_KEY('42') >
////                    If 'bAlg' is KCMVP_ARIA,
////                    < KCMVP_ARIA128_KEY('50') / KCMVP_ARIA192_KEY('51') /
////                      KCMVP_ARIA256_KEY('52') >
////                 - 'usKeyIndex' should be 0 ~ 7.
////                 - 'pbCtr' should be 16-byte array.
////                 - 'bAlg' should be one of belows.
////                    < KCMVP_AES('40') / KCMVP_ARIA('50') >
int16_t _kcmvpCtr(uint8_t *pbOutput, uint8_t *pbInput, uint16_t usInputSize,
                  uint8_t bKeyType, uint16_t usKeyIndex, uint8_t *pbCtr,
                  uint8_t bAlg)
{
    int16_t sRv;
    uint8_t abCtr[16];
    uint16_t i, usLen, usSize, usSum;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if ((pbOutput == NULL_PTR) || (pbInput == NULL_PTR) ||
        (pbCtr == NULL_PTR) || (usInputSize == 0) ||
        (usKeyIndex >= MAX_KCMVP_KEY_COUNT) ||
        (((bAlg & 0xF0) ^ bKeyType) > 0x02))
        return KSE_FAIL_WRONG_INPUT;

    // CTR.
    _cpy(abCtr, pbCtr, 16);
    do
    {
        gabTxRxData[0] = 0x02;
        gabTxRxData[1] = (uint8_t)(bAlg | 0x06);
        gabTxRxData[2] = bKeyType;
        gabTxRxData[3] = (uint8_t)(usKeyIndex >> 8);
        gabTxRxData[4] = (uint8_t)(usKeyIndex);
        usSize = (usInputSize + 16 < MAX_IO_DATA_SIZE) ?
                  usInputSize : MAX_IO_DATA_SIZE - 16;
        gabTxRxData[5] = (uint8_t)(usSize >> 8);
        gabTxRxData[6] = (uint8_t)(usSize);
        _cpy(gabTxRxData + 7, abCtr, 16);
        _cpy(gabTxRxData + 23, pbInput, usSize);
        sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, usSize + 23);
        if (sRv != KSE_SUCCESS)
            return sRv;
        sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                        ((uint16_t)gabTxRxData[1]));
        if (((sRv == KSE_SUCCESS) && (usLen != usSize + 2)) ||
            ((sRv != KSE_SUCCESS) && (usLen != 2)))
            return KSE_FAIL_UNEXPECTED_RESP_LEN;
        if (sRv != KSE_SUCCESS)
            return sRv;
        _cpy(pbOutput, gabTxRxData + 2, usSize);
        pbInput += usSize;
        pbOutput += usSize;
        usInputSize -= usSize;
        usSum = usSize / 16;
        for (i = 15; i > 0; i--)
        {
            usSum += (uint16_t)abCtr[i];
            abCtr[i] = (uint8_t)usSum;
            usSum >>= 8;
        }
        abCtr[i] += (uint8_t)usSum;
    }
    while (usInputSize > 0);

    return sRv;
}

//// Function Name : _kcmvpGcm
//// Parameters    : (Output) pbOutput
////                 (Input)  pbInput
////                 (Input)  usInputSize
////                 (Input)  bKeyType
////                 (Input)  usKeyIndex
////                 (Input)  pbIv
////                 (Input)  usIvSize
////                 (Input)  pbAuth
////                 (Input)  usAuthSize
////                 (In/Out) pbTag
////                 (Input)  usTagSize
////                 (Input)  bEnDe
////                 (Input)  bAlg
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : < AES / ARIA > GCM Mode Encryption / Decryption.
//// Description   : -  This function is internal function for AES / ARIA.
////                 -  Output size is 'usInputSize'.
////                 - 'usInputSize' should be 0 ~ 2944 - 'usIvSize' -
////                   'usAuthSize'.
////                 - 'bKeyType' should be one of belows.
////                    If 'bAlg' is KCMVP_AES,
////                    < KCMVP_AES128_KEY('40') / KCMVP_AES192_KEY('41') /
////                      KCMVP_AES256_KEY('42') >
////                    If 'bAlg' is KCMVP_ARIA,
////                    < KCMVP_ARIA128_KEY('50') / KCMVP_ARIA192_KEY('51') /
////                      KCMVP_ARIA256_KEY('52') >
////                 - 'usKeyIndex' should be 0 ~ 7.
////                 - 'usIvSize' should be 1 ~ 2944 - 'usInputSize' -
////                   'usAuthSize'.
////                 - 'usAuthSize' should be 0 ~ 2944 - 'usIvSize' -
////                   'usInputSize'.
////                 - 'usTagSize' should be 0 ~ 16.
////                 - 'bEnDe' should be one of belows.
////                    < ENCRYPT(0) / DECRYPT(1) >
////                 -  If 'bEnDe' is ENCRYPT, 'pbTag' is output.
////                 -  If 'bEnDe' is DECRYPT, 'pbTag' is input.
////                 - 'bAlg' should be one of belows.
////                    < KCMVP_AES('40') / KCMVP_ARIA('50') >
int16_t _kcmvpGcm(uint8_t *pbOutput, uint8_t *pbInput, uint16_t usInputSize,
                  uint8_t bKeyType, uint16_t usKeyIndex, uint8_t *pbIv,
                  uint16_t usIvSize, uint8_t *pbAuth, uint16_t usAuthSize,
                  uint8_t *pbTag, uint16_t usTagSize, uint8_t bEnDe,
                  uint8_t bAlg)
{
    int16_t sRv;
    uint16_t usLen, usInSize, usOutSize;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if (((pbOutput == NULL_PTR) && (usInputSize != 0)) ||
        ((pbInput == NULL_PTR) && (usInputSize != 0)) ||
        (pbIv == NULL_PTR) ||
        ((pbAuth == NULL_PTR) && (usAuthSize != 0)) ||
        ((pbTag == NULL_PTR) && (usTagSize != 0)) ||
        (((bAlg & 0xF0) ^ bKeyType) > 0x02) ||
        (usKeyIndex >= MAX_KCMVP_KEY_COUNT) ||
        (usIvSize == 0) || (usTagSize > 16) ||
        (usInputSize + usIvSize + usAuthSize > MAX_IO_DATA_SIZE) ||
        ((bEnDe != ENCRYPT) && (bEnDe != DECRYPT)))
        return KSE_FAIL_WRONG_INPUT;

    // GCM.
    gabTxRxData[0] = 0x02;
    gabTxRxData[1] = (uint8_t)(bAlg | 0x07);
    gabTxRxData[2] = bKeyType;
    gabTxRxData[3] = (uint8_t)(usKeyIndex >> 8);
    gabTxRxData[4] = (uint8_t)(usKeyIndex);
    gabTxRxData[5] = bEnDe;
    gabTxRxData[6] = (uint8_t)(usIvSize >> 8);
    gabTxRxData[7] = (uint8_t)(usIvSize);
    gabTxRxData[8] = (uint8_t)(usInputSize >> 8);
    gabTxRxData[9] = (uint8_t)(usInputSize);
    gabTxRxData[10] = (uint8_t)(usAuthSize >> 8);
    gabTxRxData[11] = (uint8_t)(usAuthSize);
    gabTxRxData[12] = (uint8_t)(usTagSize >> 8);
    gabTxRxData[13] = (uint8_t)(usTagSize);
    usInSize = 14;
    usOutSize = usInputSize;
    _cpy(gabTxRxData + usInSize, pbIv, usIvSize);
    usInSize += usIvSize;
    _cpy(gabTxRxData + usInSize, pbInput, usInputSize);
    usInSize += usInputSize;
    _cpy(gabTxRxData + usInSize, pbAuth, usAuthSize);
    usInSize += usAuthSize;
    if (bEnDe == ENCRYPT)
        usOutSize += usTagSize;
    else
    {
        _cpy(gabTxRxData + usInSize, pbTag, usTagSize);
        usInSize += usTagSize;
    }
    sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, usInSize);
    if (sRv != KSE_SUCCESS)
        return sRv;
    sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                    ((uint16_t)gabTxRxData[1]));
    if (((sRv == KSE_SUCCESS) && (usLen != usOutSize + 2)) ||
        ((sRv != KSE_SUCCESS) && (usLen != 2)))
        return KSE_FAIL_UNEXPECTED_RESP_LEN;
    if (sRv != KSE_SUCCESS)
        return sRv;
    _cpy(pbOutput, gabTxRxData + 2, usInputSize);
    if (bEnDe == ENCRYPT)
        _cpy(pbTag, gabTxRxData + usInputSize + 2, usTagSize);

    return sRv;
}

//// Function Name : _kcmvpAesEcb
//// Parameters    : (Output) pbOutput
////                 (Input)  pbInput
////                 (Input)  usInputSize
////                 (Input)  bKeyType
////                 (Input)  usKeyIndex
////                 (Input)  bEnDe
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : < AES > ECB Mode Encryption / Decryption.
//// Description   : -  Output size is 'usInputSize'.
////                 - 'usInputSize' should be non-zero, multiples of 16.
////                 - 'bKeyType' should be one of belows.
////                    < KCMVP_AES128_KEY('40') / KCMVP_AES192_KEY('41') /
////                      KCMVP_AES256_KEY('42') >
////                 - 'usKeyIndex' should be 0 ~ 7.
////                 - 'bEnDe' should be one of belows.
////                    < ENCRYPT(0) / DECRYPT(1) >
int16_t _kcmvpAesEcb(uint8_t *pbOutput, uint8_t *pbInput, uint16_t usInputSize,
                     uint8_t bKeyType, uint16_t usKeyIndex, uint8_t bEnDe)
{
    return _kcmvpEcb(pbOutput, pbInput, usInputSize, bKeyType, usKeyIndex,
                     bEnDe, KCMVP_AES);
}

//// Function Name : _kcmvpAesCbc
//// Parameters    : (Output) pbOutput
////                 (Input)  pbInput
////                 (Input)  usInputSize
////                 (Input)  bKeyType
////                 (Input)  usKeyIndex
////                 (Input)  pbIv
////                 (Input)  bEnDe
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : < AES > CBC Mode Encryption / Decryption.
//// Description   : -  Output size is 'usInputSize'.
////                 - 'usInputSize' should be non-zero, multiples of 16.
////                 - 'bKeyType' should be one of belows.
////                    < KCMVP_AES128_KEY('40') / KCMVP_AES192_KEY('41') /
////                      KCMVP_AES256_KEY('42') >
////                 - 'usKeyIndex' should be 0 ~ 7.
////                 - 'pbIv' should be 16-byte array.
////                 - 'bEnDe' should be one of belows.
////                    < ENCRYPT(0) / DECRYPT(1) >
int16_t _kcmvpAesCbc(uint8_t *pbOutput, uint8_t *pbInput, uint16_t usInputSize,
                     uint8_t bKeyType, uint16_t usKeyIndex, uint8_t *pbIv,
                     uint8_t bEnDe)
{
    return _kcmvpCbc(pbOutput, pbInput, usInputSize, bKeyType, usKeyIndex, pbIv,
                     bEnDe, KCMVP_AES);
}

//// Function Name : _kcmvpAesCtr
//// Parameters    : (Output) pbOutput
////                 (Input)  pbInput
////                 (Input)  usInputSize
////                 (Input)  bKeyType
////                 (Input)  usKeyIndex
////                 (Input)  pbCtr
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : < AES > CTR Mode Encryption / Decryption.
//// Description   : -  Output size is 'usInputSize'.
////                 - 'usInputSize' should be non-zero.
////                 - 'bKeyType' should be one of belows.
////                    < KCMVP_AES128_KEY('40') / KCMVP_AES192_KEY('41') /
////                      KCMVP_AES256_KEY('42') >
////                 - 'usKeyIndex' should be 0 ~ 7.
////                 - 'pbCtr' should be 16-byte array.
int16_t _kcmvpAesCtr(uint8_t *pbOutput, uint8_t *pbInput, uint16_t usInputSize,
                     uint8_t bKeyType, uint16_t usKeyIndex, uint8_t *pbCtr)
{
    return _kcmvpCtr(pbOutput, pbInput, usInputSize, bKeyType, usKeyIndex,
                     pbCtr, KCMVP_AES);
}

//// Function Name : _kcmvpAesGcm
//// Parameters    : (Output) pbOutput
////                 (Input)  pbInput
////                 (Input)  usInputSize
////                 (Input)  bKeyType
////                 (Input)  usKeyIndex
////                 (Input)  pbIv
////                 (Input)  usIvSize
////                 (Input)  pbAuth
////                 (Input)  usAuthSize
////                 (In/Out) pbTag
////                 (Input)  usTagSize
////                 (Input)  bEnDe
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : < AES > GCM Mode Encryption / Decryption.
//// Description   : -  Output size is 'usInputSize'.
////                 - 'usInputSize' should be 0 ~ 2944 - 'usIvSize' -
////                   'usAuthSize'.
////                 - 'bKeyType' should be one of belows.
////                    < KCMVP_AES128_KEY('40') / KCMVP_AES192_KEY('41') /
////                      KCMVP_AES256_KEY('42') >
////                 - 'usKeyIndex' should be 0 ~ 7.
////                 - 'usIvSize' should be 1 ~ 2944 - 'usInputSize' -
////                   'usAuthSize'.
////                 - 'usAuthSize' should be 0 ~ 2944 - 'usIvSize' -
////                   'usInputSize'.
////                 - 'usTagSize' should be 0 ~ 16.
////                 - 'bEnDe' should be one of belows.
////                    < ENCRYPT(0) / DECRYPT(1) >
////                 -  If 'bEnDe' is ENCRYPT, 'pbTag' is output.
////                 -  If 'bEnDe' is DECRYPT, 'pbTag' is input.
int16_t _kcmvpAesGcm(uint8_t *pbOutput, uint8_t *pbInput, uint16_t usInputSize,
                     uint8_t bKeyType, uint16_t usKeyIndex, uint8_t *pbIv,
                     uint16_t usIvSize, uint8_t *pbAuth, uint16_t usAuthSize,
                     uint8_t *pbTag, uint16_t usTagSize, uint8_t bEnDe)
{
    return _kcmvpGcm(pbOutput, pbInput, usInputSize, bKeyType, usKeyIndex, pbIv,
                     usIvSize, pbAuth, usAuthSize, pbTag, usTagSize, bEnDe,
                     KCMVP_AES);
}

//// Function Name : _kcmvpAriaEcb
//// Parameters    : (Output) pbOutput
////                 (Input)  pbInput
////                 (Input)  usInputSize
////                 (Input)  bKeyType
////                 (Input)  usKeyIndex
////                 (Input)  bEnDe
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : < ARIA > ECB Mode Encryption / Decryption.
//// Description   : -  Output size is 'usInputSize'.
////                 - 'usInputSize' should be non-zero, multiples of 16.
////                 - 'bKeyType' should be one of belows.
////                    < KCMVP_ARIA128_KEY('50') / KCMVP_ARIA192_KEY('51') /
////                      KCMVP_ARIA256_KEY('52') >
////                 - 'usKeyIndex' should be 0 ~ 7.
////                 - 'bEnDe' should be one of belows.
////                    < ENCRYPT(0) / DECRYPT(1) >
int16_t _kcmvpAriaEcb(uint8_t *pbOutput, uint8_t *pbInput, uint16_t usInputSize,
                      uint8_t bKeyType, uint16_t usKeyIndex, uint8_t bEnDe)
{
    return _kcmvpEcb(pbOutput, pbInput, usInputSize, bKeyType, usKeyIndex,
                     bEnDe, KCMVP_ARIA);
}

//// Function Name : _kcmvpAriaCbc
//// Parameters    : (Output) pbOutput
////                 (Input)  pbInput
////                 (Input)  usInputSize
////                 (Input)  bKeyType
////                 (Input)  usKeyIndex
////                 (Input)  pbIv
////                 (Input)  bEnDe
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : < ARIA > CBC Mode Encryption / Decryption.
//// Description   : -  Output size is 'usInputSize'.
////                 - 'usInputSize' should be non-zero, multiples of 16.
////                 - 'bKeyType' should be one of belows.
////                    < KCMVP_ARIA128_KEY('50') / KCMVP_ARIA192_KEY('51') /
////                      KCMVP_ARIA256_KEY('52') >
////                 - 'usKeyIndex' should be 0 ~ 7.
////                 - 'pbIv' should be 16-byte array.
////                 - 'bEnDe' should be one of belows.
////                    < ENCRYPT(0) / DECRYPT(1) >
int16_t _kcmvpAriaCbc(uint8_t *pbOutput, uint8_t *pbInput, uint16_t usInputSize,
                      uint8_t bKeyType, uint16_t usKeyIndex, uint8_t *pbIv,
                      uint8_t bEnDe)
{
    return _kcmvpCbc(pbOutput, pbInput, usInputSize, bKeyType, usKeyIndex, pbIv,
                     bEnDe, KCMVP_ARIA);
}

//// Function Name : _kcmvpAriaCtr
//// Parameters    : (Output) pbOutput
////                 (Input)  pbInput
////                 (Input)  usInputSize
////                 (Input)  bKeyType
////                 (Input)  usKeyIndex
////                 (Input)  pbCtr
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : < ARIA > CTR Mode Encryption / Decryption.
//// Description   : -  Output size is 'usInputSize'.
////                 - 'usInputSize' should be non-zero.
////                 - 'bKeyType' should be one of belows.
////                    < KCMVP_ARIA128_KEY('50') / KCMVP_ARIA192_KEY('51') /
////                      KCMVP_ARIA256_KEY('52') >
////                 - 'usKeyIndex' should be 0 ~ 7.
////                 - 'pbCtr' should be 16-byte array.
int16_t _kcmvpAriaCtr(uint8_t *pbOutput, uint8_t *pbInput, uint16_t usInputSize,
                      uint8_t bKeyType, uint16_t usKeyIndex, uint8_t *pbCtr)
{
    return _kcmvpCtr(pbOutput, pbInput, usInputSize, bKeyType, usKeyIndex,
                     pbCtr, KCMVP_ARIA);
}

//// Function Name : _kcmvpAriaGcm
//// Parameters    : (Output) pbOutput
////                 (Input)  pbInput
////                 (Input)  usInputSize
////                 (Input)  bKeyType
////                 (Input)  usKeyIndex
////                 (Input)  pbIv
////                 (Input)  usIvSize
////                 (Input)  pbAuth
////                 (Input)  usAuthSize
////                 (In/Out) pbTag
////                 (Input)  usTagSize
////                 (Input)  bEnDe
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : < ARIA > GCM Mode Encryption / Decryption.
//// Description   : -  Output size is 'usInputSize'.
////                 - 'usInputSize' should be 0 ~ 2944 - 'usIvSize' -
////                   'usAuthSize'.
////                 - 'bKeyType' should be one of belows.
////                    < KCMVP_ARIA128_KEY('50') / KCMVP_ARIA192_KEY('51') /
////                      KCMVP_ARIA256_KEY('52') >
////                 - 'usKeyIndex' should be 0 ~ 7.
////                 - 'usIvSize' should be 1 ~ 2944 - 'usInputSize' -
////                   'usAuthSize'.
////                 - 'usAuthSize' should be 0 ~ 2944 - 'usIvSize' -
////                   'usInputSize'.
////                 - 'usTagSize' should be 0 ~ 16.
////                 - 'bEnDe' should be one of belows.
////                    < ENCRYPT(0) / DECRYPT(1) >
////                 -  If 'bEnDe' is ENCRYPT, 'pbTag' is output.
////                 -  If 'bEnDe' is DECRYPT, 'pbTag' is input.
int16_t _kcmvpAriaGcm(uint8_t *pbOutput, uint8_t *pbInput, uint16_t usInputSize,
                      uint8_t bKeyType, uint16_t usKeyIndex, uint8_t *pbIv,
                      uint16_t usIvSize, uint8_t *pbAuth, uint16_t usAuthSize,
                      uint8_t *pbTag, uint16_t usTagSize, uint8_t bEnDe)
{
    return _kcmvpGcm(pbOutput, pbInput, usInputSize, bKeyType, usKeyIndex, pbIv,
                     usIvSize, pbAuth, usAuthSize, pbTag, usTagSize, bEnDe,
                     KCMVP_ARIA);
}

//// Function Name : _kcmvpHashDsa
//// Parameters    : (In/Out) pbInOut1
////                 (In/Out) pbInOut2
////                 (Input)  pbMessage
////                 (Input)  usMessageSize
////                 (Input)  usKeyIndex
////                 (Input)  bAlg
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : < SHA2-256 / HMAC-SHA2-256 / ECDSA-P256 > Hash, MAC,
////                 Signature, Verification.
//// Description   : -  This function is internal function for SHA2-256 /
////                    HMAC-SHA2-256 / ECDSA-P256.
////                 - 'pbInOut1' should be 32-byte array.
////                 - 'pbInOut2' should be NULL_PTR or 32-byte array.
////                 - 'usKeyIndex' should be 0 ~ 7.
////                 - 'bAlg' should be one of belows.
////                    < KCMVP_SHA('60') / KCMVP_HMAC_GEN('70') /
////                      KCMVP_HMAC_VERI('78') / KCMVP_ECDSA_SIGN('80') /
////                      KCMVP_ECDSA_VERI('88') >
////                 -  If 'bAlg' is KCMVP_SHA,
////                   'pbInOut1' should be output 32-byte array,
////                   'pbInOut2', 'usKeyIndex' are not used.
////                 -  If 'bAlg' is KCMVP_HMAC_GEN,
////                   'pbInOut1' should be output 32-byte array,
////                   'pbInOut2' is not used.
////                 -  If 'bAlg' is KCMVP_HMAC_VERI,
////                   'pbInOut1' should be input 32-byte array,
////                   'pbInOut2' is not used.
////                 -  If 'bAlg' is KCMVP_ECDSA_SIGN,
////                   'pbInOut1' and 'pbInOut2' should be output 32-byte array.
////                 -  If 'bAlg' is KCMVP_ECDSA_VERI,
////                   'pbInOut1' and 'pbInOut2' should be input 32-byte array.
int16_t _kcmvpHashDsa(uint8_t *pbInOut1, uint8_t *pbInOut2, uint8_t *pbMessage,
                      uint16_t usMessageSize, uint16_t usKeyIndex, uint8_t bAlg)
{
    int16_t sRv;
    uint16_t i, usLen;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if (((pbMessage == NULL_PTR) && (usMessageSize != 0)) ||
        (pbInOut1 == NULL_PTR) ||
        (((bAlg == KCMVP_ECDSA_SIGN) || (bAlg == KCMVP_ECDSA_VERI)) &&
         (pbInOut2 == NULL_PTR)) ||
        ((bAlg != KCMVP_SHA) && (usKeyIndex >= MAX_KCMVP_KEY_COUNT)))
        return KSE_FAIL_WRONG_INPUT;

    // Hash, MAC, Signature, Verification - One step.
    if (usMessageSize <= MAX_IO_DATA_SIZE)
    {
        gabTxRxData[0] = 0x02;
        gabTxRxData[1] = (uint8_t)(bAlg | 0x04);
        i = 2;
        if (bAlg != KCMVP_SHA)
        {
            gabTxRxData[i++] = (uint8_t)(usKeyIndex >> 8);
            gabTxRxData[i++] = (uint8_t)(usKeyIndex);
        }
        gabTxRxData[i++] = (uint8_t)(usMessageSize >> 8);
        gabTxRxData[i++] = (uint8_t)(usMessageSize);
        _cpy(gabTxRxData + i, pbMessage, usMessageSize);
        i += usMessageSize;
        if ((bAlg == KCMVP_HMAC_VERI) || (bAlg == KCMVP_ECDSA_VERI))
        {
            _cpy(gabTxRxData + i, pbInOut1, 32);
            i += 32;
        }
        if (bAlg == KCMVP_ECDSA_VERI)
        {
            _cpy(gabTxRxData + i, pbInOut2, 32);
            i += 32;
        }
        sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, i);
    }

    // Hash, MAC, Signature, Verification - Multi steps.
    else
    {
        // Hash, MAC, Signature, Verification - Begin.
        gabTxRxData[0] = 0x02;
        gabTxRxData[1] = (uint8_t)(bAlg | 0x05);
        i = 2;
        if ((bAlg == KCMVP_HMAC_GEN) || (bAlg == KCMVP_HMAC_VERI))
        {
            gabTxRxData[i++] = (uint8_t)(usKeyIndex >> 8);
            gabTxRxData[i++] = (uint8_t)(usKeyIndex);
        }
        gabTxRxData[i++] = (uint8_t)(MAX_IO_DATA_SIZE >> 8);
        gabTxRxData[i++] = (uint8_t)(MAX_IO_DATA_SIZE);
        _cpy(gabTxRxData + i, pbMessage, MAX_IO_DATA_SIZE);
        sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData,
                             MAX_IO_DATA_SIZE + i);
        if (sRv != KSE_SUCCESS)
            return sRv;
        if (usLen != 2)
            return KSE_FAIL_UNEXPECTED_RESP_LEN;
        sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                        ((uint16_t)gabTxRxData[1]));
        if (sRv != KSE_SUCCESS)
            return sRv;
        pbMessage += MAX_IO_DATA_SIZE;
        usMessageSize -= MAX_IO_DATA_SIZE;

        // Hash, MAC, Signature, Verification - Mid.
        while (usMessageSize > MAX_IO_DATA_SIZE)
        {
            gabTxRxData[0] = 0x02;
            gabTxRxData[1] = (uint8_t)(bAlg | 0x06);
            gabTxRxData[2] = (uint8_t)(MAX_IO_DATA_SIZE >> 8);
            gabTxRxData[3] = (uint8_t)(MAX_IO_DATA_SIZE);
            _cpy(gabTxRxData + 4, pbMessage, MAX_IO_DATA_SIZE);
            sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData,
                                 MAX_IO_DATA_SIZE + 4);
            if (sRv != KSE_SUCCESS)
                return sRv;
            if (usLen != 2)
                return KSE_FAIL_UNEXPECTED_RESP_LEN;
            sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                            ((uint16_t)gabTxRxData[1]));
            if (sRv != KSE_SUCCESS)
                return sRv;
            pbMessage += MAX_IO_DATA_SIZE;
            usMessageSize -= MAX_IO_DATA_SIZE;
        }

        // Hash, MAC, Signature, Verification - End.
        gabTxRxData[0] = 0x02;
        gabTxRxData[1] = (uint8_t)(bAlg | 0x07);
        i = 2;
        if ((bAlg == KCMVP_ECDSA_SIGN) || (bAlg == KCMVP_ECDSA_VERI))
        {
            gabTxRxData[i++] = (uint8_t)(usKeyIndex >> 8);
            gabTxRxData[i++] = (uint8_t)(usKeyIndex);
        }
        gabTxRxData[i++] = (uint8_t)(usMessageSize >> 8);
        gabTxRxData[i++] = (uint8_t)(usMessageSize);
        _cpy(gabTxRxData + i, pbMessage, usMessageSize);
        i += usMessageSize;
        if ((bAlg == KCMVP_HMAC_VERI) || (bAlg == KCMVP_ECDSA_VERI))
        {
            _cpy(gabTxRxData + i, pbInOut1, 32);
            i += 32;
        }
        if (bAlg == KCMVP_ECDSA_VERI)
        {
            _cpy(gabTxRxData + i, pbInOut2, 32);
            i += 32;
        }
        sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, i);
    }

    if (sRv != KSE_SUCCESS)
        return sRv;
    sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                    ((uint16_t)gabTxRxData[1]));
    if (((sRv == KSE_SUCCESS) &&
         ((((bAlg == KCMVP_SHA) || (bAlg == KCMVP_HMAC_GEN)) &&
           (usLen != 34)) ||
          ((bAlg == KCMVP_ECDSA_SIGN) && (usLen != 66)))) ||
        (((sRv != KSE_SUCCESS) || (bAlg == KCMVP_HMAC_VERI) ||
          (bAlg == KCMVP_ECDSA_VERI)) &&
         (usLen != 2)))
        return KSE_FAIL_UNEXPECTED_RESP_LEN;
    if (sRv != KSE_SUCCESS)
        return sRv;
    if ((bAlg == KCMVP_SHA) || (bAlg == KCMVP_HMAC_GEN))
        _cpy(pbInOut1, gabTxRxData + 2, 32);
    else if (bAlg == KCMVP_ECDSA_SIGN)
    {
        _cpy(pbInOut1, gabTxRxData + 2, 32);
        _cpy(pbInOut2, gabTxRxData + 34, 32);
    }

    return sRv;
}

//// Function Name : _kcmvpSha256
//// Parameters    : (Output) pbHash
////                 (Input)  pbMessage
////                 (Input)  usMessageSize
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : < SHA2-256 >.
//// Description   : - 'pbHash' should be 32-byte array.
int16_t _kcmvpSha256(uint8_t *pbHash, uint8_t *pbMessage,
                     uint16_t usMessageSize)
{
    return _kcmvpHashDsa(pbHash, (uint8_t *)NULL_PTR, pbMessage, usMessageSize,
                         0, KCMVP_SHA);
}

//// Function Name : _kcmvpHmacSha256
//// Parameters    : (Output) pbHmac
////                 (Input)  pbMessage
////                 (Input)  usMessageSize
////                 (Input)  usKeyIndex
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : < HMAC-SHA2-256 > Generation.
//// Description   : - 'pbHmac' should be 32-byte array.
////                 - 'usKeyIndex' should be 0 ~ 7.
int16_t _kcmvpHmacSha256(uint8_t *pbHmac, uint8_t *pbMessage,
                         uint16_t usMessageSize, uint16_t usKeyIndex)
{
    return _kcmvpHashDsa(pbHmac, (uint8_t *)NULL_PTR, pbMessage, usMessageSize,
                         usKeyIndex, KCMVP_HMAC_GEN);
}

//// Function Name : _kcmvpHmacSha256Verify
//// Parameters    : (Input) pbMessage
////                 (Input) usMessageSize
////                 (Input) pbHmac
////                 (Input) usKeyIndex
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : < HMAC-SHA2-256 > Verification.
//// Description   : - 'pbHmac' should be 32-byte array.
////                 - 'usKeyIndex' should be 0 ~ 7.
int16_t _kcmvpHmacSha256Verify(uint8_t *pbMessage, uint16_t usMessageSize,
                               uint8_t *pbHmac, uint16_t usKeyIndex)
{
    return _kcmvpHashDsa(pbHmac, (uint8_t *)NULL_PTR, pbMessage, usMessageSize,
                         usKeyIndex, KCMVP_HMAC_VERI);
}

//// Function Name : _kcmvpEcdsaSign
//// Parameters    : (Output) pbR
////                 (Output) pbS
////                 (Input)  pbMessage
////                 (Input)  usMessageSize
////                 (Input)  usKeyIndex
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : < ECDSA-P256 > Signature.
//// Description   : - 'pbR' and 'pbS' should be 32-byte array.
////                 - 'usKeyIndex' should be 0 ~ 7.
int16_t _kcmvpEcdsaSign(uint8_t *pbR, uint8_t *pbS, uint8_t *pbMessage,
                        uint16_t usMessageSize, uint16_t usKeyIndex)
{
    return _kcmvpHashDsa(pbR, pbS, pbMessage, usMessageSize, usKeyIndex,
                         KCMVP_ECDSA_SIGN);
}

//// Function Name : _kcmvpEcdsaVerify
//// Parameters    : (Input) pbMessage
////                 (Input) usMessageSize
////                 (Input) pbR
////                 (Input) pbS
////                 (Input) usKeyIndex
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : < ECDSA-P256 > Verification.
//// Description   : - 'pbR' and 'pbS' should be 32-byte array.
////                 - 'usKeyIndex' should be 0 ~ 7.
int16_t _kcmvpEcdsaVerify(uint8_t *pbMessage, uint16_t usMessageSize,
                          uint8_t *pbR, uint8_t *pbS, uint16_t usKeyIndex)
{
    return _kcmvpHashDsa(pbR, pbS, pbMessage, usMessageSize, usKeyIndex,
                         KCMVP_ECDSA_VERI);
}

//// Function Name : _kcmvpEcdhExchKey
//// Parameters    : (Output) pbSesKey
////                 (Input)  pbPubKeytoken
////                 (Input)  usKeyIndex
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : < ECDH-P256 > Exchange Key.
//// Description   : - 'pbSesKey' and 'pbPubKeytoken' should be 64-byte array.
////                 - 'usKeyIndex' should be 0 ~ 7.
int16_t _kcmvpEcdhExchKey(uint8_t *pbSesKey, uint8_t *pbPubKeytoken,
                          uint16_t usKeyIndex)
{
    int16_t sRv;
    uint16_t usLen;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if ((pbSesKey == NULL_PTR) || (pbPubKeytoken == NULL_PTR) ||
        (usKeyIndex >= MAX_KCMVP_KEY_COUNT))
        return KSE_FAIL_WRONG_INPUT;

    // ECDH-P256.
    gabTxRxData[0] = 0x02;
    gabTxRxData[1] = 0x94;
    gabTxRxData[2] = (uint8_t)(usKeyIndex >> 8);
    gabTxRxData[3] = (uint8_t)(usKeyIndex);
    _cpy(gabTxRxData + 4, pbPubKeytoken, 64);
    sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, 68);
    if (sRv != KSE_SUCCESS)
        return sRv;
    sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                    ((uint16_t)gabTxRxData[1]));
    if (((sRv == KSE_SUCCESS) && (usLen != 66)) ||
        ((sRv != KSE_SUCCESS) && (usLen != 2)))
        return KSE_FAIL_UNEXPECTED_RESP_LEN;
    if (sRv != KSE_SUCCESS)
        return sRv;
    _cpy(pbSesKey, gabTxRxData + 2, 64);

    return sRv;
}

//// Function Name : _kcmvpGetClibState
//// Parameters    : (Output) pbClibState
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : Get CLIB state in KSE.
//// Description   : - 'pbClibState' should be 20-byte array.
int16_t _kcmvpGetClibState(uint8_t *pbClibState)
{
    int16_t sRv;
    uint16_t usLen;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if (pbClibState == NULL_PTR)
        return KSE_FAIL_WRONG_INPUT;

    // Get CLIB state.
    gabTxRxData[0] = 0x02;
    gabTxRxData[1] = 0x00;
    sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, 2);
    if (sRv != KSE_SUCCESS)
        return sRv;
    sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                    ((uint16_t)gabTxRxData[1]));
    if (((sRv == KSE_SUCCESS) && (usLen != 22)) ||
        ((sRv != KSE_SUCCESS) && (usLen != 2)))
        return KSE_FAIL_UNEXPECTED_RESP_LEN;
    if (sRv != KSE_SUCCESS)
        return sRv;
    _cpy(pbClibState, gabTxRxData + 2, 20);

    return sRv;
}

//// Cert //////////////////////////////////////////////////////////////////////

//// Function Name : _certGenerateKeypair
//// Parameters    : (Input) bKeyType
////                 (Input) usCertIndex
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : Generate device ECDSA / ECDH keypair in the KSE nonvolatile
////                 memory.
//// Description   : - 'bKeyType' should be one of belows.
////                    < CERT_KEY_INT_DEV_ECDSA('00') /
////                      CERT_KEY_INT_DEV_ECDH('01') /
////                      CERT_KEY_INT_DEV_ECC('02') >
////                 - 'usCertIndex' should be 0 ~ 95.
int16_t _certGenerateKeypair(uint8_t bKeyType, uint16_t usCertIndex)
{
    int16_t sRv;
    uint16_t usLen;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if (((bKeyType != CERT_KEY_INT_DEV_ECDSA) &&
         (bKeyType != CERT_KEY_INT_DEV_ECDH) &&
         (bKeyType != CERT_KEY_INT_DEV_ECC)) ||
        (usCertIndex >= MAX_CERT_KEY_COUNT))
        return KSE_FAIL_WRONG_INPUT;

    // Generate device ECDSA / ECDH keypair.
    gabTxRxData[0] = 0x04;
    gabTxRxData[1] = 0x00;
    gabTxRxData[2] = bKeyType;
    gabTxRxData[3] = (uint8_t)(usCertIndex >> 8);
    gabTxRxData[4] = (uint8_t)(usCertIndex);
    sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, 5);
    if (sRv != KSE_SUCCESS)
        return sRv;
    if (usLen != 2)
        return KSE_FAIL_UNEXPECTED_RESP_LEN;
    sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                    ((uint16_t)gabTxRxData[1]));

    return sRv;
}

//// Function Name : _certPutPriKey
//// Parameters    : (Input) bKeyType
////                 (Input) pbPriKey
////                 (Input) usCertIndex
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : Put device ECDSA / ECDH private key in the KSE nonvolatile
////                 memory.
//// Description   : - 'bKeyType' should be one of belows.
////                    < CERT_KEY_EXT_DEV_ECDSA('04') /
////                      CERT_KEY_EXT_DEV_ECDH('05') /
////                      CERT_KEY_EXT_DEV_ECC('06') >
////                 - 'pbPriKey' should be 32-byte array.
////                 - 'usCertIndex' should be 0 ~ 95.
int16_t _certPutPriKey(uint8_t bKeyType, uint8_t *pbPriKey,
                       uint16_t usCertIndex)
{
    int16_t sRv;
    uint16_t usLen;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if (((bKeyType != CERT_KEY_EXT_DEV_ECDSA) &&
         (bKeyType != CERT_KEY_EXT_DEV_ECDH) &&
         (bKeyType != CERT_KEY_EXT_DEV_ECC)) ||
        (pbPriKey == NULL_PTR) ||
        (usCertIndex >= MAX_CERT_KEY_COUNT))
        return KSE_FAIL_WRONG_INPUT;

    // Put device ECDSA / ECDH private key.
    gabTxRxData[0] = 0x04;
    gabTxRxData[1] = 0x01;
    gabTxRxData[2] = bKeyType;
    gabTxRxData[3] = (uint8_t)(usCertIndex >> 8);
    gabTxRxData[4] = (uint8_t)(usCertIndex);
    _cpy(gabTxRxData + 5, pbPriKey, 32);
    sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, 37);
    if (sRv != KSE_SUCCESS)
        return sRv;
    if (usLen != 2)
        return KSE_FAIL_UNEXPECTED_RESP_LEN;
    sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                    ((uint16_t)gabTxRxData[1]));

    return sRv;
}

//// Function Name : _certPutPubKey
//// Parameters    : (Input) bKeyType
////                 (Input) pbPubKey
////                 (Input) usCertIndex
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : Put device ECDSA / ECDH public key or CA ECDSA public key
////                 in the KSE nonvolatile memory.
//// Description   : - 'bKeyType' should be one of belows.
////                    < CERT_KEY_EXT_DEV_ECDSA('04') /
////                      CERT_KEY_EXT_DEV_ECDH('05') /
////                      CERT_KEY_EXT_DEV_ECC('06') /
////                      CERT_KEY_EXT_CA_ECDSA('07') >
////                 - 'pbPubKey' should be 64-byte array.
////                 - 'usCertIndex' should be 0 ~ 95.
int16_t _certPutPubKey(uint8_t bKeyType, uint8_t *pbPubKey,
                       uint16_t usCertIndex)
{
    int16_t sRv;
    uint16_t usLen;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if (((bKeyType != CERT_KEY_EXT_DEV_ECDSA) &&
         (bKeyType != CERT_KEY_EXT_DEV_ECDH) &&
         (bKeyType != CERT_KEY_EXT_DEV_ECC) &&
         (bKeyType != CERT_KEY_EXT_CA_ECDSA)) ||
        (pbPubKey == NULL_PTR) ||
        (usCertIndex >= MAX_CERT_KEY_COUNT))
        return KSE_FAIL_WRONG_INPUT;

    // Put the public key.
    gabTxRxData[0] = 0x04;
    gabTxRxData[1] = 0x03;
    gabTxRxData[2] = bKeyType;
    gabTxRxData[3] = (uint8_t)(usCertIndex >> 8);
    gabTxRxData[4] = (uint8_t)(usCertIndex);
    _cpy(gabTxRxData + 5, pbPubKey, 64);
    sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, 69);
    if (sRv != KSE_SUCCESS)
        return sRv;
    if (usLen != 2)
        return KSE_FAIL_UNEXPECTED_RESP_LEN;
    sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                    ((uint16_t)gabTxRxData[1]));

    return sRv;
}

//// Function Name : _certGetPriKey
//// Parameters    : (Output) pbKeyState
////                 (Output) pbPriKey
////                 (Input)  usCertIndex
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : Get private key and state in the KSE nonvolatile memory.
//// Description   : - 'pbKeyState' should be 1-byte pointer.
////                 - '*pbKeyState' would be one of belows.
////                    < CERT_KEY_INT_DEV_ECDSA('00') /
////                      CERT_KEY_INT_DEV_ECDH('01') /
////                      CERT_KEY_EXT_DEV_ECDSA('04') /
////                      CERT_KEY_EXT_DEV_ECDH('05') /
////                      CERT_KEY_EXT_DEV_ECC('06') / CERT_KEY_EMPTY('FF') >
////                 - 'pbPriKey' should be 32-byte array.
////                 - 'usCertIndex' should be 0 ~ 95.
int16_t _certGetPriKey(uint8_t *pbKeyState, uint8_t *pbPriKey,
                       uint16_t usCertIndex)
{
    int16_t sRv;
    uint16_t usLen;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if ((pbKeyState == NULL_PTR) || (pbPriKey == NULL_PTR) ||
        (usCertIndex >= MAX_CERT_KEY_COUNT))
        return KSE_FAIL_WRONG_INPUT;

    // Get private key and state.
    gabTxRxData[0] = 0x04;
    gabTxRxData[1] = 0x04;
    gabTxRxData[2] = (uint8_t)(usCertIndex >> 8);
    gabTxRxData[3] = (uint8_t)(usCertIndex);
    sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, 4);
    if (sRv != KSE_SUCCESS)
        return sRv;
    sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                    ((uint16_t)gabTxRxData[1]));
    if (((sRv == KSE_SUCCESS) && (usLen != 35)) ||
        ((sRv != KSE_SUCCESS) && (usLen != 2)))
        return KSE_FAIL_UNEXPECTED_RESP_LEN;
    if (sRv != KSE_SUCCESS)
        return sRv;
    *pbKeyState = gabTxRxData[2];
    _cpy(pbPriKey, gabTxRxData + 3, 32);

    return sRv;
}

//// Function Name : _certGetPriKeyState
//// Parameters    : (Output) pbKeyState
////                 (Input)  usCertIndex
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : Get private key state in the KSE nonvolatile memory.
//// Description   : - 'pbKeyState' should be 1-byte pointer.
////                 - '*pbKeyState' would be one of belows.
////                    < CERT_KEY_INT_DEV_ECDSA('00') /
////                      CERT_KEY_INT_DEV_ECDH('01') /
////                      CERT_KEY_EXT_DEV_ECDSA('04') /
////                      CERT_KEY_EXT_DEV_ECDH('05') /
////                      CERT_KEY_EXT_DEV_ECC('06') / CERT_KEY_EMPTY('FF') >
////                 - 'usCertIndex' should be 0 ~ 95.
int16_t _certGetPriKeyState(uint8_t *pbKeyState, uint16_t usCertIndex)
{
    int16_t sRv;
    uint16_t usLen;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if ((pbKeyState == NULL_PTR) ||
        (usCertIndex >= MAX_CERT_KEY_COUNT))
        return KSE_FAIL_WRONG_INPUT;

    // Get private key state.
    gabTxRxData[0] = 0x04;
    gabTxRxData[1] = 0x05;
    gabTxRxData[2] = (uint8_t)(usCertIndex >> 8);
    gabTxRxData[3] = (uint8_t)(usCertIndex);
    sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, 4);
    if (sRv != KSE_SUCCESS)
        return sRv;
    sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                    ((uint16_t)gabTxRxData[1]));
    if (((sRv == KSE_SUCCESS) && (usLen != 3)) ||
        ((sRv != KSE_SUCCESS) && (usLen != 2)))
        return KSE_FAIL_UNEXPECTED_RESP_LEN;
    if (sRv != KSE_SUCCESS)
        return sRv;
    *pbKeyState = gabTxRxData[2];

    return sRv;
}

//// Function Name : _certGetPubKey
//// Parameters    : (Output) pbKeyState
////                 (Output) pbPubKey
////                 (Input)  usCertIndex
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : Get public key and state in the KSE nonvolatile memory.
//// Description   : - 'pbKeyState' should be 1-byte pointer.
////                 - '*pbKeyState' would be one of belows.
////                    < CERT_KEY_INT_DEV_ECDSA('00') /
////                      CERT_KEY_INT_DEV_ECDH('01') /
////                      CERT_KEY_EXT_DEV_ECDSA('04') /
////                      CERT_KEY_EXT_DEV_ECDH('05') /
////                      CERT_KEY_EXT_DEV_ECC('06') /
////                      CERT_KEY_EXT_CA_ECDSA('07') / CERT_KEY_EMPTY('FF') >
////                 - 'pbPubKey' should be 64-byte array.
////                 - 'usCertIndex' should be 0 ~ 95.
int16_t _certGetPubKey(uint8_t *pbKeyState, uint8_t *pbPubKey,
                       uint16_t usCertIndex)
{
    int16_t sRv;
    uint16_t usLen;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if ((pbKeyState == NULL_PTR) || (pbPubKey == NULL_PTR) ||
        (usCertIndex >= MAX_CERT_KEY_COUNT))
        return KSE_FAIL_WRONG_INPUT;

    // Get public key and state.
    gabTxRxData[0] = 0x04;
    gabTxRxData[1] = 0x06;
    gabTxRxData[2] = (uint8_t)(usCertIndex >> 8);
    gabTxRxData[3] = (uint8_t)(usCertIndex);
    sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, 4);
    if (sRv != KSE_SUCCESS)
        return sRv;
    sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                    ((uint16_t)gabTxRxData[1]));
    if (((sRv == KSE_SUCCESS) && (usLen != 67)) ||
        ((sRv != KSE_SUCCESS) && (usLen != 2)))
        return KSE_FAIL_UNEXPECTED_RESP_LEN;
    if (sRv != KSE_SUCCESS)
        return sRv;
    *pbKeyState = gabTxRxData[2];
    _cpy(pbPubKey, gabTxRxData + 3, 64);

    return sRv;
}

//// Function Name : _certGetPubKeyCsr
//// Parameters    : (Output) pbCsr
////                 (Input)  usCertIndex
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : Get CSR(Certificate Signing Request) of public key in the
////                 KSE nonvolatile memory.
//// Description   : - 'pbCsr' should be 500-byte array.
////                 - 'usCertIndex' should be 0 ~ 95.
int16_t _certGetPubKeyCsr(uint8_t *pbCsr, uint16_t usCertIndex)
{
    int16_t sRv;
    uint16_t usLen;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if ((pbCsr == NULL_PTR) ||
        (usCertIndex >= MAX_CERT_KEY_COUNT))
        return KSE_FAIL_WRONG_INPUT;

    // Get CSR(Certificate Signing Request) of public key.
    gabTxRxData[0] = 0x04;
    gabTxRxData[1] = 0x07;
    gabTxRxData[2] = (uint8_t)(usCertIndex >> 8);
    gabTxRxData[3] = (uint8_t)(usCertIndex);
    sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, 4);
    if (sRv != KSE_SUCCESS)
        return sRv;
    sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                    ((uint16_t)gabTxRxData[1]));
    if (((sRv == KSE_SUCCESS) && (usLen != 502)) ||
        ((sRv != KSE_SUCCESS) && (usLen != 2)))
        return KSE_FAIL_UNEXPECTED_RESP_LEN;
    if (sRv != KSE_SUCCESS)
        return sRv;
    _cpy(pbCsr, gabTxRxData + 2, 500);

    return sRv;
}

//// Function Name : _certPutCert
//// Parameters    : (Input) pbCert
////                 (Input) usCertIndex
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : Put certificate in the KSE nonvolatile memory.
//// Description   : - 'pbCert' should be 500-byte array.
////                 - 'usCertIndex' should be 0 ~ 95.
int16_t _certPutCert(uint8_t *pbCert, uint16_t usCertIndex)
{
    int16_t sRv;
    uint16_t usLen;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if ((pbCert == NULL_PTR) ||
        (usCertIndex >= MAX_CERT_KEY_COUNT))
        return KSE_FAIL_WRONG_INPUT;

    // Put certificate.
    gabTxRxData[0] = 0x04;
    gabTxRxData[1] = 0x10;
    gabTxRxData[2] = (uint8_t)(usCertIndex >> 8);
    gabTxRxData[3] = (uint8_t)(usCertIndex);
    _cpy(gabTxRxData + 4, pbCert, 500);
    sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, 504);
    if (sRv != KSE_SUCCESS)
        return sRv;
    if (usLen != 2)
        return KSE_FAIL_UNEXPECTED_RESP_LEN;
    sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                    ((uint16_t)gabTxRxData[1]));

    return sRv;
}

//// Function Name : _certPutCertPubKey
//// Parameters    : (Input) pbCert
////                 (Input) usCertIndex
////                 (Input) usCaCertIndex
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : Put certificate and public key in the KSE nonvolatile
////                 memory.
//// Description   : - 'pbCert' should be 500-byte array.
////                 - 'usCertIndex' and 'usCaCertIndex' should be 0 ~ 95.
////                 -  This function verifies the input certificate.
////                 -  If Cert(Index) == CaCert(Index), this function verifies
////                    the input certificate as ROOT CA certificate.
int16_t _certPutCertPubKey(uint8_t *pbCert, uint16_t usCertIndex,
                           uint16_t usCaCertIndex)
{
    int16_t sRv;
    uint16_t usLen;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if ((pbCert == NULL_PTR) ||
        (usCertIndex >= MAX_CERT_KEY_COUNT) ||
        (usCaCertIndex >= MAX_CERT_KEY_COUNT))
        return KSE_FAIL_WRONG_INPUT;

    // Put certificate and put public key.
    gabTxRxData[0] = 0x04;
    gabTxRxData[1] = 0x11;
    gabTxRxData[2] = (uint8_t)(usCertIndex >> 8);
    gabTxRxData[3] = (uint8_t)(usCertIndex);
    gabTxRxData[4] = (uint8_t)(usCaCertIndex >> 8);
    gabTxRxData[5] = (uint8_t)(usCaCertIndex);
    _cpy(gabTxRxData + 6, pbCert, 500);
    sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, 506);
    if (sRv != KSE_SUCCESS)
        return sRv;
    if (usLen != 2)
        return KSE_FAIL_UNEXPECTED_RESP_LEN;
    sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                    ((uint16_t)gabTxRxData[1]));

    return sRv;
}

//// Function Name : _certGetCert
//// Parameters    : (Output) pbCert
////                 (Input)  usCertIndex
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : Get certificate in the KSE nonvolatile memory.
//// Description   : - 'pbCert' should be 500-byte array.
////                 - 'usCertIndex' should be 0 ~ 95.
int16_t _certGetCert(uint8_t *pbCert, uint16_t usCertIndex)
{
    int16_t sRv;
    uint16_t usLen;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if ((pbCert == NULL_PTR) ||
        (usCertIndex >= MAX_CERT_KEY_COUNT))
        return KSE_FAIL_WRONG_INPUT;

    // Get certificate.
    gabTxRxData[0] = 0x04;
    gabTxRxData[1] = 0x12;
    gabTxRxData[2] = (uint8_t)(usCertIndex >> 8);
    gabTxRxData[3] = (uint8_t)(usCertIndex);
    sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, 4);
    if (sRv != KSE_SUCCESS)
        return sRv;
    sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                    ((uint16_t)gabTxRxData[1]));
    if (((sRv == KSE_SUCCESS) && (usLen != 502)) ||
        ((sRv != KSE_SUCCESS) && (usLen != 2)))
        return KSE_FAIL_UNEXPECTED_RESP_LEN;
    if (sRv != KSE_SUCCESS)
        return sRv;
    _cpy(pbCert, gabTxRxData + 2, 500);

    return sRv;
}

//// Function Name : _certGetCertField
//// Parameters    : (Output) pbField
////                 (In/Out) pusSize
////                 (Input)  bFieldIndex
////                 (Input)  usCertIndex
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : Get certificate field in the KSE nonvolatile memory.
//// Description   : -  If 'pbField' is NULL_PTR, only '*pusSize' is output.
////                 - 'Input *pusSize' is 'pbField' array size.
////                 - 'Output *pusSize' is field data size.
////                 - 'bFieldIndex' should be one of belows.
////                    < CERT_SN('00') / CERT_ISSUER_DN('01') /
////                      CERT_ISSUER_CN('02') / CERT_VALIDITY('03') /
////                      CERT_SUBJ_DN('04') / CERT_SUBJ_CN('05') /
////                      CERT_SUBJ_PUBKEY('06') / CERT_AUTH_KEY_ID('07') /
////                      CERT_SUBJ_KEY_ID('08') / CERT_KEY_USAGE('09') /
////                      CERT_BC('0A') >
////                 - 'usCertIndex' should be 0 ~ 95.
int16_t _certGetCertField(uint8_t *pbField, uint16_t *pusSize,
                          uint8_t bFieldIndex, uint16_t usCertIndex)
{
    int16_t sRv;
    uint16_t usLen, usSize;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if ((pusSize == NULL_PTR) || (bFieldIndex > CERT_BC) ||
        (usCertIndex >= MAX_CERT_KEY_COUNT))
        return KSE_FAIL_WRONG_INPUT;

    // Get certificate field.
    gabTxRxData[0] = 0x04;
    gabTxRxData[1] = 0x13;
    gabTxRxData[2] = (uint8_t)(usCertIndex >> 8);
    gabTxRxData[3] = (uint8_t)(usCertIndex);
    gabTxRxData[4] = bFieldIndex;
    sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, 5);
    if (sRv != KSE_SUCCESS)
        return sRv;
    sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                    ((uint16_t)gabTxRxData[1]));
    if (((sRv == KSE_SUCCESS) && (usLen < 2)) ||
        ((sRv != KSE_SUCCESS) && (usLen != 2)))
        return KSE_FAIL_UNEXPECTED_RESP_LEN;
    if (sRv != KSE_SUCCESS)
        return sRv;
    usSize = usLen - 2;
    usLen = (usSize < *pusSize) ? usSize : *pusSize;
    if (pbField != NULL_PTR)
        _cpy(pbField, gabTxRxData + 2, usLen);
    *pusSize = usSize;

    return sRv;
}

//// Function Name : _certEraseCertKey
//// Parameters    : (Input) bOpt
////                 (Input) usCertIndex
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : Erase certificate, public key, private key in the KSE
////                 nonvolatile memory.
//// Description   : - 'bOpt' should be one of belows.
////                    < SELECT_CERT('01') / SELECT_PUB_KEY('02') /
////                      SELECT_PRI_KEY('04') / SELECT_ALL('07') >
////                 - 'usCertIndex' should be 0 ~ 95.
int16_t _certEraseCertKey(uint8_t bOpt, uint16_t usCertIndex)
{
    int16_t sRv;
    uint16_t usLen;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if (((bOpt != SELECT_CERT) && (bOpt != SELECT_PUB_KEY) &&
         (bOpt != SELECT_PRI_KEY) && (bOpt != SELECT_ALL)) ||
        (usCertIndex >= MAX_CERT_KEY_COUNT))
        return KSE_FAIL_WRONG_INPUT;

    // Erase certificate, public key, private key.
    gabTxRxData[0] = 0x04;
    gabTxRxData[1] = 0x20;
    gabTxRxData[2] = (uint8_t)(usCertIndex >> 8);
    gabTxRxData[3] = (uint8_t)(usCertIndex);
    gabTxRxData[4] = bOpt;
    sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, 5);
    if (sRv != KSE_SUCCESS)
        return sRv;
    if (usLen != 2)
        return KSE_FAIL_UNEXPECTED_RESP_LEN;
    sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                    ((uint16_t)gabTxRxData[1]));

    return sRv;
}

//// kseTLS ////////////////////////////////////////////////////////////////////

//// Function Name : _ksetlsOpen
//// Parameters    : (Input) socketDesc
////                 (Input) bMode
////                 (Input) bEndpoint
////                 (Input) usCertIndex
////                 (Input) usCaCertIndex
////                 (Input) usRfu
////                 (Input) bSaveOption
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0) / KSETLS_ERR_...(<0)
//// Operation     : Open kseTLS channel.
//// Description   : - 'bMode' should be one of belows.
////                    < KSETLS_MODE_TLS(0) / KSETLS_MODE_DTLS(1) >
////                 - 'bEndpoint' should be one of belows.
////                    < KSETLS_CLIENT(0) / KSETLS_SERVER(1) >
////                 - 'usCertIndex' and 'usCaCertIndex' should be 0 ~ 95.
////                 - 'bSaveOption' should be one of belows.
////                    < NONE('00') / SESSION('08') >
int16_t _ksetlsOpen(SOCKET socketDesc, uint8_t bMode, uint8_t bEndpoint,
                    uint16_t usCertIndex, uint16_t usCaCertIndex,
                    uint16_t usRfu, uint8_t bSaveOption)
{
    int16_t sRv;
    uint16_t usLen;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if ((socketDesc == INVALID_SOCKET) ||
        ((bMode != KSETLS_MODE_TLS) && (bMode != KSETLS_MODE_DTLS)) ||
        ((bEndpoint != KSETLS_CLIENT) && (bEndpoint != KSETLS_SERVER)) ||
        (usCertIndex >= MAX_CERT_KEY_COUNT) ||
        (usCaCertIndex >= MAX_CERT_KEY_COUNT) ||
        ((bSaveOption != NONE) && (bSaveOption != SESSION)))
        return KSE_FAIL_WRONG_INPUT;

    // Setup kseTLS channel.
    gabTxRxData[0] = 0x06;
    gabTxRxData[1] = 0x00;
    gabTxRxData[2] = (uint8_t)((int32_t)socketDesc >> 24);
    gabTxRxData[3] = (uint8_t)((int32_t)socketDesc >> 16);
    gabTxRxData[4] = (uint8_t)((int32_t)socketDesc >> 8);
    gabTxRxData[5] = (uint8_t)((int32_t)socketDesc);
    gabTxRxData[6] = bMode;
    gabTxRxData[7] = bEndpoint;
    gabTxRxData[8] = (uint8_t)(usCertIndex >> 8);
    gabTxRxData[9] = (uint8_t)(usCertIndex);
    gabTxRxData[10] = (uint8_t)(usCaCertIndex >> 8);
    gabTxRxData[11] = (uint8_t)(usCaCertIndex);
    gabTxRxData[12] = 0xFF;
    gabTxRxData[13] = 0xFF;
    gabTxRxData[14] = bSaveOption;
    sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, 15);
    if (sRv != KSE_SUCCESS)
        return sRv;
    if (usLen != 2)
        return KSE_FAIL_UNEXPECTED_RESP_LEN;
    sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                    ((uint16_t)gabTxRxData[1]));

    gusTxRxDataOffset = 0;
    gusTxRxDataLength = 0;

    return sRv;
}

//// Function Name : _ksetlsClose
//// Parameters    : (Input) socketDesc
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0) / KSETLS_ERR_...(<0)
//// Operation     : Close kseTLS channel.
//// Description   : -  None.
int16_t _ksetlsClose(SOCKET socketDesc)
{
    int16_t sRv;
    uint16_t usLen;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if (socketDesc == INVALID_SOCKET)
        return KSE_FAIL_WRONG_INPUT;

    // Close kseTLS channel.
    gabTxRxData[0] = 0x06;
    gabTxRxData[1] = 0x02;
    gabTxRxData[2] = (uint8_t)((int32_t)socketDesc >> 24);
    gabTxRxData[3] = (uint8_t)((int32_t)socketDesc >> 16);
    gabTxRxData[4] = (uint8_t)((int32_t)socketDesc >> 8);
    gabTxRxData[5] = (uint8_t)((int32_t)socketDesc);
    sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, 6);
    if (sRv != KSE_SUCCESS)
        return sRv;
    if (usLen != 2)
        return KSE_FAIL_UNEXPECTED_RESP_LEN;
    sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                    ((uint16_t)gabTxRxData[1]));
    if (sRv != KSE_SUCCESS)
        return sRv;

    gusTxRxDataOffset = 0;
    gusTxRxDataLength = 0;

    return sRv;
}

//// Function Name : _ksetlsTlsClientHandshake
//// Parameters    : (Input) socketDesc
////                 (Input) bType
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0) / KSETLS_ERR_...(<0)
//// Operation     : TLS Client Handshake.
//// Description   : - 'bType' should be one of belows.
////                    < KSETLS_FULL_HANDSHAKE(0) / KSETLS_ABBR_HANDSHAKE(1) >
////                 -  If 'bType' is KSETLS_FULL_HANDSHAKE, TLS client and
////                    server perform a full handshake.
////                 -  If 'bType' is KSETLS_ABBR_HANDSHAKE, TLS client and
////                    server perform an abbreviated handshake(a session
////                    resuming).
int16_t _ksetlsTlsClientHandshake(SOCKET socketDesc, uint8_t bType)
{
    uint8_t bNextInput;
    int16_t sRv, sInLen, sOutLen;
    uint16_t usLen, usRecordLen, usUnusedInputLen;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if (socketDesc == INVALID_SOCKET)
        return KSE_FAIL_WRONG_INPUT;

    // TLS Client Handshake.
    bNextInput = KSE_FALSE;
    do
    {
        // Receive Record.
        if (bNextInput == KSE_TRUE)
        {
            if (gusTxRxDataLength == 0)
            {
                sInLen = _tlsRecv(gabTxRxData + 9, MAX_IO_DATA_SIZE,
                                  socketDesc);
                if (sInLen < 0)
                    return sInLen;
            }
            else
            {
                _cpy(gabTxRxData + 9, gabTxRxData + gusTxRxDataOffset,
                     gusTxRxDataLength);
                sInLen = gusTxRxDataLength;
            }
        }
        else
            sInLen = 0;

        // Process Handshake.
        gabTxRxData[0] = 0x06;
        gabTxRxData[1] = 0x10;
        gabTxRxData[2] = (uint8_t)((int32_t)socketDesc >> 24);
        gabTxRxData[3] = (uint8_t)((int32_t)socketDesc >> 16);
        gabTxRxData[4] = (uint8_t)((int32_t)socketDesc >> 8);
        gabTxRxData[5] = (uint8_t)((int32_t)socketDesc);
        gabTxRxData[6] = bType;
        gabTxRxData[7] = (uint8_t)(sInLen >> 8);
        gabTxRxData[8] = (uint8_t)(sInLen);
        sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, sInLen + 9);
        if (sRv != KSE_SUCCESS)
            return sRv;
        sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                        ((uint16_t)gabTxRxData[1]));
        if ((sRv == KSE_SUCCESS) && (usLen < 7))
            return KSE_FAIL_UNEXPECTED_RESP_LEN;
        if (usLen > 2)
        {
            usUnusedInputLen = ((uint16_t)gabTxRxData[2] << 8) |
                               ((uint16_t)gabTxRxData[3]);
            bNextInput = gabTxRxData[4];
            usRecordLen = ((uint16_t)gabTxRxData[5] << 8) |
                          ((uint16_t)gabTxRxData[6]);
        }
        else
        {
            usUnusedInputLen = 0;
            bNextInput = KSE_FALSE;
            usRecordLen = 0;
        }

        // Send Record.
        if (usRecordLen > 0)
        {
            sOutLen = _tlsSend(socketDesc, gabTxRxData + 7, usRecordLen);
            if (sOutLen < 0)
                return sOutLen;
        }

        // Next.
        if (usUnusedInputLen > 0)
        {
            gusTxRxDataOffset = sInLen + 9 - usUnusedInputLen;
            gusTxRxDataLength = usUnusedInputLen;
        }
        else
            gusTxRxDataLength = 0;
    }
    while (sRv == KSE_SUCCESS);

    if (sRv == KSETLS_HANDSHAKE_DONE)
        sRv = KSE_SUCCESS;
    return sRv;
}

//// Function Name : _ksetlsTlsServerHandshake
//// Parameters    : (Input) socketDesc
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0) / KSETLS_ERR_...(<0)
//// Operation     : TLS Server Handshake.
//// Description   : -  None.
int16_t _ksetlsTlsServerHandshake(SOCKET socketDesc)
{
    uint8_t bNextInput;
    int16_t sRv, sInLen, sOutLen;
    uint16_t usLen, usRecordLen, usUnusedInputLen;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if (socketDesc == INVALID_SOCKET)
        return KSE_FAIL_WRONG_INPUT;

    // TLS Server Handshake.
    bNextInput = KSE_TRUE;
    do
    {
        // Receive Record.
        if (bNextInput == KSE_TRUE)
        {
            if (gusTxRxDataLength == 0)
            {
                sInLen = _tlsRecv(gabTxRxData + 8, MAX_IO_DATA_SIZE,
                                  socketDesc);
                if (sInLen < 0)
                    return sInLen;
            }
            else
            {
                _cpy(gabTxRxData + 8, gabTxRxData + gusTxRxDataOffset,
                     gusTxRxDataLength);
                sInLen = gusTxRxDataLength;
            }
        }
        else
            sInLen = 0;

        // Process Handshake.
        gabTxRxData[0] = 0x06;
        gabTxRxData[1] = 0x11;
        gabTxRxData[2] = (uint8_t)((int32_t)socketDesc >> 24);
        gabTxRxData[3] = (uint8_t)((int32_t)socketDesc >> 16);
        gabTxRxData[4] = (uint8_t)((int32_t)socketDesc >> 8);
        gabTxRxData[5] = (uint8_t)((int32_t)socketDesc);
        gabTxRxData[6] = (uint8_t)(sInLen >> 8);
        gabTxRxData[7] = (uint8_t)(sInLen);
        sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, sInLen + 8);
        if (sRv != KSE_SUCCESS)
            return sRv;
        sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                        ((uint16_t)gabTxRxData[1]));
        if ((sRv == KSE_SUCCESS) && (usLen < 7))
            return KSE_FAIL_UNEXPECTED_RESP_LEN;
        if (usLen > 2)
        {
            usUnusedInputLen = ((uint16_t)gabTxRxData[2] << 8) |
                               ((uint16_t)gabTxRxData[3]);
            bNextInput = gabTxRxData[4];
            usRecordLen = ((uint16_t)gabTxRxData[5] << 8) |
                          ((uint16_t)gabTxRxData[6]);
        }
        else
        {
            usUnusedInputLen = 0;
            bNextInput = KSE_FALSE;
            usRecordLen = 0;
        }

        // Send Record.
        if (usRecordLen > 0)
        {
            sOutLen = _tlsSend(socketDesc, gabTxRxData + 7, usRecordLen);
            if (sOutLen < 0)
                return sOutLen;
        }

        // Next.
        if (usUnusedInputLen > 0)
        {
            gusTxRxDataOffset = sInLen + 8 - usUnusedInputLen;
            gusTxRxDataLength = usUnusedInputLen;
        }
        else
            gusTxRxDataLength = 0;
    }
    while (sRv == KSE_SUCCESS);

    if (sRv == KSETLS_HANDSHAKE_DONE)
        sRv = KSE_SUCCESS;
    return sRv;
}

//// Function Name : _ksetlsTlsRead
//// Parameters    : (Output) pbInAppData
////                 (Output) psInAppDataLen
////                 (Input)  socketDesc
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0) / KSETLS_ERR_...(<0)
//// Operation     : TLS Read Application Data.
//// Description   : - 'pbInAppData' should be minimum 2944-byte array.
int16_t _ksetlsTlsRead(uint8_t *pbInAppData, int16_t *psInAppDataLen,
                       SOCKET socketDesc)
{
    int16_t sRv, sInLen, sOutLen;
    uint16_t usLen, usRecordLen, usUnusedInputLen, usMessageLen;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if ((pbInAppData == NULL_PTR) || (psInAppDataLen == NULL_PTR) ||
        (socketDesc == INVALID_SOCKET))
        return KSE_FAIL_WRONG_INPUT;

    // Read Record.
    if (gusTxRxDataLength == 0)
    {
        // Receive Record.
        sInLen = _tlsRecv(gabTxRxData + 8, MAX_IO_DATA_SIZE, socketDesc);
        if (sInLen < 0)
            return sInLen;
    }
    else
    {
        _cpy(gabTxRxData + 8, gabTxRxData + gusTxRxDataOffset,
             gusTxRxDataLength);
        sInLen = gusTxRxDataLength;
    }

    // Parse Record.
    gabTxRxData[0] = 0x06;
    gabTxRxData[1] = 0x20;
    gabTxRxData[2] = (uint8_t)((int32_t)socketDesc >> 24);
    gabTxRxData[3] = (uint8_t)((int32_t)socketDesc >> 16);
    gabTxRxData[4] = (uint8_t)((int32_t)socketDesc >> 8);
    gabTxRxData[5] = (uint8_t)((int32_t)socketDesc);
    gabTxRxData[6] = (uint8_t)(sInLen >> 8);
    gabTxRxData[7] = (uint8_t)(sInLen);
    sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, sInLen + 8);
    if (sRv != KSE_SUCCESS)
        return sRv;
    sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                    ((uint16_t)gabTxRxData[1]));
    if (((sRv == KSE_SUCCESS) && (usLen < 6)) ||
        ((sRv != KSE_SUCCESS) && (usLen != 2) && (usLen != 4)))
        return KSE_FAIL_UNEXPECTED_RESP_LEN;
    if (sRv != KSE_SUCCESS)
    {
        if (usLen == 4)
        {
            // Set Alert.
            gabTxRxData[6] = gabTxRxData[2];
            gabTxRxData[7] = gabTxRxData[3];
            gabTxRxData[0] = 0x06;
            gabTxRxData[1] = 0x22;
            gabTxRxData[2] = (uint8_t)((int32_t)socketDesc >> 24);
            gabTxRxData[3] = (uint8_t)((int32_t)socketDesc >> 16);
            gabTxRxData[4] = (uint8_t)((int32_t)socketDesc >> 8);
            gabTxRxData[5] = (uint8_t)((int32_t)socketDesc);
            sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, 8);
            if (sRv != KSE_SUCCESS)
                return sRv;
            sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                            ((uint16_t)gabTxRxData[1]));
            if (((sRv == KSE_SUCCESS) && (usLen < 4)) ||
                ((sRv != KSE_SUCCESS) && (usLen != 2)))
                return KSE_FAIL_UNEXPECTED_RESP_LEN;
            if (sRv != KSE_SUCCESS)
                return sRv;
            usRecordLen = ((uint16_t)gabTxRxData[2] << 8) |
                          ((uint16_t)gabTxRxData[3]);

            // Send Record.
            if (usRecordLen > 0)
            {
                sOutLen = _tlsSend(socketDesc, gabTxRxData + 4, usRecordLen);
                if (sOutLen < 0)
                    return sOutLen;
            }
        }
        gusTxRxDataLength = 0;

        return sRv;
    }
    usUnusedInputLen = ((uint16_t)gabTxRxData[2] << 8) |
                       ((uint16_t)gabTxRxData[3]);
    usMessageLen = ((uint16_t)gabTxRxData[4] << 8) | ((uint16_t)gabTxRxData[5]);
    _cpy(pbInAppData, gabTxRxData + 6, usMessageLen);
    *psInAppDataLen = usMessageLen;

    if (usUnusedInputLen > 0)
    {
        gusTxRxDataOffset = sInLen + 8 - usUnusedInputLen;
        gusTxRxDataLength = usUnusedInputLen;
    }
    else
        gusTxRxDataLength = 0;

    return *psInAppDataLen;
}

//// Function Name : _ksetlsTlsWrite
//// Parameters    : (Input) socketDesc
////                 (Input) pbOutAppData
////                 (Input) sOutAppDataLen
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0) / KSETLS_ERR_...(<0)
//// Operation     : TLS Write Application Data.
//// Description   : - 'sOutAppDataLen' should be 1 ~ 2944.
int16_t _ksetlsTlsWrite(SOCKET socketDesc, uint8_t *pbOutAppData,
                        int16_t sOutAppDataLen)
{
    int16_t sRv, sOutLen;
    uint16_t usLen, usRecordLen;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if ((socketDesc == INVALID_SOCKET) || (pbOutAppData == NULL_PTR) ||
        (sOutAppDataLen <= 0) || (sOutAppDataLen > MAX_IO_DATA_SIZE))
        return KSE_FAIL_WRONG_INPUT;

    // Set Record.
    gabTxRxData[0] = 0x06;
    gabTxRxData[1] = 0x21;
    gabTxRxData[2] = (uint8_t)((int32_t)socketDesc >> 24);
    gabTxRxData[3] = (uint8_t)((int32_t)socketDesc >> 16);
    gabTxRxData[4] = (uint8_t)((int32_t)socketDesc >> 8);
    gabTxRxData[5] = (uint8_t)((int32_t)socketDesc);
    gabTxRxData[6] = (uint8_t)(sOutAppDataLen >> 8);
    gabTxRxData[7] = (uint8_t)(sOutAppDataLen);
    _cpy(gabTxRxData + 8, pbOutAppData, sOutAppDataLen);
    sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, sOutAppDataLen + 8);
    if (sRv != KSE_SUCCESS)
        return sRv;
    sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                    ((uint16_t)gabTxRxData[1]));
    if (((sRv == KSE_SUCCESS) && (usLen < 4)) ||
        ((sRv != KSE_SUCCESS) && (usLen != 2)))
        return KSE_FAIL_UNEXPECTED_RESP_LEN;
    if (sRv != KSE_SUCCESS)
        return sRv;
    usRecordLen = ((uint16_t)gabTxRxData[2] << 8) | ((uint16_t)gabTxRxData[3]);

    // Send Record.
    if (usRecordLen > 0)
    {
        sOutLen = _tlsSend(socketDesc, gabTxRxData + 4, usRecordLen);
        if (sOutLen < 0)
            return sOutLen;
    }
    gusTxRxDataLength = 0;

    return sOutLen;
}

//// Function Name : _ksetlsTlsCloseNotify
//// Parameters    : (Input) socketDesc
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0) / KSETLS_ERR_...(<0)
//// Operation     : TLS Write Close Notify.
//// Description   : -  None.
int16_t _ksetlsTlsCloseNotify(SOCKET socketDesc)
{
    int16_t sRv, sOutLen;
    uint16_t usLen, usRecordLen;

    // Check KSE power state.
    if (gusKsePower != KSE_POWER_ON)
        return KSE_FAIL_NOT_POWERED_ON;

    // Check input.
    if (socketDesc == INVALID_SOCKET)
        return KSE_FAIL_WRONG_INPUT;

    // Close notify.
    gabTxRxData[0] = 0x06;
    gabTxRxData[1] = 0x23;
    gabTxRxData[2] = (uint8_t)((int32_t)socketDesc >> 24);
    gabTxRxData[3] = (uint8_t)((int32_t)socketDesc >> 16);
    gabTxRxData[4] = (uint8_t)((int32_t)socketDesc >> 8);
    gabTxRxData[5] = (uint8_t)((int32_t)socketDesc);
    sRv = _kseTransceive(gabTxRxData, &usLen, gabTxRxData, 6);
    if (sRv != KSE_SUCCESS)
        return sRv;
    sRv = (int16_t)(((uint16_t)gabTxRxData[0] << 8) |
                    ((uint16_t)gabTxRxData[1]));
    if (((sRv == KSE_SUCCESS) && (usLen < 4)) ||
        ((sRv != KSE_SUCCESS) && (usLen != 2)))
        return KSE_FAIL_UNEXPECTED_RESP_LEN;
    if (sRv != KSE_SUCCESS)
        return sRv;
    usRecordLen = ((uint16_t)gabTxRxData[2] << 8) | ((uint16_t)gabTxRxData[3]);

    // Send Record.
    if (usRecordLen > 0)
    {
        sOutLen = _tlsSend(socketDesc, gabTxRxData + 4, usRecordLen);
        if (sOutLen < 0)
            return sOutLen;
    }
    gusTxRxDataLength = 0;

    return sOutLen;
}
