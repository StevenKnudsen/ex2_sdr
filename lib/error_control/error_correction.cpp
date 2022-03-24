/*!
 * @file error_correction.cpp
 * @author Steven Knudsen
 * @date April 30, 2021
 *
 * @details
 *
 * @copyright AlbertaSat 2021
 *
 * @license
 * This software may not be modified or distributed in any form, except as described in the LICENSE file.
 */

#include "error_correction.hpp"

#include <exception>
#include <string>

#define ERROR_CORRECTION_DEBUG 0 // set to 1 to enable extra debugging info

namespace ex2 {
  namespace sdr {

    class ECException: public std::exception {
    private:
      std::string message_;
    public:
      explicit ECException(const std::string& message);
      virtual const char* what() const throw() {
        return message_.c_str();
      }
    };

    ECException::ECException(const std::string& message) : message_(message) {
    }

    ErrorCorrection::ErrorCorrection(ErrorCorrectionScheme ecScheme,
      uint32_t continuousMaxCodewordLen) :
                m_errorCorrectionScheme(ecScheme),
                m_continuousMaxCodewordLen(continuousMaxCodewordLen)
    {
      // TODO this is hard coded. Find a more elegant way to update the code for
      // new implemented FEC schemes
      if (!isValid(ecScheme)) {
#if ERROR_CORRECTION_DEBUG
        printf("scheme %d\n", (uint16_t) scheme);
#endif
        throw ECException("Invalid FEC Scheme");
      }
      m_codingRate = m_getCodingRate(ecScheme);
      if (m_codingRate == ErrorCorrection::CodingRate::RATE_BAD) {
        throw ECException("Invalid FEC Scheme; no rate known");
      }

      m_rate = m_codingRateToFractionalRate();
      m_codewordLen = m_codewordLength();
      m_messageLen = m_messageLength();
    }

    ErrorCorrection::~ErrorCorrection() {
    }

    uint32_t
    ErrorCorrection::numCodewordFragments(uint32_t payloadLength) {
      uint32_t numFrags = 0;
      uint32_t codewordBytes = m_codewordLen / 8;

      // There is no codeword length for NO_FEC, so the answer is always 1 fragment
      if (m_errorCorrectionScheme == ErrorCorrectionScheme::NO_FEC) {
        numFrags = 1;
      }
      else {
        if (codewordBytes > 0) {
          numFrags = codewordBytes / payloadLength;
          if (codewordBytes % payloadLength != 0) numFrags++;
        }
      }

      return numFrags;
    }

    ErrorCorrection::CodingRate
    ErrorCorrection::m_bits2rate(uint16_t bits) const
    {
      return static_cast<CodingRate>(bits);
    }

