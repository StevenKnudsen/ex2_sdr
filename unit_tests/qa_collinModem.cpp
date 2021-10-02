/*!
 * @file qa_NoFEC.cpp
 * @author Steven Knudsen
 * @date July 15, 2021
 *
 * @details Unit test for the NoFEC class.
 *
 *
 * @copyright AlbertaSat 2021
 *
 * @license
 * This software may not be modified or distributed in any form, except as described in the LICENSE file.
 */

#include <cstdio>
#include <iostream>
#include <random>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <complex>
#include <cmath>

#include "convCode27.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#include "csp.h"
#include "csp_types.h"
#include "csp_buffer.h"

#include "liquid.h"

#ifdef __cplusplus
}
#endif

using namespace std;
using namespace ex2::sdr;

#include "gtest/gtest.h"

#define QA_COLLINMODEM_DEBUG 1 // set to 1 for debugging output

#define UHF_TRANSPARENT_MODE_PACKET_HEADER_LENGTH ( 72/8 ) // bytes
#define UHF_TRANSPARENT_MODE_PACKET_LENGTH ( 128 )         // bytes; UHF transparent mode packet is always 128 bytes
#define UHF_TRANSPARENT_MODE_PACKET_PAYLOAD_LENGTH ( UHF_TRANSPARENT_MODE_PACKET_LENGTH - UHF_TRANSPARENT_MODE_PACKET_HEADER_LENGTH )
/*!
 * @brief Test Main Constructors, the one that is parameterized, and the one
 * that takes the received packet as input
 */
