/**
 * Copyright (c) 2026 Paul-Louis Ageneau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#if RTC_ENABLE_MEDIA

#include "vp8rtppacketizer.hpp"

#include <cstring>

namespace rtc {

VP8RtpPacketizer::VP8RtpPacketizer(shared_ptr<RtpPacketizationConfig> rtpConfig,
                                   size_t maxFragmentSize)
    : RtpPacketizer(std::move(rtpConfig)), mMaxFragmentSize(maxFragmentSize) {}

std::vector<binary> VP8RtpPacketizer::fragment(binary data) {
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

    const uint8_t N = 0b00100000;
    const uint8_t S = 0b00010000;

	if (data.empty())
		return {};

	const size_t descriptorSize = 1;
	if (mMaxFragmentSize <= descriptorSize)
		return {};

    bool isKeyframe = (std::to_integer<uint8_t>(data[0]) & 0b00000001) == 0;

	std::vector<binary> payloads;
	size_t index = 0;
	size_t remaining = data.size();
	bool firstFragment = true;

	while (remaining > 0) {
		size_t payloadSize = std::min(mMaxFragmentSize - descriptorSize, remaining);

		binary payload(descriptorSize + payloadSize);

		// Set 1-byte Payload Descriptor
		uint8_t descriptor = 0;
		if (!isKeyframe) {
			descriptor |= N;
        }
		if (firstFragment) {
			descriptor |= S;
			firstFragment = false;
		}

		payload[0] = std::byte(descriptor);

		// Copy data
		std::memcpy(payload.data() + descriptorSize, data.data() + index, payloadSize);

		payloads.push_back(std::move(payload));

		index += payloadSize;
		remaining -= payloadSize;
	}

	return payloads;
}

} // namespace rtc

#endif /* RTC_ENABLE_MEDIA */