    const std::string
    ErrorCorrection::ErrorCorrectionName(ErrorCorrection::ErrorCorrectionScheme scheme)
    {
      // @TODO This is so ugly
      switch(scheme) {
        case ErrorCorrectionScheme::CCSDS_CONVOLUTIONAL_CODING_R_1_2:
          return std::string("CCSDS Convolutional Coding rate 1/2");
          break;
        case ErrorCorrectionScheme::CCSDS_CONVOLUTIONAL_CODING_R_2_3:
          return std::string("CCSDS Convolutional Coding rate 2/3");
          break;
        case ErrorCorrectionScheme::CCSDS_CONVOLUTIONAL_CODING_R_3_4:
          return std::string("CCSDS Convolutional Coding rate 3/4");
          break;
        case ErrorCorrectionScheme::CCSDS_CONVOLUTIONAL_CODING_R_5_6:
          return std::string("CCSDS Convolutional Coding rate 5/6");
          break;
        case ErrorCorrectionScheme::CCSDS_CONVOLUTIONAL_CODING_R_7_8:
          return std::string("CCSDS Convolutional Coding rate 7/8");
          break;
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_239_INTERLEAVING_1:
          return std::string("CCSDS Reed-Solomon (255,239) interleaving level 1");
          break;
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_239_INTERLEAVING_2:
          return std::string("CCSDS Reed-Solomon (255,239) interleaving level 2");
          break;
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_239_INTERLEAVING_3:
          return std::string("CCSDS Reed-Solomon (255,239) interleaving level 3");
          break;
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_239_INTERLEAVING_4:
          return std::string("CCSDS Reed-Solomon (255,239) interleaving level 4");
          break;
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_239_INTERLEAVING_5:
          return std::string("CCSDS Reed-Solomon (255,239) interleaving level 5");
          break;
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_239_INTERLEAVING_8:
          return std::string("CCSDS Reed-Solomon (255,239) interleaving level 8");
          break;
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_223_INTERLEAVING_1:
          return std::string("CCSDS Reed-Solomon (255,223) interleaving level 1");
          break;
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_223_INTERLEAVING_2:
          return std::string("CCSDS Reed-Solomon (255,223) interleaving level 2");
          break;
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_223_INTERLEAVING_3:
          return std::string("CCSDS Reed-Solomon (255,223) interleaving level 3");
          break;
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_223_INTERLEAVING_4:
          return std::string("CCSDS Reed-Solomon (255,223) interleaving level 4");
          break;
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_223_INTERLEAVING_5:
          return std::string("CCSDS Reed-Solomon (255,223) interleaving level 5");
          break;
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_223_INTERLEAVING_8:
          return std::string("CCSDS Reed-Solomon (255,223) interleaving level 8");
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_1784_R_1_2:
          return std::string("CCSDS Turbo rate n=1784 1/2");
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_1784_R_1_3:
          return std::string("CCSDS Turbo rate n=1784 1/3");
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_1784_R_1_4:
          return std::string("CCSDS Turbo rate n=1784 1/4");
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_1784_R_1_6:
          return std::string("CCSDS Turbo rate n=1784 1/6");
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_3568_R_1_2:
          return std::string("CCSDS Turbo rate n=3568 1/2");
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_3568_R_1_3:
          return std::string("CCSDS Turbo rate n=3568 1/3");
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_3568_R_1_4:
          return std::string("CCSDS Turbo rate n=3568 1/4");
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_3568_R_1_6:
          return std::string("CCSDS Turbo rate n=3568 1/6");
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_7136_R_1_2:
          return std::string("CCSDS Turbo rate n=7136 1/2");
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_7136_R_1_3:
          return std::string("CCSDS Turbo rate n=7136 1/3");
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_7136_R_1_4:
          return std::string("CCSDS Turbo rate n=7136 1/4");
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_7136_R_1_6:
          return std::string("CCSDS Turbo rate n=7136 1/6");
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_8920_R_1_2:
          return std::string("CCSDS Turbo rate n=8920 1/2");
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_8920_R_1_3:
          return std::string("CCSDS Turbo rate n=8920 1/3");
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_8920_R_1_4:
          return std::string("CCSDS Turbo rate n=8920 1/4");
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_8920_R_1_6:
          return std::string("CCSDS Turbo rate n=8920 1/6");
          break;
        case ErrorCorrectionScheme::CCSDS_LDPC_ORANGE_BOOK_1280:
          return std::string("CCSDS Orange Book 131.1-O-2 LDPC n=1288");
          break;
        case ErrorCorrectionScheme::CCSDS_LDPC_ORANGE_BOOK_1536:
          return std::string("CCSDS Orange Book 131.1-O-2 LDPC n=1536");
          break;
        case ErrorCorrectionScheme::CCSDS_LDPC_ORANGE_BOOK_2048:
          return std::string("CCSDS Orange Book 131.1-O-2 LDPC n=2048");
          break;
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_648_R_1_2:
          return std::string("IEEE 802.11n QC-LDPC n=648 rate 1/2");
          break;
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_648_R_2_3:
          return std::string("IEEE 802.11n QC-LDPC n=648 rate 2/3");
          break;
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_648_R_3_4:
          return std::string("IEEE 802.11n QC-LDPC n=648 rate 3/4");
          break;
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_648_R_5_6:
          return std::string("IEEE 802.11n QC-LDPC n=648 rate 5/6");
          break;
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1296_R_1_2:
          return std::string("IEEE 802.11n QC-LDPC n=1296 rate 1/2");
          break;
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1296_R_2_3:
          return std::string("IEEE 802.11n QC-LDPC n=1296 rate 2/3");
          break;
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1296_R_3_4:
          return std::string("IEEE 802.11n QC-LDPC n=1296 rate 3/4");
          break;
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1296_R_5_6:
          return std::string("IEEE 802.11n QC-LDPC n=1296 rate 5/6");
          break;
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1944_R_1_2:
          return std::string("IEEE 802.11n QC-LDPC n=1944 rate 1/2");
          break;
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1944_R_2_3:
          return std::string("IEEE 802.11n QC-LDPC n=1944 rate 2/3");
          break;
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1944_R_3_4:
          return std::string("IEEE 802.11n QC-LDPC n=1944 rate 3/4");
          break;
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1944_R_5_6:
          return std::string("IEEE 802.11n QC-LDPC n=1944 rate 5/6");
          break;
        case ErrorCorrectionScheme::NO_FEC:
          return std::string("No FEC");
          break;

        default:
          throw ECException("Invalid Error Correction Coding value.");
          break;
      }
      return std::string("bad error coding name");
    }

