/**
 * Copyright (c) 2026 Paul-Louis Ageneau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#if RTC_ENABLE_MEDIA

#include "vp8rtpdepacketizer.hpp"
#include "rtp.hpp"

namespace rtc {

VP8RtpDepacketizer::VP8RtpDepacketizer() {}

VP8RtpDepacketizer::~VP8RtpDepacketizer() {}

message_ptr VP8RtpDepacketizer::reassemble(message_buffer &buffer) {
    /*
	 * VP8 payload descriptor
	 *
     *     0 1 2 3 4 5 6 7
     *    +-+-+-+-+-+-+-+-+
     *    |X|R|N|S|R| PID | (REQUIRED)
     *    +-+-+-+-+-+-+-+-+
     * X: |I|L|T|K| RSV   | (OPTIONAL)
     *    +-+-+-+-+-+-+-+-+
     * I: |M| PictureID   | (OPTIONAL)
     *    +-+-+-+-+-+-+-+-+
 	 *
	 * X: Extended control bits present
	 * R: Reserved (always 0)
	 * N: Non-reference frame
	 * S: Start of VP8 partition (1 for first fragment, 0 otherwise)
	 * PID: Partition index (assumed 0)
	 * I: PictureID present
	 * M: PictureID 15-byte extension flag
	 */

	// First byte
    const uint8_t X = 0b10000000;
    //const uint8_t N = 0b00100000;
    //const uint8_t S = 0b00010000;

	// Extension byte
	const uint8_t I = 0b10000000;
    const uint8_t L = 0b01000000;
    //const uint8_t T = 0b00100000;
    const uint8_t K = 0b00010000;

	// PictureID byte
	const uint8_t M = 0b10000000;

	if (buffer.empty())
		return nullptr;

	auto first = *buffer.begin();
	auto firstRtpHeader = reinterpret_cast<const RtpHeader *>(first->data());
	uint8_t payloadType = firstRtpHeader->payloadType();
	uint32_t timestamp = firstRtpHeader->timestamp();
	uint16_t nextSeqNumber = firstRtpHeader->seqNumber();

	binary frame;
	for (const auto &packet : buffer) {
		auto rtpHeader = reinterpret_cast<const rtc::RtpHeader *>(packet->data());
		if (rtpHeader->seqNumber() < nextSeqNumber) {
			// Skip
			continue;
		}

		nextSeqNumber = rtpHeader->seqNumber() + 1;

		auto rtpHeaderSize = rtpHeader->getSize() + rtpHeader->getExtensionHeaderSize();
		auto paddingSize = 0;
		if (rtpHeader->padding())
			paddingSize = std::to_integer<uint8_t>(packet->back());

		if (packet->size() <= rtpHeaderSize + paddingSize)
			continue; // Empty payload

		const std::byte *payloadData = packet->data() + rtpHeaderSize;
		size_t payloadLen = packet->size() - rtpHeaderSize - paddingSize;

		// VP8 Payload Descriptor (RFC 7741)
		if (payloadLen < 1)
			continue;

		size_t descriptorSize = 1;
		uint8_t firstByte = std::to_integer<uint8_t>(payloadData[0]);

		if (firstByte & X) {
			if (payloadLen < descriptorSize + 1)
				continue;

			uint8_t extensionByte = std::to_integer<uint8_t>(payloadData[descriptorSize]);
			descriptorSize++;

			if (extensionByte & I) {
				if (payloadLen < descriptorSize + 1)
					continue;
				uint8_t pictureIdByte = std::to_integer<uint8_t>(payloadData[descriptorSize]);
				descriptorSize++;
				if (pictureIdByte & M) { // M bit, 16-bit PictureID
					if (payloadLen < descriptorSize + 1)
						continue;
					descriptorSize++;
				}
			}

			if (extensionByte & L) {
				if (payloadLen < descriptorSize + 1)
					continue;
				descriptorSize++;
			}

			if ((extensionByte & L) || (extensionByte & K)) {
				if (payloadLen < descriptorSize + 1)
					continue;
				descriptorSize++;
			}
		}

		if (payloadLen <= descriptorSize)
			continue;

		frame.insert(frame.end(), packet->begin() + rtpHeaderSize + descriptorSize,
		             packet->end() - paddingSize);
	}

	return make_message(std::move(frame), createFrameInfo(timestamp, payloadType));
}

} // namespace rtc

#endif // RTC_ENABLE_MEDIA
