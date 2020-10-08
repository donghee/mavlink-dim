////////////////////////////////////////////////////////////////////////////////
////                                                                        ////
////                         KSE DIM API for Ubuntu                         ////
////                                                                        ////
////          Copyright (C) 2017,2019 Keypair All rights reserved.          ////
////                                                                        ////
////////////////////////////////////////////////////////////////////////////////
//// Module Name    : kse_ubuntu.h                                          ////
//// Description    : KSE DIM API for Windows, Header File                  ////
//// Programmer     : Jung-Youp Lee                                         ////
//// OS             : Ubuntu 18.04.2 LTS                                    ////
//// Compiler       : gcc (Ubuntu 7.4.0-1ubuntu1~18.04.1) 7.4.0             ////
////                  with libusb-1.0-0(2:1.0.21-2)                         ////
//// Last Update    : 2019.11.21                                            ////
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//// Define Conditions /////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#ifndef KSE_UBUNTU_H
#define KSE_UBUNTU_H


////////////////////////////////////////////////////////////////////////////////
//// Include Header Files //////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#include <stdint.h>


////////////////////////////////////////////////////////////////////////////////
//// Declare Global Constants //////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//// Debug /////////////////////////////////////////////////////////////////////

#define SENT  0
#define RECV  1

//// Network Communication /////////////////////////////////////////////////////

#define SOCKET          int
#define INVALID_SOCKET  -1

//// KSE DIM ///////////////////////////////////////////////////////////////////

#define MAX_KCMVP_KEY_COUNT     8
#define MAX_CERT_KEY_COUNT     96
#define MAX_IO_DATA_SIZE     2944
#define MAX_INFO_FILE_SIZE   8192

//// KSE ///////////////////////////////////////////////////////////////////////

#define KSE_TRUE   1
#define KSE_FALSE  0
#define NOT_USED   0
#define NULL_PTR   ((void *)0)

//// KCMVP /////////////////////////////////////////////////////////////////////

#define KCMVP_AES128_KEY     0x40
#define KCMVP_AES192_KEY     0x41
#define KCMVP_AES256_KEY     0x42
#define KCMVP_ARIA128_KEY    0x50
#define KCMVP_ARIA192_KEY    0x51
#define KCMVP_ARIA256_KEY    0x52
#define KCMVP_HMAC_KEY       0x70
#define KCMVP_ECDSA_KEYPAIR  0x80
#define KCMVP_ECDSA_PRI_KEY  0x81
#define KCMVP_ECDSA_PUB_KEY  0x82
#define KCMVP_ECDH_KEYPAIR   0x90
#define KCMVP_ECDH_PRI_KEY   0x91
#define KCMVP_ECDH_PUB_KEY   0x92

#define ENCRYPT  0
#define DECRYPT  1

//// Cert //////////////////////////////////////////////////////////////////////

#define CERT_KEY_EMPTY          0xFF
#define CERT_KEY_INT_DEV_ECDSA  0x00
#define CERT_KEY_INT_DEV_ECDH   0x01
#define CERT_KEY_INT_DEV_ECC    0x02
#define CERT_KEY_EXT_DEV_ECDSA  0x04
#define CERT_KEY_EXT_DEV_ECDH   0x05
#define CERT_KEY_EXT_DEV_ECC    0x06
#define CERT_KEY_EXT_CA_ECDSA   0x07

#define CERT_SN           0x00
#define CERT_ISSUER_DN    0x01
#define CERT_ISSUER_CN    0x02
#define CERT_VALIDITY     0x03
#define CERT_SUBJ_DN      0x04
#define CERT_SUBJ_CN      0x05
#define CERT_SUBJ_PUBKEY  0x06
#define CERT_AUTH_KEY_ID  0x07
#define CERT_SUBJ_KEY_ID  0x08
#define CERT_KEY_USAGE    0x09
#define CERT_BC           0x0A

#define SELECT_CERT     0x01
#define SELECT_PUB_KEY  0x02
#define SELECT_PRI_KEY  0x04
#define SELECT_ALL      0x07

//// kseTLS ////////////////////////////////////////////////////////////////////

#define KSETLS_MODE_TLS   0
#define KSETLS_MODE_DTLS  1

#define KSETLS_CLIENT  0
#define KSETLS_SERVER  1