    bool
    ErrorCorrection::isValid(ErrorCorrectionScheme scheme) {

      bool isValid = false;

      // @TODO This is so ugly
      switch(scheme) {
        // CCSDS convolutional codiing is supported.
        case ErrorCorrectionScheme::CCSDS_CONVOLUTIONAL_CODING_R_1_2:
        case ErrorCorrectionScheme::CCSDS_CONVOLUTIONAL_CODING_R_2_3:
        case ErrorCorrectionScheme::CCSDS_CONVOLUTIONAL_CODING_R_3_4:
        case ErrorCorrectionScheme::CCSDS_CONVOLUTIONAL_CODING_R_5_6:
        case ErrorCorrectionScheme::CCSDS_CONVOLUTIONAL_CODING_R_7_8:
          isValid = true;
          break;
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_239_INTERLEAVING_1:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_239_INTERLEAVING_2:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_239_INTERLEAVING_3:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_239_INTERLEAVING_4:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_239_INTERLEAVING_5:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_239_INTERLEAVING_8:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_223_INTERLEAVING_1:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_223_INTERLEAVING_2:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_223_INTERLEAVING_3:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_223_INTERLEAVING_4:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_223_INTERLEAVING_5:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_223_INTERLEAVING_8:
        case ErrorCorrectionScheme::CCSDS_TURBO_1784_R_1_2:
        case ErrorCorrectionScheme::CCSDS_TURBO_1784_R_1_3:
        case ErrorCorrectionScheme::CCSDS_TURBO_1784_R_1_4:
        case ErrorCorrectionScheme::CCSDS_TURBO_1784_R_1_6:
        case ErrorCorrectionScheme::CCSDS_TURBO_3568_R_1_2:
        case ErrorCorrectionScheme::CCSDS_TURBO_3568_R_1_3:
        case ErrorCorrectionScheme::CCSDS_TURBO_3568_R_1_4:
        case ErrorCorrectionScheme::CCSDS_TURBO_3568_R_1_6:
        case ErrorCorrectionScheme::CCSDS_TURBO_7136_R_1_2:
        case ErrorCorrectionScheme::CCSDS_TURBO_7136_R_1_3:
        case ErrorCorrectionScheme::CCSDS_TURBO_7136_R_1_4:
        case ErrorCorrectionScheme::CCSDS_TURBO_7136_R_1_6:
        case ErrorCorrectionScheme::CCSDS_TURBO_8920_R_1_2:
        case ErrorCorrectionScheme::CCSDS_TURBO_8920_R_1_3:
        case ErrorCorrectionScheme::CCSDS_TURBO_8920_R_1_4:
        case ErrorCorrectionScheme::CCSDS_TURBO_8920_R_1_6:
        case ErrorCorrectionScheme::CCSDS_LDPC_ORANGE_BOOK_1280:
        case ErrorCorrectionScheme::CCSDS_LDPC_ORANGE_BOOK_1536:
        case ErrorCorrectionScheme::CCSDS_LDPC_ORANGE_BOOK_2048:
          break;
          // IEEE QCLPDC is valid
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_648_R_1_2:
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_648_R_2_3:
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_648_R_3_4:
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_648_R_5_6:
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1296_R_1_2:
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1296_R_2_3:
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1296_R_3_4:
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1296_R_5_6:
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1944_R_1_2:
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1944_R_2_3:
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1944_R_3_4:
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1944_R_5_6:
          isValid = true;
          break;
          // No FEC scheme is valid
        case ErrorCorrectionScheme::NO_FEC:
          isValid = true;
          break;
        default:
          throw ECException("Invalid Error Correction Coding value.");
          break;
      }
      return isValid;
    }


