/**
 * Basic Environment for Game 2048
 * use 'g++ -std=c++11 -O3 -g -o 2048 2048.cpp' to compile the source
 *
 * Computer Games and Intelligence (CGI) Lab, NCTU, Taiwan
 * http://www.aigames.nctu.edu.tw
 */
#include <iostream>
#include <fstream>
#include <iterator>
#include <string>
#include "board.h"
#include "action.h"
#include "agent.h"
#include "episode.h"
#include "statistic.h"
#include <stdio.h>
#include<vector>
int main(int argc, const char* argv[]) {
	std::cout << "Thress-Demo: ";
	std::copy(argv, argv + argc, std::ostream_iterator<const char*>(std::cout, " "));
	std::cout << std::endl << std::endl;

	size_t total = 5000, block = 0, limit = 0;
	std::string play_args, evil_args;
	std::string load, save;
	bool summary = false;
	for (int i = 1; i < argc; i++) {
		std::string para(argv[i]);
		if (para.find("--total=") == 0) {
			total = std::stoull(para.substr(para.find("=") + 1));
		} else if (para.find("--block=") == 0) {
			block = std::stoull(para.substr(para.find("=") + 1));
		} else if (para.find("--limit=") == 0) {
			limit = std::stoull(para.substr(para.find("=") + 1));
		} else if (para.find("--play=") == 0) {
			play_args = para.substr(para.find("=") + 1);
		} else if (para.find("--evil=") == 0) {
			evil_args = para.substr(para.find("=") + 1);
		} else if (para.find("--load=") == 0) {
			load = para.substr(para.find("=") + 1);
		} else if (para.find("--save=") == 0) {
			save = para.substr(para.find("=") + 1);
		} else if (para.find("--summary") == 0) {
			summary = true;
		}
	}

	statistic stat(total, block, limit);

	if (load.size()) {
		std::ifstream in(load, std::ios::in);
		in >> stat;
		in.close();
		summary |= stat.is_finished();
	}

	weight_agent play(play_args);
	rndenv evil(evil_args);
	action move;
	int last_slide;
	action_op player_move;
	action_reward action_result;
	while (!stat.is_finished()) {
		play.open_episode("~:" + evil.name());
		evil.open_episode(play.name() + ":~");

		std::vector<std::vector<int>> state_index;
		std::vector<std::vector<int>> after_state_index;
		std::vector<int> rewards;

		stat.open_episode(play.name() + ":" + evil.name());
		episode& game = stat.back();
		last_slide = -1;
		std::vector<int> state;
		while (true) {
			agent& who = game.take_turns(play, evil);
			if(who.role().compare("player") == 0){
				player_move = who.take_action2(game.state());
				last_slide = player_move.op;	
				move = player_move.s;
			}
			else{

				move = who.take_action(game.state(),last_slide);
			}
			action_result = game.apply_action(move);
			if (who.role().compare("player") == 0){
				if(state.size() == 0){
					state = game.state().features();
				}else{
					state_index.push_back(state);
					after_state_index.push_back(game.state().features());
					rewards.push_back(action_result.reward);
					state = game.state().features();
				}
			}
			if (not action_result.legal_action or who.check_for_win(game.state())) break;
		}
		agent& win = game.last_turns(play, evil);
		stat.close_episode(win.name());
		play.update_weights(state_index, rewards, after_state_index);
		play.close_episode(win.name());
		evil.close_episode(win.name());
	}
	if (summary) {
		stat.summary();
	}

	if (save.size()) {
		std::ofstream out(save, std::ios::out | std::ios::trunc);
		out << stat;
		out.close();
	}

	return 0;
}