#define KSETLS_FULL_HANDSHAKE  0
#define KSETLS_ABBR_HANDSHAKE  1

#define NONE     0x00
#define SESSION  0x08  // Save Session ID.

//// KSE API Error Codes ///////////////////////////////////////////////////////

#define KSE_SUCCESS                        0
#define KSE_FAIL                           (-0x8000)  // 0x8000

#define KSE_FAIL_WRONG_INPUT               (-0x0200)  // 0xFE00
#define KSE_FAIL_NOT_SUPPORTED             (-0x0201)  // 0xFDFF
#define KSE_FAIL_NOT_POWERED_ON            (-0x0202)  // 0xFDFE
#define KSE_FAIL_ALREADY_POWERED_ON        (-0x0203)  // 0xFDFD
#define KSE_FAIL_ATR                       (-0x0204)  // 0xFDFC
#define KSE_FAIL_PPS                       (-0x0205)  // 0xFDFB
#define KSE_FAIL_TX                        (-0x0206)  // 0xFDFA
#define KSE_FAIL_RX                        (-0x0207)  // 0xFDF9
#define KSE_FAIL_CHAINING                  (-0x0208)  // 0xFDF8
#define KSE_FAIL_APDU_FORMAT               (-0x0209)  // 0xFDF7
#define KSE_FAIL_UNKNOWN_CMD               (-0x020A)  // 0xFDF6
#define KSE_FAIL_STATE                     (-0x020B)  // 0xFDF5
#define KSE_FAIL_PIN_VERIFY                (-0x020C)  // 0xFDF4
#define KSE_FAIL_CRYPTO_VERIFY             (-0x020D)  // 0xFDF3
#define KSE_FAIL_CERT_VERIFY               (-0x020E)  // 0xFDF2
#define KSE_FAIL_FLASH                     (-0x020F)  // 0xFDF1
//      KSE_FAIL_INTERNAL                    0x6xxx

#define KSE_FAIL_USB_INIT                  (-0x0300)  // 0xFD00
#define KSE_FAIL_USB_NO_DEVICES            (-0x0301)  // 0xFCFF
#define KSE_FAIL_USB_DEVICE_OPEN           (-0x0302)  // 0xFCFE
#define KSE_FAIL_USB_DETACH_KERNEL_DRIVER  (-0x0303)  // 0xFCFD
#define KSE_FAIL_USB_CLAIM_INTERFACE       (-0x0304)  // 0xFCFC
#define KSE_FAIL_USB_SEND_REPORT           (-0x0305)  // 0xFCFB
#define KSE_FAIL_USB_RECV_REPORT           (-0x0306)  // 0xFCFA
#define KSE_FAIL_NOT_FOUND                 (-0x0307)  // 0xFCF9
#define KSE_FAIL_UNEXPECTED_RESP           (-0x0308)  // 0xFCF8
#define KSE_FAIL_UNEXPECTED_RESP_LEN       (-0x0309)  // 0xFCF7
#define KSE_FAIL_RECV_BUF_OVERFLOW         (-0x030A)  // 0xFCF6

#define KSETLS_HANDSHAKE_DONE                           0x0001   // Handshake done.

#define KSETLS_SUCCESS                                  0x0000   // Success.
#define KSETLS_FAIL                                   (-0x0001)  // Fail.

#define KSETLS_ERR_NET_CONFIG                         (-0x0040)  // Failed to get ip address! Please check your network configuration.
#define KSETLS_ERR_NET_SOCKET_FAILED                  (-0x0042)  // Failed to open a socket.
#define KSETLS_ERR_NET_CONNECT_FAILED                 (-0x0044)  // The connection to the given server:port failed.
#define KSETLS_ERR_NET_BIND_FAILED                    (-0x0046)  // Binding of the socket failed.
#define KSETLS_ERR_NET_LISTEN_FAILED                  (-0x0048)  // Could not listen on the socket.
#define KSETLS_ERR_NET_ACCEPT_FAILED                  (-0x004A)  // Could not accept the incoming connection.
#define KSETLS_ERR_NET_RECV_FAILED                    (-0x004C)  // Reading information from the socket failed.
#define KSETLS_ERR_NET_SEND_FAILED                    (-0x004E)  // Sending information through the socket failed.
#define KSETLS_ERR_NET_CONN_RESET                     (-0x0050)  // Connection was reset by peer.
#define KSETLS_ERR_NET_UNKNOWN_HOST                   (-0x0052)  // Failed to get an IP address for the given hostname.
#define KSETLS_ERR_NET_BUFFER_TOO_SMALL               (-0x0043)  // Buffer is too small to hold the data.
#define KSETLS_ERR_NET_INVALID_CONTEXT                (-0x0045)  // The context is invalid, eg because it was free()ed.
#define KSETLS_ERR_NET_POLL_FAILED                    (-0x0047)  // Polling the net context failed.
#define KSETLS_ERR_NET_BAD_INPUT_DATA                 (-0x0049)  // Input invalid.