    double
    ErrorCorrection::m_codingRateToFractionalRate()
    {
      double fractionalRate = 1.0; // assume no encoding
      // @TODO This is so ugly
      switch(m_codingRate) {
        case CodingRate::RATE_1_6:
          fractionalRate = 1.0/6.0;
          break;
        case CodingRate::RATE_1_5:
          fractionalRate = 0.20;
          break;
        case CodingRate::RATE_1_4:
          fractionalRate = 0.25;
          break;
        case CodingRate::RATE_1_3:
          fractionalRate = 1.0/3.0;
          break;
        case CodingRate::RATE_1_2:
          fractionalRate = 0.5;
          break;
        case CodingRate::RATE_2_3:
          fractionalRate = 2.0/3.0;
          break;
        case CodingRate::RATE_3_4:
          fractionalRate = 0.75;
          break;
        case CodingRate::RATE_4_5:
          fractionalRate = 0.8;
          break;
        case CodingRate::RATE_5_6:
          fractionalRate = 5.0/6.0;
          break;
        case CodingRate::RATE_7_8:
          fractionalRate = 7.0/8.0;
          break;
        case CodingRate::RATE_8_9:
          fractionalRate = 8.0/9.0;
          break;
        case CodingRate::RATE_1:
          fractionalRate = 1.0;
          break;
        case CodingRate::RATE_NA:
          break;
        default:
          throw ECException("Invalid Coding Rate value.");
          break;
      }
      return fractionalRate;
    }

    uint32_t
    ErrorCorrection::m_codewordLength()
    {
      uint32_t codewordLen = 0; // @TODO this may cause trouble, but is fine for now
      // @TODO This is so ugly
      switch(m_errorCorrectionScheme) {
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_239_INTERLEAVING_1:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_239_INTERLEAVING_2:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_239_INTERLEAVING_3:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_239_INTERLEAVING_4:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_239_INTERLEAVING_5:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_239_INTERLEAVING_8:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_223_INTERLEAVING_1:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_223_INTERLEAVING_2:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_223_INTERLEAVING_3:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_223_INTERLEAVING_4:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_223_INTERLEAVING_5:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_223_INTERLEAVING_8:
          codewordLen = 255*8; // bits
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_1784_R_1_2:
          codewordLen = 3576; // bits
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_1784_R_1_3:
          codewordLen = 5364; // bits
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_1784_R_1_4:
          codewordLen = 7152; // bits
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_1784_R_1_6:
          codewordLen = 10728; // bits
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_3568_R_1_2:
          codewordLen = 7144; // bits
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_3568_R_1_3:
          codewordLen = 10716; // bits
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_3568_R_1_4:
          codewordLen = 14288; // bits
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_3568_R_1_6:
          codewordLen = 21432; // bits
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_7136_R_1_2:
          codewordLen = 14280; // bits
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_7136_R_1_3:
          codewordLen = 21420; // bits
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_7136_R_1_4:
          codewordLen = 28560; // bits
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_7136_R_1_6:
          codewordLen = 42840; // bits
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_8920_R_1_2:
          codewordLen = 17848; // bits
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_8920_R_1_3:
          codewordLen = 26772; // bits
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_8920_R_1_4:
          codewordLen = 35696; // bits
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_8920_R_1_6:
          codewordLen = 53544; // bits
          break;
        case ErrorCorrectionScheme::CCSDS_LDPC_ORANGE_BOOK_1280:
          codewordLen = 1280; // bits
          break;
        case ErrorCorrectionScheme::CCSDS_LDPC_ORANGE_BOOK_1536:
          codewordLen = 1536; // bits
          break;
        case ErrorCorrectionScheme::CCSDS_LDPC_ORANGE_BOOK_2048:
          codewordLen = 2048; // bits
          break;
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_648_R_1_2:
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_648_R_2_3:
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_648_R_3_4:
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_648_R_5_6:
          codewordLen = 648; // bits
          break;
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1296_R_1_2:
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1296_R_2_3:
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1296_R_3_4:
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1296_R_5_6:
          codewordLen = 1296; // bits
          break;
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1944_R_1_2:
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1944_R_2_3:
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1944_R_3_4:
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1944_R_5_6:
          codewordLen = 1944; // bits
          break;

        // Set to the max codeword length specified  in the constructor
        case ErrorCorrectionScheme::CCSDS_CONVOLUTIONAL_CODING_R_1_2:
        case ErrorCorrectionScheme::CCSDS_CONVOLUTIONAL_CODING_R_2_3:
        case ErrorCorrectionScheme::CCSDS_CONVOLUTIONAL_CODING_R_3_4:
        case ErrorCorrectionScheme::CCSDS_CONVOLUTIONAL_CODING_R_5_6:
        case ErrorCorrectionScheme::CCSDS_CONVOLUTIONAL_CODING_R_7_8:
          codewordLen = m_continuousMaxCodewordLen;
          break;

        case ErrorCorrectionScheme::NO_FEC:
          // If there is no FEC scheme, the codeword and message are the same.
          // We might as well use what was set for continuous coders
          codewordLen = m_continuousMaxCodewordLen; // bits
          break;

        default:
          throw ECException("Invalid Error Correction Scheme.");
          break;
      }
      return codewordLen;
    }

