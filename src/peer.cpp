#include "peer.h"

#include <mpi.h>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <algorithm>

#include "utils.h"

using namespace std;

void Peer::start() {
	init();

	thread download_thread(&Peer::download_thread_func, this);
	thread upload_thread(&Peer::upload_thread_func, this);

	download_thread.join();
	upload_thread.join();
}

void Peer::download_thread_func() {
	// TODO: send owned files to tracker
	send_owned_files_to_tracker();

	// TODO: wait for response from tracker
	int response;
	MPI_Recv(&response, 1, MPI_INT, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	if (response != OK) {
		cout << "Tracker did not send OK message." << endl;
		exit(-1);
	}

	// TODO: get list of seeds/peers for the wanted files from the tracker
	for (auto wanted_file_name : wanted_files_) {
		std::string file_owners = send_file_request_to_tracker(wanted_file_name);

		unordered_map<Segment, vector<int>> file_owners_map = parse_file_owners(file_owners);

	// TODO: check for missing segements and ask peers for them
		// TODO: choose a peer
		// TODO: send request
		// TODO: wait for segment
		// TODO: mark the segment as received
		
		// pair.first = segment
		// pair.second = vector of peers that have that segment
		for (auto pair : file_owners_map) {
			int random_peer = pair.second[rand() % pair.second.size()];
			cout << "[Downloading " << wanted_file_name << "]: Peer " << rank_ << " will download segment " << pair.first.nr << " from peer " << random_peer << endl;
	
			MPI_Send(&pair.first.hash, HASH_SIZE, MPI_CHAR, random_peer, DOWNLOAD_TAG, MPI_COMM_WORLD);
			MPI_Recv(&response, 1, MPI_INT, random_peer, UPLOAD_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

			if (response != OK) {
				cout << "[ERROR] Downloading file " << wanted_file_name << " failed. - Could not get segment " << pair.first.nr << " from " << random_peer << endl;
			}

			owned_files_[wanted_file_name].emplace_back(pair.first.nr, pair.first.hash);

			// TODO: after 10 downloaded segments notify tracker

			// downloaded_segments_nr_++;
			// if (downloaded_segments_nr_ == 10) {
			// 	send_peer_update_to_tracker();
			// 	downloaded_segments_nr_ = 0;
			// }
		}

		std::cout << "[Downloading " << wanted_file_name << "]: Peer " << rank_ << " finished downloading." << std::endl;

		// TODO: when all segments from a file are downloaded notify tracker and save file
		send_download_completed_to_tracker(wanted_file_name);
		save_file(wanted_file_name);
	
	}
	// TODO: notify tracker when all downloads are done
	std::cout << "[Downloading]: Peer " << rank_ << " finished all downloads." << std::endl;
	send_all_downloads_completed_to_tracker();

	// TODO: wait for notification that all clients finished their downloads
	// MPI_Recv(&response, 1, MPI_INT, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	// if (response == STOP_PEER) {
	// 	upload_running_ = false;
	// }

}

void Peer::upload_thread_func() {
	while (1) {
		// TODO: wait for request
		char segment_hash[HASH_SIZE];
		MPI_Status status;
		MPI_Recv(segment_hash, HASH_SIZE, MPI_CHAR, MPI_ANY_SOURCE, DOWNLOAD_TAG, MPI_COMM_WORLD, &status);

		std::string segment_hash_str(segment_hash);
		if (segment_hash_str == POISON_HASH) {
			break;
		}

		// TODO: send requested segments to peers
		int source = status.MPI_SOURCE;
		MPI_Send(&OK, 1, MPI_INT, source, UPLOAD_TAG, MPI_COMM_WORLD);
	}
	std::cout << "[Uploading]: Peer " << rank_ << " finished all uploads." << std::endl;
}

void Peer::init() {
	ifstream fin("in" + to_string(rank_) + ".txt");
	int segment_nr;
	string file_name;
	string segment_hash;

	fin >> files_owned_nr_;
	for (int i = 0; i < files_owned_nr_; i++) {
		fin >> file_name >> segment_nr;
		vector<Segment> segments;
		for (int j = 1; j <= segment_nr; j++) {
			fin >> segment_hash;
			segments.emplace_back(j, segment_hash);
		}

		owned_files_.emplace(file_name, segments);
	}

	fin >> files_wanted_nr_;
	for (int i = 0; i < files_wanted_nr_; i++) {
		fin >> file_name;
		wanted_files_.emplace_back(file_name);
	}
}

void Peer::send_owned_files_to_tracker() {
	int size;

	std::string buf;
	buf += std::to_string(owned_files_.size()) + "\n";
	for (auto file : owned_files_) {
		buf += file.first + "\n";
		buf += std::to_string(file.second.size()) + "\n";
		for (auto segment : file.second) {
			buf += segment.hash + "\n";
		}
	}

	// std::cout << "Rank: " << rank_ << "\n" << buf << std::endl;

	size = buf.size();

	MPI_Send(&size, 1, MPI_INT, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD);
	MPI_Send(buf.c_str(), buf.size(), MPI_CHAR, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD);
}

std::string Peer::send_file_request_to_tracker(std::string file_name) {
	int file_name_size = file_name.size();

	MPI_Send(&FILE_REQUEST, 1, MPI_INT, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD);
	MPI_Send(&file_name_size, 1, MPI_INT, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD);
	MPI_Send(file_name.c_str(), file_name.size(), MPI_CHAR, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD);

	int size;
	MPI_Recv(&size, 1, MPI_INT, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	char response_buf[size + 1];
	MPI_Recv(response_buf, size, MPI_CHAR, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	std::string file_owners(response_buf);

	return file_owners;
}

void Peer::send_peer_update_to_tracker() {
	MPI_Send(&PEER_UPDATE, 1, MPI_INT, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD);
	send_owned_files_to_tracker();
}

void Peer::send_download_completed_to_tracker(std::string file_name) {
	int file_name_size = file_name.size();

	MPI_Send(&DOWNLOAD_COMPLETED, 1, MPI_INT, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD);
	MPI_Send(&file_name_size, 1, MPI_INT, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD);
	MPI_Send(file_name.c_str(), file_name.size(), MPI_CHAR, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD);
}

void Peer::send_all_downloads_completed_to_tracker() {
	MPI_Send(&ALL_DOWNLOADS_COMPLETED, 1, MPI_INT, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD);
}

std::unordered_map<Segment, std::vector<int>> Peer::parse_file_owners(std::string file_owners) {
	std::unordered_map<Segment, std::vector<int>> file_owners_map;
	std::stringstream ss(file_owners);
	std::string file_name;
	std::string segment_hash;
	int nr;
	int segment_nr;
	int peers_nr;

	ss >> nr;

	for (int i = 1; i <= nr; i++) {
		ss >> segment_nr >> segment_hash;

		Segment segment(segment_nr, segment_hash);
		if (file_owners_map.find(segment) == file_owners_map.end()) {
			file_owners_map.emplace(segment, std::vector<int>());
		}
		
		ss >> peers_nr;
		for (int j = 1; j <= peers_nr; j++) {
			int peer;
			ss >> peer;
			file_owners_map[segment].emplace_back(peer);
		}
	}
	
	return file_owners_map;
}

void Peer::save_file(string file_name) {
	ofstream fout("client" + to_string(rank_) + "_" + file_name);
	std::sort(owned_files_[file_name].begin(), owned_files_[file_name].end(), [](const Segment& lhs, const Segment& rhs) {
		return lhs.nr < rhs.nr;
	});

	for (auto segment : owned_files_[file_name]) {
		fout << segment.hash << "\n";
	}
	fout.close();
}