#define KSETLS_ERR_TLS_FEATURE_UNAVAILABLE            (-0x7080)  // The requested feature is not available.
#define KSETLS_ERR_TLS_BAD_INPUT_DATA                 (-0x7100)  // Bad input parameters to function.
#define KSETLS_ERR_TLS_INVALID_MAC                    (-0x7180)  // Verification of the message MAC failed.
#define KSETLS_ERR_TLS_INVALID_RECORD                 (-0x7200)  // An invalid SSL record was received.
#define KSETLS_ERR_TLS_CONN_EOF                       (-0x7280)  // The connection indicated an EOF.
#define KSETLS_ERR_TLS_UNKNOWN_CIPHER                 (-0x7300)  // An unknown cipher was received.
#define KSETLS_ERR_TLS_NO_CIPHER_CHOSEN               (-0x7380)  // The server has no ciphersuites in common with the client.
#define KSETLS_ERR_TLS_NO_RNG                         (-0x7400)  // No RNG was provided to the SSL module.
#define KSETLS_ERR_TLS_NO_CLIENT_CERTIFICATE          (-0x7480)  // No client certification received from the client, but required by the authentication mode.
#define KSETLS_ERR_TLS_CERTIFICATE_TOO_LARGE          (-0x7500)  // Our own certificate(s) is/are too large to send in an SSL message.
#define KSETLS_ERR_TLS_CERTIFICATE_REQUIRED           (-0x7580)  // The own certificate is not set, but needed by the server.
#define KSETLS_ERR_TLS_PRIVATE_KEY_REQUIRED           (-0x7600)  // The own private key or pre-shared key is not set, but needed.
#define KSETLS_ERR_TLS_CA_CHAIN_REQUIRED              (-0x7680)  // No CA Chain is set, but required to operate.
#define KSETLS_ERR_TLS_UNEXPECTED_MESSAGE             (-0x7700)  // An unexpected message was received from our peer.
#define KSETLS_ERR_TLS_FATAL_ALERT_MESSAGE            (-0x7780)  // A fatal alert message was received from our peer.
#define KSETLS_ERR_TLS_PEER_VERIFY_FAILED             (-0x7800)  // Verification of our peer failed.
#define KSETLS_ERR_TLS_PEER_CLOSE_NOTIFY              (-0x7880)  // The peer notified us that the connection is going to be closed.
#define KSETLS_ERR_TLS_BAD_HS_CLIENT_HELLO            (-0x7900)  // Processing of the ClientHello handshake message failed.
#define KSETLS_ERR_TLS_BAD_HS_SERVER_HELLO            (-0x7980)  // Processing of the ServerHello handshake message failed.
#define KSETLS_ERR_TLS_BAD_HS_CERTIFICATE             (-0x7A00)  // Processing of the Certificate handshake message failed.
#define KSETLS_ERR_TLS_BAD_HS_CERTIFICATE_REQUEST     (-0x7A80)  // Processing of the CertificateRequest handshake message failed.
#define KSETLS_ERR_TLS_BAD_HS_SERVER_KEY_EXCHANGE     (-0x7B00)  // Processing of the ServerKeyExchange handshake message failed.
#define KSETLS_ERR_TLS_BAD_HS_SERVER_HELLO_DONE       (-0x7B80)  // Processing of the ServerHelloDone handshake message failed.
#define KSETLS_ERR_TLS_BAD_HS_CLIENT_KEY_EXCHANGE     (-0x7C00)  // Processing of the ClientKeyExchange handshake message failed.
#define KSETLS_ERR_TLS_BAD_HS_CLIENT_KEY_EXCHANGE_RP  (-0x7C80)  // Processing of the ClientKeyExchange handshake message failed in DHM / ECDH Read Public.
#define KSETLS_ERR_TLS_BAD_HS_CLIENT_KEY_EXCHANGE_CS  (-0x7D00)  // Processing of the ClientKeyExchange handshake message failed in DHM / ECDH Calculate Secret.
#define KSETLS_ERR_TLS_BAD_HS_CERTIFICATE_VERIFY      (-0x7D80)  // Processing of the CertificateVerify handshake message failed.
#define KSETLS_ERR_TLS_BAD_HS_CHANGE_CIPHER_SPEC      (-0x7E00)  // Processing of the ChangeCipherSpec handshake message failed.
#define KSETLS_ERR_TLS_BAD_HS_FINISHED                (-0x7E80)  // Processing of the Finished handshake message failed.
#define KSETLS_ERR_TLS_ALLOC_FAILED                   (-0x7F00)  // Memory allocation failed.
#define KSETLS_ERR_TLS_HW_ACCEL_FAILED                (-0x7F80)  // Hardware acceleration function returned with error.
#define KSETLS_ERR_TLS_HW_ACCEL_FALLTHROUGH           (-0x6F80)  // Hardware acceleration function skipped / left alone data.
#define KSETLS_ERR_TLS_COMPRESSION_FAILED             (-0x6F00)  // Processing of the compression / decompression failed.
#define KSETLS_ERR_TLS_BAD_HS_PROTOCOL_VERSION        (-0x6E80)  // Handshake protocol not within min/max boundaries.
#define KSETLS_ERR_TLS_BAD_HS_NEW_SESSION_TICKET      (-0x6E00)  // Processing of the NewSessionTicket handshake message failed.
#define KSETLS_ERR_TLS_SESSION_TICKET_EXPIRED         (-0x6D80)  // Session ticket has expired.
#define KSETLS_ERR_TLS_PK_TYPE_MISMATCH               (-0x6D00)  // Public key type mismatch (eg, asked for RSA key exchange and presented EC key).
#define KSETLS_ERR_TLS_UNKNOWN_IDENTITY               (-0x6C80)  // Unknown identity received (eg, PSK identity).
#define KSETLS_ERR_TLS_INTERNAL_ERROR                 (-0x6C00)  // Internal error (eg, unexpected failure in lower-level module).
#define KSETLS_ERR_TLS_COUNTER_WRAPPING               (-0x6B80)  // A counter would wrap (eg, too many messages exchanged).
#define KSETLS_ERR_TLS_WAITING_SERVER_HELLO_RENEGO    (-0x6B00)  // Unexpected message at ServerHello in renegotiation.
#define KSETLS_ERR_TLS_HELLO_VERIFY_REQUIRED          (-0x6A80)  // DTLS client must retry for hello verification.
#define KSETLS_ERR_TLS_BUFFER_TOO_SMALL               (-0x6A00)  // A buffer is too small to receive or write a message.
#define KSETLS_ERR_TLS_NO_USABLE_CIPHERSUITE          (-0x6980)  // None of the common ciphersuites is usable (eg, no suitable certificate, see debug messages).
#define KSETLS_ERR_TLS_WANT_READ                      (-0x6900)  // No data of requested type currently available on underlying transport.
#define KSETLS_ERR_TLS_WANT_WRITE                     (-0x6880)  // Connection requires a write call.
#define KSETLS_ERR_TLS_TIMEOUT                        (-0x6800)  // The operation timed out.
#define KSETLS_ERR_TLS_CLIENT_RECONNECT               (-0x6780)  // The client initiated a reconnect from the same port.
#define KSETLS_ERR_TLS_UNEXPECTED_RECORD              (-0x6700)  // Record header looks valid but is not expected.
#define KSETLS_ERR_TLS_NON_FATAL                      (-0x6680)  // The alert message received indicates a non-fatal error.
#define KSETLS_ERR_TLS_INVALID_VERIFY_HASH            (-0x6600)  // Couldn't set the hash for verifying CertificateVerify.
#define KSETLS_ERR_TLS_CONTINUE_PROCESSING            (-0x6580)  // Internal-only message signaling that further message-processing should be done.
#define KSETLS_ERR_TLS_ASYNC_IN_PROGRESS              (-0x6500)  // The asynchronous operation is not completed yet.
#define KSETLS_ERR_TLS_EARLY_MESSAGE                  (-0x6480)  // Internal-only message signaling that a message arrived early.
#define KSETLS_ERR_TLS_CRYPTO_IN_PROGRESS             (-0x7000)  // A cryptographic operation failure in progress.