    uint32_t
    ErrorCorrection::m_messageLength()
    {
      uint32_t messageLen = 0; // @TODO this may cause trouble, but is fine for now

      //      m_messageLen = (uint32_t) ((double) m_codewordLen * m_codingRateToFractionalRate());

      // @TODO This is so ugly
      switch(m_errorCorrectionScheme) {
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_239_INTERLEAVING_1:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_239_INTERLEAVING_2:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_239_INTERLEAVING_3:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_239_INTERLEAVING_4:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_239_INTERLEAVING_5:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_239_INTERLEAVING_8:
          messageLen = 239*8; // bits
          break;
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_223_INTERLEAVING_1:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_223_INTERLEAVING_2:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_223_INTERLEAVING_3:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_223_INTERLEAVING_4:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_223_INTERLEAVING_5:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_223_INTERLEAVING_8:
          messageLen = 223*8; // bits
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_1784_R_1_2:
        case ErrorCorrectionScheme::CCSDS_TURBO_1784_R_1_3:
        case ErrorCorrectionScheme::CCSDS_TURBO_1784_R_1_4:
        case ErrorCorrectionScheme::CCSDS_TURBO_1784_R_1_6:
          messageLen = 1784; // bits
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_3568_R_1_2:
        case ErrorCorrectionScheme::CCSDS_TURBO_3568_R_1_3:
        case ErrorCorrectionScheme::CCSDS_TURBO_3568_R_1_4:
        case ErrorCorrectionScheme::CCSDS_TURBO_3568_R_1_6:
          messageLen = 3568; // bits
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_7136_R_1_2:
        case ErrorCorrectionScheme::CCSDS_TURBO_7136_R_1_3:
        case ErrorCorrectionScheme::CCSDS_TURBO_7136_R_1_4:
        case ErrorCorrectionScheme::CCSDS_TURBO_7136_R_1_6:
          messageLen = 7136; // bits
          break;
        case ErrorCorrectionScheme::CCSDS_TURBO_8920_R_1_2:
        case ErrorCorrectionScheme::CCSDS_TURBO_8920_R_1_3:
        case ErrorCorrectionScheme::CCSDS_TURBO_8920_R_1_4:
        case ErrorCorrectionScheme::CCSDS_TURBO_8920_R_1_6:
          messageLen = 8920; // bits
          break;
        case ErrorCorrectionScheme::CCSDS_LDPC_ORANGE_BOOK_1280:
        case ErrorCorrectionScheme::CCSDS_LDPC_ORANGE_BOOK_1536:
        case ErrorCorrectionScheme::CCSDS_LDPC_ORANGE_BOOK_2048:
          messageLen = 1024; // bits
          break;
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_648_R_1_2:
          messageLen = 324; // bits
          break;
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_648_R_2_3:
          messageLen = 432; // bits
          break;
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_648_R_3_4:
          messageLen = 486; // bits
          break;
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_648_R_5_6:
          messageLen = 540; // bits
          break;
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1296_R_1_2:
          messageLen = 648; // bits
          break;
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1296_R_2_3:
          messageLen = 864; // bits
          break;
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1296_R_3_4:
          messageLen = 972; // bits
          break;
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1296_R_5_6:
          messageLen = 1080; // bits
          break;
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1944_R_1_2:
          messageLen = 972; // bits
          break;
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1944_R_2_3:
          messageLen = 1296; // bits
          break;
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1944_R_3_4:
          messageLen = 1458; // bits
          break;
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1944_R_5_6:
          messageLen = 1620; // bits
          break;

          // For convolutional codeing, set to the max codeword length specified
          // in the constructor, but adjust to account for the polynomial order K.
          //
          // In general, n = (m + (K -1)) / r, so m = n * r - (K - 1)

        case ErrorCorrectionScheme::CCSDS_CONVOLUTIONAL_CODING_R_1_2:
          // The CCSDS recommendation has K=7, which means that an extra 6 bits
          // are added to the codeword for a message of length m. Thus,
          // n = (m + (K - 1)) / r, where n is the codeword length and r is the
          // rate. Then m = n * r - (K - 1). We want a integral number of bytes,
          // so m is adjusted to be m = m - (m % 8).
          {
            uint32_t cwLen = m_codewordLength();
            messageLen = (uint32_t) (cwLen / 2.0 - (CCSDS_CONVOLUTIONAL_CODING_K - 1.0));
            messageLen -= (messageLen % 8);
          }
          break;
        case ErrorCorrectionScheme::CCSDS_CONVOLUTIONAL_CODING_R_2_3:
          {
            uint32_t cwLen = m_codewordLength();
            messageLen = (uint32_t) (cwLen * 2.0 / 3.0 - (CCSDS_CONVOLUTIONAL_CODING_K - 1.0));
            messageLen -= (messageLen % 8);
          }
          break;
        case ErrorCorrectionScheme::CCSDS_CONVOLUTIONAL_CODING_R_3_4:
          {
            uint32_t cwLen = m_codewordLength();
            messageLen = (uint32_t) (cwLen * 3.0  / 4.0 - (CCSDS_CONVOLUTIONAL_CODING_K - 1.0));
            messageLen -= (messageLen % 8);
          }
          break;
        case ErrorCorrectionScheme::CCSDS_CONVOLUTIONAL_CODING_R_5_6:
          {
            uint32_t cwLen = m_codewordLength();
            messageLen = (uint32_t) (cwLen * 5.0 / 6.0 - (CCSDS_CONVOLUTIONAL_CODING_K - 1.0));
            messageLen -= (messageLen % 8);
          }
          break;
        case ErrorCorrectionScheme::CCSDS_CONVOLUTIONAL_CODING_R_7_8:
          {
            uint32_t cwLen = m_codewordLength();
            messageLen = (uint32_t) (cwLen * 7.0 / 8.0 - (CCSDS_CONVOLUTIONAL_CODING_K - 1.0));
            messageLen -= (messageLen % 8);
          }
          break;

        case ErrorCorrectionScheme::NO_FEC:
          // If there is no FEC scheme, the codeword and message are the same.
          // We might as well use what was set for continuous coders
          messageLen = m_continuousMaxCodewordLen; // bits
          break;

        default:
          throw ECException("Invalid Error Correction Scheme.");
          break;
      }
      return messageLen;
    }