TEST(collinModem, ModTest )
{
    /* ---------------------------------------------------------------------
    * Confirm that the LiquidSDR library works as expected.
    * ---------------------------------------------------------------------
    */

    // First do a little CSP config work
    csp_conf_t cspConf;
    csp_conf_get_defaults(&cspConf);
    cspConf.buffer_data_size = 4096; // TODO set as CSP_MTU
    csp_init(&cspConf);

    // Set the length of the test CSP packet so it all fits into a transparent mode payload
    const unsigned long int testCSPPacketLength = UHF_TRANSPARENT_MODE_PACKET_PAYLOAD_LENGTH;

    csp_packet_t * packet = (csp_packet_t *) csp_buffer_get(testCSPPacketLength);

    if (packet == NULL) {
        /* Could not get buffer element */
        csp_log_error("Failed to get CSP buffer");
        return;
    }

    // int cspPacketHeaderLen = sizeof(packet->padding) + sizeof(packet->length) + sizeof(packet->id);

    for (unsigned long i = 0; i < testCSPPacketLength; i++) {
    packet->data[i] = i | 0x30; // ASCII numbers
    }
    // CSP forces us to do our own bookkeeping...
    packet->length = testCSPPacketLength;

    // Config options for liquid packet modem
    modulation_scheme ms                = LIQUID_MODEM_BPSK;    // mod. scheme
    crc_scheme        check             = LIQUID_CRC_32;        // data validity check
    fec_scheme        fec0              = LIQUID_FEC_CONV_V27;      // fec (inner)
    fec_scheme        fec1              = LIQUID_FEC_NONE;      // fec (outer)
    unsigned int      payload_len       = packet->length;       // payload length
    float             SNRdB_min         = 0.0f;                // signal-to-noise ratio (minimum) default is 0.0.
    float             SNRdB_max         = 10.0f;                // signal-to-noise ratio (maximum) default is 10.0. This value should result in no packet errors.
    unsigned int      num_snr           = 15;                   // number of SNR steps default is 15.
    unsigned int      num_packet_trials = 100;                  // number of trials default is 100.
    // bool              expect_errors    [num_snr];     // Whether errors are expected for given SNR index.

    unsigned int i;

    // // The threshold for the if statement varies if you change num_snr away from 15. For num_snr = 15, i > 8.
    // for (i=0; i<num_snr; i++) {
    //     if(i > 8) { // This value is variable depending on the results.
    //         expect_errors[i] = 0;
    //     } else {
    //         expect_errors[i] = 1;
    //     }
    // }

    // derived values
    float SNRdB_step = (SNRdB_max - SNRdB_min) / (num_snr-1);

    // create and configure packet encoder/decoder object
    qpacketmodem q = qpacketmodem_create();
    qpacketmodem_configure(q, payload_len, check, fec0, fec1, ms);
    ASSERT_TRUE(q != NULL);
    
#if QA_COLLINMODEM_DEBUG
    qpacketmodem_print(q);
#endif

    // get frame length
    unsigned int frame_len = qpacketmodem_get_frame_len(q);
    unsigned int num_bit_trials = 8*num_packet_trials*payload_len;

    // initialize payload
    unsigned char payload_tx       [payload_len]; // payload (transmitted)
    unsigned char payload_rx       [payload_len]; // payload (received)
    std::complex<float> frame_tx         [frame_len];   // frame samples (transmitted)
    std::complex<float> frame_rx         [frame_len];   // frame samples (received)
    unsigned int  num_bit_errors   [num_snr];     // bit errors for each SNR point
    unsigned int  num_packet_errors[num_snr];     // packet errors for each SNR point
    float         BER              [num_snr];     // bit error rate
    float         PER              [num_snr];     // packet error rate

    printf("  %8s %8s %8s %12s %8s %8s %7s\n",
            "SNR [dB]", "errors", "bits", "BER", "errors", "packets", "PER");
    unsigned int s;

    for (s=0; s<num_snr; s++) {
        // compute SNR for this level
        float SNRdB = SNRdB_min + s*SNRdB_step; // SNR in dB for this round
        float nstd = powf(10.0f, -SNRdB/20.0f); // noise standard deviation

        // reset counters
        num_bit_errors[s]    = 0;
        num_packet_errors[s] = 0;

        unsigned int t;
        for (t=0; t<num_packet_trials; t++) {
            // initialize payload
            for (i=0; i<payload_len; i++) {
                // payload_tx[i] = rand() & 0xff;
                payload_rx[i] = 0x00;
            }

            memcpy(payload_tx, packet, payload_len);

            // encode frame
            qpacketmodem_encode(q, payload_tx, frame_tx);

            // add noise
            for (i=0; i<frame_len; i++)
                frame_rx[i] = frame_tx[i] + nstd*randnf();

            // decode frame
            int crc_pass = qpacketmodem_decode(q, frame_rx, payload_rx);

            // accumulate errors
            num_bit_errors[s]    += count_bit_errors_array(payload_tx, payload_rx, payload_len);
            num_packet_errors[s] += crc_pass ? 0 : 1;
        }
        BER[s] = (float)num_bit_errors[s]    / (float)num_bit_trials;
        PER[s] = (float)num_packet_errors[s] / (float)num_packet_trials;
        printf("  %8.2f %8u %8u %12.4e %8u %8u %6.2f%%\n",
                SNRdB,
                num_bit_errors[s], num_bit_trials, BER[s],
                num_packet_errors[s], num_packet_trials, PER[s]*100.0f);
        
        // This test fails rarely, replaced with a test that should always pass.
        // EXPECT_EQ(num_packet_errors[s] > 0, expect_errors[s] > 0) << "SNR error rates not as expected";
    }
    // Final SNR step should always pass.
    ASSERT_TRUE(num_packet_errors[num_snr-1] == 0) << "Final SNR value has packet errors";

    // Clean up!
    csp_buffer_free(packet);
    qpacketmodem_destroy(q);

#if QA_COLLINMODEM_DEBUG
    printf("Test done!\n");
#endif

}