////////////////////////////////////////////////////////////////////////////////
//// Define Data Types /////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//// Declare Global Variables //////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//// Declare Functions /////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

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
                    uint16_t *pusInfoFileSize);

//// Function Name : _ksePowerOff
//// Parameters    : void
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : Power off KSE.
//// Description   : -  None.
int16_t _ksePowerOff(void);

//// KSE ///////////////////////////////////////////////////////////////////////

//// Function Name : _kseReadInfoFile
//// Parameters    : (Output) pbData
////                 (Input)  usOffset
////                 (Input)  usSize
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : Read data from the KSE Info File.
//// Description   : - 'usSize' should be max 2944.
////                 -  Info File size is 8192 bytes.
int16_t _kseReadInfoFile(uint8_t *pbData, uint16_t usOffset, uint16_t usSize);

//// Function Name : _kseWriteInfoFile
//// Parameters    : (Input) pbData
////                 (Input) usSize
////                 (Input) usOffset
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : Write data to the KSE Info File.
//// Description   : - 'usSize' should be max 2944.
////                 -  Info File size is 8192 bytes.
int16_t _kseWriteInfoFile(uint8_t *pbData, uint16_t usSize, uint16_t usOffset);

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
                          uint16_t usHmacKeySize);

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
                     uint16_t usKeySize);

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
                     uint16_t usKeyIndex);

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
int16_t _kcmvpEraseKey(uint8_t bKeyType, uint16_t usKeyIndex);

