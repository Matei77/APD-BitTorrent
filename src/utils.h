#pragma once

#include <string>

constexpr int TRACKER_RANK = 0;
constexpr int MAX_FILES = 10;
constexpr int MAX_FILENAME = 15;
constexpr int HASH_SIZE = 32;
constexpr int MAX_CHUNKS = 100;

constexpr int OK = 1;
constexpr int STOP_PEER = 10;

constexpr int FILE_REQUEST = 2;
constexpr int PEER_UPDATE = 3;
constexpr int DOWNLOAD_COMPLETED = 4;
constexpr int ALL_DOWNLOADS_COMPLETED = 5;

constexpr int TRACKER_TAG = 1;
constexpr int DOWNLOAD_TAG = 2;
constexpr int UPLOAD_TAG = 3;

const std::string POISON_HASH = "0000000000000000000000000000000";

struct Segment {
	int nr;
	std::string hash;

	Segment(int nr, std::string hash) : nr(nr), hash(hash) {}

	bool operator==(const Segment& other) const {
        return hash == other.hash;
    }
};

template <>
struct std::hash<Segment> {
    std::size_t operator()(const Segment& segment) const {
		return std::hash<std::string>()(segment.hash);
	}
};