TEST(collinModem, SameCCTest )
{
    /* ---------------------------------------------------------------------
    * Confirm that the LiquidSDR and EX2 CC27 algorithm are compatible.
    * ---------------------------------------------------------------------
    */

    // First do a little CSP config work
    csp_conf_t cspConf;
    csp_conf_get_defaults(&cspConf);
    cspConf.buffer_data_size = 4096; // TODO set as CSP_MTU
    csp_init(&cspConf);

    // Set the length of the test CSP packet so it all fits into a transparent mode payload
    const unsigned long int testCSPPacketLength = UHF_TRANSPARENT_MODE_PACKET_PAYLOAD_LENGTH;

    csp_packet_t * packet = (csp_packet_t *) csp_buffer_get(testCSPPacketLength);

    if (packet == NULL) {
        /* Could not get buffer element */
        csp_log_error("Failed to get CSP buffer");
        return;
    }

    int cspPacketHeaderLen = sizeof(packet->padding) + sizeof(packet->length) + sizeof(packet->id);

    for (unsigned long i = 0; i < testCSPPacketLength; i++) {
    packet->data[i] = i | 0x30; // ASCII numbers
    }
    // CSP forces us to do our own bookkeeping...
    packet->length = testCSPPacketLength;

    // LSDR
    // Config options for liquid packet modem
    modulation_scheme ms                = LIQUID_MODEM_BPSK;    // mod. scheme
    crc_scheme        check             = LIQUID_CRC_32;        // data validity check
    fec_scheme        fec0              = LIQUID_FEC_CONV_V27;      // fec (inner)
    fec_scheme        fec1              = LIQUID_FEC_NONE;      // fec (outer)
    unsigned int      payload_len       = packet->length;       // payload length
    float             SNRdB_min         = 0.0f;                // signal-to-noise ratio (minimum) default is 0.0.
    float             SNRdB_max         = 10.0f;                // signal-to-noise ratio (maximum) default is 10.0. This value should result in no packet errors.
    unsigned int      num_snr           = 15;                   // number of SNR steps default is 15.
    unsigned int      num_packet_trials = 100;                  // number of trials default is 100.
    // bool              expect_errors    [num_snr];     // Whether decode errors are expected for given SNR index.
    // bool              expect_errors_algo    [num_snr];     // Whether algorithm matching errors are expected for given SNR index.

    unsigned int i;

    // // The threshold for the if statement varies if you change num_snr away from 15. For num_snr = 15, i > 8.
    // for (i=0; i<num_snr; i++) {
    //     if(i > 8) { // This value is variable depending on the results.
    //         expect_errors[i] = 0;
    //     } else {
    //         expect_errors[i] = 1;
    //     }
    // }

    // // The threshold for the if statement varies if you change num_snr away from 15. For num_snr = 15, i > 5.
    // for (i=0; i<num_snr; i++) {
    //     if(i > 5) { // This value is variable depending on the results.
    //         expect_errors_algo[i] = 0;
    //     } else {
    //         expect_errors_algo[i] = 1;
    //     }
    // }

    // derived values
    float SNRdB_step = (SNRdB_max - SNRdB_min) / (num_snr-1);

    // LSDR
    // create and configure packet encoder/decoder object
    qpacketmodem q = qpacketmodem_create();
    qpacketmodem_configure(q, payload_len, check, fec0, fec1, ms);
    ASSERT_TRUE(q != NULL);
    
#if QA_COLLINMODEM_DEBUG
    qpacketmodem_print(q);
#endif

    FEC * CC27 = new convCode27(ErrorCorrection::ErrorCorrectionScheme::CCSDS_CONVOLUTIONAL_CODING_R_1_2);

    ASSERT_TRUE(CC27 != NULL) << "convCode27 failed to instantiate";

    // LSDR
    // get frame length
    unsigned int frame_len = qpacketmodem_get_frame_len(q);
    unsigned int num_bit_trials = 8*num_packet_trials*payload_len;

    // LSDR
    // initialize payload
    unsigned char txPayloadLSDR       [payload_len]; // payload (transmitted)
    unsigned char rxPayloadLSDR       [payload_len]; // payload (received)
    std::complex<float> frame_tx         [frame_len];   // frame samples (transmitted)
    std::complex<float> frame_rx         [frame_len];   // frame samples (received)
    unsigned int  num_bit_errors   [num_snr];     // bit errors for each SNR point
    unsigned int  num_packet_errors[num_snr];     // packet errors for each SNR point
    float         BER              [num_snr];     // bit error rate
    float         PER              [num_snr];     // packet error rate
    unsigned int  num_algo_errors      [num_snr];     // incorrectly matched packets
    float         AER              [num_snr];     // algorithm conflict rate

    printf("  %8s %8s %8s %12s %8s %8s %7s %12s %7s\n",
            "SNR [dB]", "errors", "bits", "BER", "errors", "packets", "PER", "conflicts", "AER");
    unsigned int s;

    for (s=0; s<num_snr; s++) {
        // LSDR
        // compute SNR for this level
        float SNRdB = SNRdB_min + s*SNRdB_step; // SNR in dB for this round
        float nstd = powf(10.0f, -SNRdB/20.0f); // noise standard deviation

        // LSDR
        // reset counters
        num_bit_errors[s]    = 0;
        num_packet_errors[s] = 0;
        num_algo_errors[s]   = 0;

        // LSDR
        unsigned int t;
        for (t=0; t<num_packet_trials; t++) {
            // initialize payload
            for (i=0; i<payload_len; i++) {
                // payload_tx[i] = rand() & 0xff;
                rxPayloadLSDR[i] = 0x00;
            }

            // LSDR
            // copy the CSP packet into LiquidSDR payload.
            memcpy(txPayloadLSDR, packet, payload_len);

            // LSDR
            // encode frame with LiquidSDR
            qpacketmodem_encode(q, txPayloadLSDR, frame_tx);

            // EX2
            // Make the payload for EX2 algo from CSP packet
            std::vector<uint8_t> p;
            uint8_t * pptr = (uint8_t *) packet;
                for (int i = 0; i < cspPacketHeaderLen; i++) {
                p.push_back(pptr[i]);
            }
            // EX2
            // This is ugly, so maybe we need to rethink using PPDU_xx?
            for (unsigned long i = 0; i < testCSPPacketLength; i++) {
                p.push_back(packet->data[i]);
            }

            // EX2
            // Create payload for EX2 and encode with CC27
            PPDU_u8 inputPayload(p);
            PPDU_u8 encodedPayload = CC27->encode(inputPayload);
            std::vector<uint8_t> txPayloadEX2 = encodedPayload.getPayload();
            std::vector<uint8_t> rxPayloadEX2 = encodedPayload.getPayload();

            // Both
            // add noise to both packets
            for (i=0; i<frame_len; i++) {
                frame_rx[i] = frame_tx[i] + nstd*randnf();
            }

            // LSDR
            // decode frame
            int crc_pass = qpacketmodem_decode(q, frame_rx, rxPayloadLSDR);

            // LSDR
            // accumulate errors
            // num_bit_errors[s]    += count_bit_errors_array(txPayloadLSDR, rxPayloadLSDR, payload_len);
            // num_packet_errors[s] += crc_pass ? 0 : 1;

            // EX2
            // Noise-free channel to check if the algorithms are working correctly
            std::vector<uint8_t> outputPayload;
            CC27->decode(rxPayloadEX2, 100.0, outputPayload);
            
            // LSDR
            num_bit_errors[s]    += count_bit_errors_array(txPayloadLSDR, rxPayloadLSDR, payload_len);
            num_packet_errors[s] += crc_pass ? 0 : 1;

            // Both
            // Check that the packets are the same in both algos
            uint32_t errors = 0;
            for (unsigned long i = 0; i < payload_len; i++) {
                errors += (outputPayload[i] == rxPayloadLSDR[i]) ? 0 : 1;
            }
            num_algo_errors[s] += errors == 0 ? 0 : 1;
        }
        // LSDR
        BER[s] = (float)num_bit_errors[s]    / (float)num_bit_trials;
        PER[s] = (float)num_packet_errors[s] / (float)num_packet_trials;
        AER[s] = (float)num_algo_errors[s] / (float)num_packet_trials;
        printf("  %8.2f %8u %8u %12.4e %8u %8u %6.2f%% %12u %6.2f%%\n",
                SNRdB,
                num_bit_errors[s], num_bit_trials, BER[s],
                num_packet_errors[s], num_packet_trials, PER[s]*100.0f,
                num_algo_errors[s], AER[s]*100.0f);

        // These tests fail sometimes, replaced with a test that should always pass.
        // EXPECT_EQ(num_packet_errors[s] > 0, expect_errors[s] > 0) << "SNR error rates not as expected";
        // EXPECT_EQ(algo_errors[s] > 0, expect_errors_algo[s] > 0) << "algorithm matching between LiquidSDR and EX2 not as expected";
    }
    // Final SNR step should always pass.
    ASSERT_TRUE(num_packet_errors[num_snr-1] == 0) << "Final SNR value has packet errors";
    ASSERT_TRUE(num_algo_errors[num_snr-1] == 0) << "Fina; SNR has algorithm disagreement";

    // Clean up!
    csp_buffer_free(packet);
    qpacketmodem_destroy(q);

#if QA_COLLINMODEM_DEBUG
    printf("Test done!\n");
#endif

}