//// Function Name : _kcmvpDrbg
//// Parameters    : (Output) pbRandom
////                 (Input)  usRandomSize
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : < DRBG > Generate random number.
//// Description   : - 'usRandomSize' should be 1 ~ 256.
int16_t _kcmvpDrbg(uint8_t *pbRandom, uint16_t usRandomSize);

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
                     uint8_t bKeyType, uint16_t usKeyIndex, uint8_t bEnDe);

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
                     uint8_t bEnDe);

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
                     uint8_t bKeyType, uint16_t usKeyIndex, uint8_t *pbCtr);

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
                     uint8_t *pbTag, uint16_t usTagSize, uint8_t bEnDe);

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
                      uint8_t bKeyType, uint16_t usKeyIndex, uint8_t bEnDe);

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
                      uint8_t bEnDe);

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
                      uint8_t bKeyType, uint16_t usKeyIndex, uint8_t *pbCtr);

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
                      uint8_t *pbTag, uint16_t usTagSize, uint8_t bEnDe);

//// Function Name : _kcmvpSha256
//// Parameters    : (Output) pbHash
////                 (Input)  pbMessage
////                 (Input)  usMessageSize
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : < SHA2-256 >.
//// Description   : - 'pbHash' should be 32-byte array.
int16_t _kcmvpSha256(uint8_t *pbHash, uint8_t *pbMessage,
                     uint16_t usMessageSize);

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
                         uint16_t usMessageSize, uint16_t usKeyIndex);

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
                               uint8_t *pbHmac, uint16_t usKeyIndex);

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
                        uint16_t usMessageSize, uint16_t usKeyIndex);

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
                          uint8_t *pbR, uint8_t *pbS, uint16_t usKeyIndex);

//// Function Name : _kcmvpEcdhExchKey
//// Parameters    : (Output) pbSesKey
////                 (Input)  pbPubKeytoken
////                 (Input)  usKeyIndex
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : < ECDH-P256 > Exchange Key.
//// Description   : - 'pbSesKey' and 'pbPubKeytoken' should be 64-byte array.
////                 - 'usKeyIndex' should be 0 ~ 7.
int16_t _kcmvpEcdhExchKey(uint8_t *pbSesKey, uint8_t *pbPubKeytoken,
                          uint16_t usKeyIndex);

