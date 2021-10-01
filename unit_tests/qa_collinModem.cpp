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

// using namespace std;
// using namespace ex2::sdr;

#include "gtest/gtest.h"

#define QA_COLLINMODEM_DEBUG 0 // set to 1 for debugging output

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
    float             SNRdB_min         = 0.0f;                // signal-to-noise ratio (minimum)
    float             SNRdB_max         = 10.0f;                // signal-to-noise ratio (maximum)
    unsigned int      num_snr           = 11;                   // number of SNR steps
    unsigned int      num_packet_trials = 100;                  // number of trials

    unsigned int i;

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

    printf("  %8s %8s %8s %12s %8s %8s %6s\n",
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
#if QA_COLLINMODEM_DEBUG
            printf("    Tx Packet: %x %x %x %x\n", payload_tx[0], payload_tx[1], payload_tx[2], payload_tx[3]);
            printf("    Rx Packet: %x %x %x %x\n", payload_rx[0], payload_rx[1], payload_rx[2], payload_rx[3]);
#endif
        }
        BER[s] = (float)num_bit_errors[s]    / (float)num_bit_trials;
        PER[s] = (float)num_packet_errors[s] / (float)num_packet_trials;
        printf("  %8.2f %8u %8u %12.4e %8u %8u %6.2f%%\n",
                SNRdB,
                num_bit_errors[s], num_bit_trials, BER[s],
                num_packet_errors[s], num_packet_trials, PER[s]*100.0f);
    }

    // Clean up!
    csp_buffer_free(packet);
    qpacketmodem_destroy(q);

#if QA_COLLINMODEM_DEBUG
    printf("Test done!\n");
#endif

}

