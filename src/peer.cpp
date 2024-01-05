// Copyright: Ionescu Matei-Stefan - 333CA - 2023-2024
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
	// send owned files to tracker
	send_owned_files_to_tracker();

	// wait for response from tracker
	int response;
	MPI_Recv(&response, 1, MPI_INT, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	if (response != OK) {
		cout << "Tracker did not send OK message." << endl;
		exit(-1);
	}

	for (auto wanted_file_name : wanted_files_) {
		// get list of seeds/peers for the wanted file from the tracker
		string file_owners = send_file_request_to_tracker(wanted_file_name);

		// parse the list of seeds/peers
		unordered_map<Segment, vector<int>> file_owners_map = parse_file_owners(file_owners);
		
		// pair.first = segment
		// pair.second = vector of peers that have that segment
		for (auto pair : file_owners_map) {
			// choose a random peer that has the segment
			int random_peer = pair.second[rand() % pair.second.size()];

			#ifdef DEBUG
			cout << "[Downloading " << wanted_file_name << "]: Peer " << rank_ << " will download segment " <<
			pair.first.nr << " from peer " << random_peer << endl;
			#endif

			// ask the peer for the segment
			MPI_Send(&pair.first.hash, HASH_SIZE, MPI_CHAR, random_peer, DOWNLOAD_TAG, MPI_COMM_WORLD);
			MPI_Recv(&response, 1, MPI_INT, random_peer, UPLOAD_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

			// check the response
			if (response != OK) {
				cerr << "[ERROR] Downloading file " << wanted_file_name << " failed. - Could not get segment " <<
				pair.first.nr << " from " << random_peer << endl;
			}

			// add the segment to the owned files
			owned_files_[wanted_file_name].emplace_back(pair.first.nr, pair.first.hash);

			// after 10 downloaded segments notify tracker
			downloaded_segments_nr_++;
			if (downloaded_segments_nr_ == 10) {
				send_peer_update_to_tracker();
				downloaded_segments_nr_ = 0;
			}
		}

		#ifdef DEBUG
		cout << "[Downloading " << wanted_file_name << "]: Peer " << rank_ << " finished downloading." << endl;
		#endif

		// when all segments from a file are downloaded notify tracker and save file
		send_download_completed_to_tracker(wanted_file_name);
		save_file(wanted_file_name);
	
	}

	#ifdef DEBUG
	cout << "[Downloading]: Peer " << rank_ << " finished all downloads." << endl;
	#endif

	// notify tracker when all downloads are done
	send_all_downloads_completed_to_tracker();
}

void Peer::upload_thread_func() {
	while (1) {
		// wait for request
		char segment_hash[HASH_SIZE];
		MPI_Status status;
		MPI_Recv(segment_hash, HASH_SIZE, MPI_CHAR, MPI_ANY_SOURCE, DOWNLOAD_TAG, MPI_COMM_WORLD, &status);

		// check if the tracker told the upload thread to stop
		string segment_hash_str(segment_hash);
		if (segment_hash_str == POISON_HASH) {
			break;
		}

		// send requested segments to peers
		int source = status.MPI_SOURCE;
		MPI_Send(&OK, 1, MPI_INT, source, UPLOAD_TAG, MPI_COMM_WORLD);
	}

	#ifdef DEBUG
	cout << "[Uploading]: Peer " << rank_ << " Upload thread finished." << endl;
	#endif
}

void Peer::init() {
	ifstream fin("in" + to_string(rank_) + ".txt");
	int segment_nr;
	string file_name;
	string segment_hash;

	// read owned files
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

	// read wanted files
	fin >> files_wanted_nr_;
	for (int i = 0; i < files_wanted_nr_; i++) {
		fin >> file_name;
		wanted_files_.emplace_back(file_name);
	}

	fin.close();
}

string Peer::send_file_request_to_tracker(string file_name) {
	int file_name_size = file_name.size();
	
	// request the list of seeds/peers for the file
	MPI_Send(&FILE_REQUEST, 1, MPI_INT, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD);
	MPI_Send(&file_name_size, 1, MPI_INT, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD);
	MPI_Send(file_name.c_str(), file_name.size(), MPI_CHAR, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD);

	// wait for response from tracker
	int size;
	MPI_Recv(&size, 1, MPI_INT, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	char response_buf[size];
	MPI_Recv(response_buf, size, MPI_CHAR, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	// return the list of seeds/peers
	string file_owners(response_buf);
	return file_owners;
}

void Peer::send_peer_update_to_tracker() {
	// notify tracker that the peer has 10 new segments
	MPI_Send(&PEER_UPDATE, 1, MPI_INT, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD);

	// send the list of owned files to the tracker
	send_owned_files_to_tracker();
}

void Peer::send_download_completed_to_tracker(string file_name) {
	int file_name_size = file_name.size();

	// notify tracker that the peer finished downloading a file
	MPI_Send(&DOWNLOAD_COMPLETED, 1, MPI_INT, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD);
	MPI_Send(&file_name_size, 1, MPI_INT, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD);
	MPI_Send(file_name.c_str(), file_name.size(), MPI_CHAR, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD);
}

void Peer::send_all_downloads_completed_to_tracker() {
	// notify tracker that the peer finished all downloads
	MPI_Send(&ALL_DOWNLOADS_COMPLETED, 1, MPI_INT, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD);
}

void Peer::send_owned_files_to_tracker() {
	// create a buffer with the list of owned files
	string buf;
	buf += to_string(owned_files_.size()) + "\n";
	for (auto file : owned_files_) {
		buf += file.first + "\n";
		buf += to_string(file.second.size()) + "\n";
		for (auto segment : file.second) {
			buf += segment.hash + "\n";
		}
	}
	int size = buf.size();

	// send the list of owned files to the tracker
	MPI_Send(&size, 1, MPI_INT, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD);
	MPI_Send(buf.c_str(), buf.size(), MPI_CHAR, TRACKER_RANK, TRACKER_TAG, MPI_COMM_WORLD);
}

unordered_map<Segment, vector<int>> Peer::parse_file_owners(string file_owners) {
	unordered_map<Segment, vector<int>> file_owners_map;
	stringstream ss(file_owners);
	string file_name;
	string segment_hash;
	int nr;
	int segment_nr;
	int peers_nr;

	// parse the list of seeds/peers for the file
	ss >> nr;
	for (int i = 1; i <= nr; i++) {
		ss >> segment_nr >> segment_hash;

		Segment segment(segment_nr, segment_hash);
		if (file_owners_map.find(segment) == file_owners_map.end()) {
			file_owners_map.emplace(segment, vector<int>());
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

	// sort the segments by their order number
	sort(owned_files_[file_name].begin(), owned_files_[file_name].end(),
			  [](const Segment& s1, const Segment& s2) {
		return s1.nr < s2.nr;
	});

	// save the hash of each segment
	for (auto segment : owned_files_[file_name]) {
		fout << segment.hash << "\n";
	}
	fout.close();
}