//// Function Name : _kcmvpGetClibState
//// Parameters    : (Output) pbClibState
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : Get CLIB state in KSE.
//// Description   : - 'pbClibState' should be 20-byte array.
int16_t _kcmvpGetClibState(uint8_t *pbClibState);

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
int16_t _certGenerateKeypair(uint8_t bKeyType, uint16_t usCertIndex);

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
                       uint16_t usCertIndex);

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
                       uint16_t usCertIndex);

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
                       uint16_t usCertIndex);

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
int16_t _certGetPriKeyState(uint8_t *pbKeyState, uint16_t usCertIndex);

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
                       uint16_t usCertIndex);

//// Function Name : _certGetPubKeyCsr
//// Parameters    : (Output) pbCsr
////                 (Input)  usCertIndex
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : Get CSR(Certificate Signing Request) of public key in the
////                 KSE nonvolatile memory.
//// Description   : - 'pbCsr' should be 500-byte array.
////                 - 'usCertIndex' should be 0 ~ 95.
int16_t _certGetPubKeyCsr(uint8_t *pbCsr, uint16_t usCertIndex);

//// Function Name : _certPutCert
//// Parameters    : (Input) pbCert
////                 (Input) usCertIndex
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : Put certificate in the KSE nonvolatile memory.
//// Description   : - 'pbCert' should be 500-byte array.
////                 - 'usCertIndex' should be 0 ~ 95.
int16_t _certPutCert(uint8_t *pbCert, uint16_t usCertIndex);

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
                           uint16_t usCaCertIndex);

//// Function Name : _certGetCert
//// Parameters    : (Output) pbCert
////                 (Input)  usCertIndex
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0)
//// Operation     : Get certificate in the KSE nonvolatile memory.
//// Description   : - 'pbCert' should be 500-byte array.
////                 - 'usCertIndex' should be 0 ~ 95.
int16_t _certGetCert(uint8_t *pbCert, uint16_t usCertIndex);

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
                          uint8_t bFieldIndex, uint16_t usCertIndex);

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
int16_t _certEraseCertKey(uint8_t bOpt, uint16_t usCertIndex);

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
                    uint16_t usRfu, uint8_t bSaveOption);

//// Function Name : _ksetlsClose
//// Parameters    : (Input) socketDesc
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0) / KSETLS_ERR_...(<0)
//// Operation     : Close kseTLS channel.
//// Description   : -  None.
int16_t _ksetlsClose(SOCKET socketDesc);

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
int16_t _ksetlsTlsClientHandshake(SOCKET socketDesc, uint8_t bType);

//// Function Name : _ksetlsTlsServerHandshake
//// Parameters    : (Input) socketDesc
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0) / KSETLS_ERR_...(<0)
//// Operation     : TLS Server Handshake.
//// Description   : -  None.
int16_t _ksetlsTlsServerHandshake(SOCKET socketDesc);

//// Function Name : _ksetlsTlsRead
//// Parameters    : (Output) pbInAppData
////                 (Output) psInAppDataLen
////                 (Input)  socketDesc
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0) / KSETLS_ERR_...(<0)
//// Operation     : TLS Read Application Data.
//// Description   : - 'pbInAppData' should be minimum 2944-byte array.
int16_t _ksetlsTlsRead(uint8_t *pbInAppData, int16_t *psInAppDataLen,
                       SOCKET socketDesc);

//// Function Name : _ksetlsTlsWrite
//// Parameters    : (Input) socketDesc
////                 (Input) pbOutAppData
////                 (Input) sOutAppDataLen
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0) / KSETLS_ERR_...(<0)
//// Operation     : TLS Write Application Data.
//// Description   : - 'sOutAppDataLen' should be 1 ~ 2944.
int16_t _ksetlsTlsWrite(SOCKET socketDesc, uint8_t *pbOutAppData,
                        int16_t sOutAppDataLen);

//// Function Name : _ksetlsTlsCloseNotify
//// Parameters    : (Input) socketDesc
//// Return Value  : KSE_SUCCESS(0) / KSE_FAIL...(<0) / KSETLS_ERR_...(<0)
//// Operation     : TLS Write Close Notify.
//// Description   : -  None.
int16_t _ksetlsTlsCloseNotify(SOCKET socketDesc);


////////////////////////////////////////////////////////////////////////////////
//// Define Functions //////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


#endif // KSE_UBUNTU_H