    ErrorCorrection::CodingRate
    ErrorCorrection::m_getCodingRate(ErrorCorrectionScheme scheme) {

      CodingRate r = ErrorCorrection::CodingRate::RATE_BAD;

      switch(scheme) {

        case ErrorCorrectionScheme::CCSDS_CONVOLUTIONAL_CODING_R_1_2:
        case ErrorCorrectionScheme::CCSDS_TURBO_1784_R_1_2:
        case ErrorCorrectionScheme::CCSDS_TURBO_3568_R_1_2:
        case ErrorCorrectionScheme::CCSDS_TURBO_7136_R_1_2:
        case ErrorCorrectionScheme::CCSDS_TURBO_8920_R_1_2:
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_648_R_1_2:
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1296_R_1_2:
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1944_R_1_2:
          r = ErrorCorrection::CodingRate::RATE_1_2;
          break;

        case ErrorCorrectionScheme::CCSDS_TURBO_1784_R_1_3:
        case ErrorCorrectionScheme::CCSDS_TURBO_3568_R_1_3:
        case ErrorCorrectionScheme::CCSDS_TURBO_7136_R_1_3:
        case ErrorCorrectionScheme::CCSDS_TURBO_8920_R_1_3:
          r = ErrorCorrection::CodingRate::RATE_1_3;
          break;

        case ErrorCorrectionScheme::CCSDS_TURBO_1784_R_1_4:
        case ErrorCorrectionScheme::CCSDS_TURBO_3568_R_1_4:
        case ErrorCorrectionScheme::CCSDS_TURBO_7136_R_1_4:
        case ErrorCorrectionScheme::CCSDS_TURBO_8920_R_1_4:
          r = ErrorCorrection::CodingRate::RATE_1_4;
          break;

        case ErrorCorrectionScheme::CCSDS_TURBO_1784_R_1_6:
        case ErrorCorrectionScheme::CCSDS_TURBO_3568_R_1_6:
        case ErrorCorrectionScheme::CCSDS_TURBO_7136_R_1_6:
        case ErrorCorrectionScheme::CCSDS_TURBO_8920_R_1_6:
          r = ErrorCorrection::CodingRate::RATE_1_6;
          break;

        case ErrorCorrectionScheme::CCSDS_CONVOLUTIONAL_CODING_R_2_3:
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_648_R_2_3:
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1296_R_2_3:
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1944_R_2_3:
          r = ErrorCorrection::CodingRate::RATE_2_3;
          break;

        case ErrorCorrectionScheme::CCSDS_CONVOLUTIONAL_CODING_R_3_4:
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_648_R_3_4:
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1296_R_3_4:
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1944_R_3_4:
          r = ErrorCorrection::CodingRate::RATE_3_4;
          break;

        case ErrorCorrectionScheme::CCSDS_CONVOLUTIONAL_CODING_R_5_6:
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_648_R_5_6:
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1296_R_5_6:
        case ErrorCorrectionScheme::IEEE_802_11N_QCLDPC_1944_R_5_6:
          r = ErrorCorrection::CodingRate::RATE_5_6;
          break;

        case ErrorCorrectionScheme::CCSDS_CONVOLUTIONAL_CODING_R_7_8:
          r = ErrorCorrection::CodingRate::RATE_7_8;
          break;

        case ErrorCorrectionScheme::CCSDS_LDPC_ORANGE_BOOK_1280:
          r = ErrorCorrection::CodingRate::RATE_4_5;
          break;
        case ErrorCorrectionScheme::CCSDS_LDPC_ORANGE_BOOK_1536:
          r = ErrorCorrection::CodingRate::RATE_2_3;
          break;
        case ErrorCorrectionScheme::CCSDS_LDPC_ORANGE_BOOK_2048:
          r = ErrorCorrection::CodingRate::RATE_1_2;
          break;

        case ErrorCorrectionScheme::NO_FEC:
          r = ErrorCorrection::CodingRate::RATE_1;
          break;

        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_239_INTERLEAVING_1:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_239_INTERLEAVING_2:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_239_INTERLEAVING_3:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_239_INTERLEAVING_4:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_239_INTERLEAVING_5:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_239_INTERLEAVING_8:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_223_INTERLEAVING_1:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_223_INTERLEAVING_2:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_223_INTERLEAVING_3:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_223_INTERLEAVING_4:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_223_INTERLEAVING_5:
        case ErrorCorrectionScheme::CCSDS_REED_SOLOMON_255_223_INTERLEAVING_8:
          r = ErrorCorrection::CodingRate::RATE_NA;
          break;

        default:
          r = ErrorCorrection::CodingRate::RATE_BAD;
          break;
      }
      return r;
    }

  } /* namespace darkstar */
} /* namespace xiphos */
